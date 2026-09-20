#ifndef _WIN32
#define _POSIX_C_SOURCE 200112L
#endif
#include "ps.h"
#include <errno.h>
#include <fcntl.h>
#ifdef _WIN32
#include <io.h>
#include <sys/stat.h>
#define CREATE_NEW(p) _open(p, _O_WRONLY | _O_CREAT | _O_EXCL | _O_BINARY, _S_IREAD | _S_IWRITE)
#define FDOPEN _fdopen
#define CLOSE _close
#else
#include <unistd.h>
#define CREATE_NEW(p) open(p, O_WRONLY | O_CREAT | O_EXCL, 0666)
#define FDOPEN fdopen
#define CLOSE close
#endif

void fail(const char *message)
{
    ps_raise(message);
    fprintf(stderr, "partystuffer: %s\n", message);
    exit(EXIT_FAILURE);
}

void *alloc(size_t n)
{
    void *p;
    if (n > LIMIT) fail("allocation exceeds 64 MiB safety limit");
    p = malloc(n ? n : 1);
    if (!p) fail("out of memory");
    return p;
}

U32 be32(const unsigned char *p)
{
    return ((U32)p[0] << 24) | ((U32)p[1] << 16) |
           ((U32)p[2] << 8) | (U32)p[3];
}

void put32(unsigned char *p, U32 x)
{
    p[0] = (unsigned char)(x >> 24);
    p[1] = (unsigned char)(x >> 16);
    p[2] = (unsigned char)(x >> 8);
    p[3] = (unsigned char)x;
}

Buf read_file(const char *path)
{
    FILE *f;
    long len;
    Buf b;
    f = fopen(path, "rb");
    if (!f) { fprintf(stderr, "%s: %s\n", path, strerror(errno)); fail("cannot open input"); }
    if (fseek(f, 0, SEEK_END) || (len = ftell(f)) < 0 ||
        (unsigned long)len > LIMIT || fseek(f, 0, SEEK_SET))
        fail("input is not seekable or exceeds 64 MiB");
    b.n = b.cap = (size_t)len;
    b.p = alloc(b.n);
    if (fread(b.p, 1, b.n, f) != b.n) fail("short input read");
    if (fclose(f)) fail("input close failed");
    return b;
}

void write_new(const char *path, const unsigned char *p, size_t n)
{
    FILE *f;
    int fd;
    fd = CREATE_NEW(path);
    if (fd < 0) {
        fprintf(stderr, "%s: %s\n", path, strerror(errno));
        fail("cannot create output; output paths must not already exist");
    }
    f = FDOPEN(fd, "wb");
    if (!f) { CLOSE(fd); remove(path); fail("cannot open output stream"); }
    if (fwrite(p, 1, n, f) != n) { fclose(f); remove(path); fail("output write failed"); }
    if (fclose(f)) { remove(path); fail("output close failed"); }
}

void append(Buf *b, const void *p, size_t n)
{
    size_t cap;
    unsigned char *q;
    if (n > LIMIT || b->n > LIMIT - n) fail("buffer exceeds 64 MiB");
    if (b->n + n > b->cap) {
        cap = b->cap ? b->cap : 256;
        while (cap < b->n + n) {
            if (cap > LIMIT / 2) { cap = LIMIT; break; }
            cap *= 2;
        }
        q = realloc(b->p, cap);
        if (!q) fail("out of memory");
        b->p = q; b->cap = cap;
    }
    if (n) memcpy(b->p + b->n, p, n);
    b->n += n;
}

void byte(Buf *b, unsigned int x)
{
    unsigned char c;
    c = (unsigned char)x;
    append(b, &c, 1);
}

void zeros(Buf *b, size_t n)
{
    static const unsigned char z[256] = {0};
    size_t k;
    while (n) { k = n > sizeof(z) ? sizeof(z) : n; append(b, z, k); n -= k; }
}

void align2(Buf *b)
{
    if (b->n & 1) byte(b, 0);
}

U32 crc32(const unsigned char *p, size_t n)
{
    U32 c;
    unsigned int j;
    c = (U32)0xffffffffUL;
    while (n--) {
        c ^= *p++;
        for (j = 0; j < 8; ++j)
            c = (c >> 1) ^ ((c & 1) ? (U32)0xedb88320UL : 0);
    }
    return c ^ (U32)0xffffffffUL;
}

void swap_order(Buf *rom, int order)
{
    size_t i;
    unsigned char c;
    if (order == 1) {
        for (i = 0; i < rom->n; i += 2) {
            c = rom->p[i]; rom->p[i] = rom->p[i+1]; rom->p[i+1] = c;
        }
    } else if (order == 2) {
        for (i = 0; i < rom->n; i += 4) {
            c = rom->p[i]; rom->p[i] = rom->p[i+3]; rom->p[i+3] = c;
            c = rom->p[i+1]; rom->p[i+1] = rom->p[i+2]; rom->p[i+2] = c;
        }
    }
}

int rom_order(Buf *rom)
{
    int order;
    U32 magic;
    if (rom->n < 0x101000UL || (rom->n & 3)) fail("ROM is too short or not word aligned");
    magic = be32(rom->p);
    if (magic == (U32)0x80371240UL) order = 0;
    else if (magic == (U32)0x37804012UL) order = 1;
    else if (magic == (U32)0x40123780UL) order = 2;
    else { fail("unrecognized N64 byte order"); return 0; }
    swap_order(rom, order);
    return order;
}

int cic_detect(const Buf *rom)
{
    switch (crc32(rom->p + 0x40, 0xfc0)) {
    case 0x6170a4a1UL: return 6101;
    case 0x90bb6cb5UL: return 6102;
    case 0x0b050ee0UL: return 6103;
    case 0x98bc2c86UL: return 6105;
    case 0xacc8580aUL: return 6106;
    default: return 0;
    }
}

/* N64 IPL3 checksum arithmetic, modulo 2^32. See docs in README. */
void checksum(Buf *rom, int cic, U32 *a, U32 *b)
{
    U32 sum, carry, xors, rotations, mixed, accum, seed, word, rot, next;
    unsigned int shift;
    size_t i;
    switch (cic) {
    case 6101: case 6102: seed = (U32)0xf8ca4ddcUL; break;
    case 6103: seed = (U32)0xa3886759UL; break;
    case 6105: seed = (U32)0xdf26f436UL; break;
    case 6106: seed = (U32)0x1fea617aUL; break;
    default: fail("unknown CIC; specify --cic 6101, 6102, 6103, 6105 or 6106"); return;
    }
    sum = carry = xors = rotations = mixed = accum = seed;
    for (i = 0x1000; i < 0x101000UL; i += 4) {
        word = be32(rom->p + i);
        next = sum + word;
        if (next < sum) ++carry;
        sum = next;
        xors ^= word;
        shift = (unsigned int)(word & 31);
        rot = shift ? (word << shift) | (word >> (32 - shift)) : word;
        rotations += rot;
        mixed ^= mixed > word ? rot : sum ^ word;
        accum += (cic == 6105 ? be32(rom->p + 0x750 + (i & 255)) : rotations) ^ word;
    }
    if (cic == 6103) { *a = (sum ^ carry) + xors; *b = (rotations ^ mixed) + accum; }
    else if (cic == 6106) { *a = sum * carry + xors; *b = rotations * mixed + accum; }
    else { *a = sum ^ carry ^ xors; *b = rotations ^ mixed ^ accum; }
}
