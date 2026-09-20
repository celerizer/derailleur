/* Nintendo B1 / Hudson S2, T3, MBF0, SBF0 sample extraction.
 * Format references and limitations are documented in SOUND.md.
 * No bank bytes are interpreted as PCM without a wavetable descriptor. */
#include "ps.h"
#include "mp2_voice_rates.h"
#include <errno.h>
#ifdef _WIN32
#include <direct.h>
#define AUDIO_MKDIR(p) _mkdir(p)
#else
#include <sys/stat.h>
#define AUDIO_MKDIR(p) mkdir(p, 0777)
#endif

#define AUDIO_PATH 4096

/* Set only during a synchronous pack traversal; source ROM stays immutable. */
static Buf *packing_rom;
static int packing_binary;
#define MAX_AUDIO_ITEMS 65536UL

typedef struct {
    size_t wave, rate, id;
} Sample;

typedef struct {
    const Buf *rom;
    size_t base, end, ctl, ctl_size, tbl, tbl_size;
    char root[AUDIO_PATH];
    Sample *samples;
    size_t count;
    Buf listing, references;
} Bank;

static unsigned int a16(const unsigned char *p)
{
    return (unsigned int)p[0] * 256U + p[1];
}

static long signed16(const unsigned char *p)
{
    unsigned int n;
    n = a16(p);
    return n < 32768U ? (long)n : (long)n - 65536L;
}

static void little16(unsigned char *p, unsigned int n)
{
    p[0] = (unsigned char)n; p[1] = (unsigned char)(n >> 8);
}

static void little32(unsigned char *p, U32 n)
{
    p[0] = (unsigned char)n; p[1] = (unsigned char)(n >> 8);
    p[2] = (unsigned char)(n >> 16); p[3] = (unsigned char)(n >> 24);
}

/* Overflow-free bounded integer operations for all ROM-relative pointers. */
static void bounds(size_t size, size_t p, size_t n)
{
    if (p > size || n > size - p) fail("audio pointer or length outside its section");
}

static const unsigned char *rom_at(const Buf *r, size_t p, size_t n)
{
    bounds(r->n, p, n); return r->p + p;
}

static size_t word(const Buf *r, size_t p)
{
    return (size_t)be32(rom_at(r, p, 4));
}

static const unsigned char *control(const Bank *b, size_t p, size_t n)
{
    bounds(b->ctl_size, p, n);
    return rom_at(b->rom, b->ctl + p, n);
}

static size_t relative(const Bank *b, size_t p, size_t n)
{
    bounds(b->end - b->base, p, n); return b->base + p;
}

/* Wave payloads may be relocated beyond their original container. The game
 * adds this offset to the container ROM base. Relocated control banks use
 * the same ROM-relative addressing and remain loaded into resident RAM. */
static size_t wave_table(const Bank *b, size_t offset, size_t length)
{
    if (offset > b->rom->n-b->base) fail("wave table offset outside ROM");
    rom_at(b->rom,b->base+offset,length); return b->base+offset;
}

static void join(char *p, const char *root, const char *name)
{
    if (strlen(root) + strlen(name) + 2 > AUDIO_PATH) fail("audio path too long");
    strcpy(p, root); strcat(p, "/"); strcat(p, name);
}

static void directory(const char *p)
{
    if (packing_rom) return;
    if (AUDIO_MKDIR(p)) {
        fprintf(stderr, "%s: %s\n", p, strerror(errno));
        fail("cannot create audio directory; destination must be new");
    }
}

static void save(const char *root, const char *name, const unsigned char *p, size_t n)
{
    char path[AUDIO_PATH];
    if (packing_rom) return;
    join(path, root, name); write_new(path, p, n);
}

static void raw_section(Bank *b, const char *name, size_t p, size_t n)
{
    if (p < b->base) fail("audio section precedes container");
    bounds(b->end - b->base, p - b->base, n);
    save(b->root, name, rom_at(b->rom, p, n), n);
}

/* Decode standard 9-byte/16-sample N64 VADPCM frames. The order-2
 * predictor uses the previous two decoded samples and the current block's
 * residuals, NOT its already reconstructed samples. Two 8-sample blocks
 * share each frame header. Saturation happens after the Q11 accumulator.
 * Double holds these integer sums exactly and avoids signed C89 overflow.
 */
Buf audio_pcm(const unsigned char *data, size_t length, unsigned int type,
              const unsigned char *book, size_t book_size)
{
    Buf pcm;
    size_t frames, frame, block, i, j, p, predictors, pred;
    unsigned int scale, nibble;
    long last1, last2, residual[8], output[8], value;
    double sum;
    const unsigned char *coeff;
    pcm.p = NULL; pcm.n = pcm.cap = 0;
    if (type == 1) {
        if (length & 1) fail("odd-sized raw 16-bit audio");
        zeros(&pcm, length);
        for (i = 0; i < length; i += 2) {
            pcm.p[i] = data[i + 1]; pcm.p[i + 1] = data[i];
        }
        return pcm;
    }
    if (type != 0) fail("unsupported N64 audio codec");
    if (book_size < 8 || be32(book) != 2) fail("unsupported ADPCM book order (expected 2)");
    predictors = (size_t)be32(book + 4);
    if (!predictors || predictors > 16 || book_size != 8 + predictors * 32)
        fail("invalid ADPCM predictor book");
    /* SDK banks align the encoded payload to two bytes. One trailing byte
     * is therefore storage padding, never an extra partial ADPCM frame. */
    if (length % 9 > 1) fail("truncated ADPCM frame");
    frames = length / 9;
    if (frames > (LIMIT - 128) / 32) fail("decoded sound too large");
    zeros(&pcm, frames * 32);
    last1 = last2 = 0;
    for (frame = 0; frame < frames; ++frame) {
        p = frame * 9;
        scale = data[p] >> 4; pred = data[p] & 15U;
        if (pred >= predictors) fail("ADPCM frame selects nonexistent predictor");
        if (scale > 12) scale = 12;
        coeff = book + 8 + pred * 32;
        for (block = 0; block < 2; ++block) {
            for (i = 0; i < 8; ++i) {
                nibble = data[p + 1 + block * 4 + i / 2];
                nibble = i & 1 ? nibble & 15U : nibble >> 4;
                residual[i] = (nibble < 8 ? (long)nibble : (long)nibble - 16) * (1L << scale);
            }
            for (i = 0; i < 8; ++i) {
                sum = (double)residual[i] * 2048.0;
                sum += (double)signed16(coeff + i * 2) * last1;
                sum += (double)signed16(coeff + 16 + i * 2) * last2;
                for (j = 0; j < i; ++j)
                    sum += (double)signed16(coeff + 16 + (i - j - 1) * 2) * residual[j];
                if (sum >= 32767.0 * 2048.0) value = 32767;
                else if (sum <= -32768.0 * 2048.0) value = -32768;
                else {
                    value = (long)(sum / 2048.0);
                    if (sum < (double)value * 2048.0) --value;
                }
                output[i] = value;
                little16(pcm.p + frame * 32 + block * 16 + i * 2,
                         (unsigned int)(value < 0 ? value + 65536L : value));
            }
            last1 = output[6]; last2 = output[7];
        }
    }
    return pcm;
}

static void wav(const char *root, const char *name, const Buf *pcm, size_t rate,
                size_t start, size_t end, U32 loops)
{
    Buf out;
    unsigned char h[44], smpl[68];
    int looping;
    looping = loops != 0 && start < end && end <= pcm->n / 2;
    memset(h, 0, sizeof(h)); memcpy(h, "RIFF", 4);
    little32(h + 4, (U32)(36 + pcm->n + (looping ? 68 : 0)));
    memcpy(h + 8, "WAVEfmt ", 8); little32(h + 16, 16);
    little16(h + 20, 1); little16(h + 22, 1);
    little32(h + 24, (U32)rate); little32(h + 28, (U32)(rate * 2));
    little16(h + 32, 2); little16(h + 34, 16);
    memcpy(h + 36, "data", 4); little32(h + 40, (U32)pcm->n);
    out.p = NULL; out.n = out.cap = 0;
    append(&out, h, sizeof(h)); append(&out, pcm->p, pcm->n);
    if (looping) {
        memset(smpl, 0, sizeof(smpl)); memcpy(smpl, "smpl", 4);
        little32(smpl + 4, 60); little32(smpl + 16, (U32)(1000000000UL / rate));
        little32(smpl + 20, 60); little32(smpl + 36, 1);
        little32(smpl + 52, (U32)start); little32(smpl + 56, (U32)(end - 1));
        little32(smpl + 64, loops == (U32)0xffffffffUL ? 0 : loops);
        append(&out, smpl, sizeof(smpl));
    }
    save(root, name, out.p, out.n); free(out.p);
}

static U32 le32(const unsigned char *p)
{
    return (U32)p[0] | ((U32)p[1] << 8) | ((U32)p[2] << 16) | ((U32)p[3] << 24);
}

static long pcm_value(const unsigned char *p)
{
    unsigned char v[2]; v[0] = p[1]; v[1] = p[0]; return signed16(v);
}

/* Search the original predictor book and all legal scales. Quantize each
 * residual against its reconstructed history, then minimize frame error. */
static Buf encode_sample(const Buf *pcm, const unsigned char *book, size_t length)
{
    Buf out;
    size_t f, pred, scale, block, i, j, np;
    long h1, h2, a, b, residual[8], decoded[8], q, value;
    long best1, best2;
    double sum, want, error, best_error, difference;
    unsigned char frame[9], best[9];
    const unsigned char *coeff;
    out.p = NULL; out.n = out.cap = 0; zeros(&out, length);
    np = (size_t)be32(book + 4); h1 = h2 = 0;
    for (f = 0; f < pcm->n / 32; ++f) {
        best_error = -1; best1 = best2 = 0;
        for (pred = 0; pred < np; ++pred) for (scale = 0; scale <= 12; ++scale) {
            memset(frame, 0, sizeof(frame)); frame[0] = (unsigned char)(scale * 16 + pred);
            coeff = book + 8 + pred * 32; a = h1; b = h2; error = 0;
            for (block = 0; block < 2; ++block) {
                for (i = 0; i < 8; ++i) {
                    sum = (double)signed16(coeff + i * 2) * a +
                          (double)signed16(coeff + 16 + i * 2) * b;
                    for (j = 0; j < i; ++j)
                        sum += (double)signed16(coeff + 16 + (i-j-1)*2) * residual[j];
                    value = pcm_value(pcm->p + f*32 + block*16 + i*2);
                    want = ((double)value - sum / 2048.0) / (double)(1L << scale);
                    q = want > 7 ? 7 : want < -8 ? -8 : (long)(want < 0 ? want - 0.5 : want + 0.5);
                    residual[i] = q * (1L << scale);
                    sum += (double)residual[i] * 2048.0;
                    if (sum >= 32767.0*2048.0) decoded[i] = 32767;
                    else if (sum <= -32768.0*2048.0) decoded[i] = -32768;
                    else {
                        decoded[i] = (long)(sum / 2048.0);
                        if (sum < (double)decoded[i]*2048.0) --decoded[i];
                    }
                    difference = (double)value - decoded[i]; error += difference*difference;
                    frame[1+block*4+i/2] |= (unsigned char)((q < 0 ? q+16 : q) << (i & 1 ? 0 : 4));
                }
                a = decoded[6]; b = decoded[7];
            }
            if (best_error < 0 || error < best_error) {
                best_error = error; memcpy(best, frame, 9); best1 = a; best2 = b;
            }
        }
        memcpy(out.p + f*9, best, 9); h1 = best1; h2 = best2;
    }
    return out;
}

static void import_wav(Bank *b, const char *name, const Buf *original,
                       size_t offset, size_t length, size_t rate,
                       unsigned int type, const unsigned char *book, size_t book_size, size_t loop)
{
    char path[AUDIO_PATH];
    Buf file, pcm, encoded;
    size_t p, n, data, bytes, i;
    int format;
    join(path, b->root, name); file = read_file(path);
    bounds(file.n, 0, 12);
    if (memcmp(file.p, "RIFF", 4) || memcmp(file.p+8, "WAVE", 4) ||
        (size_t)le32(file.p+4) != file.n-8) fail("invalid RIFF WAV file");
    data = bytes = 0; format = 0; p = 12;
    while (p < file.n) {
        bounds(file.n, p, 8); n = (size_t)le32(file.p+p+4); p += 8;
        bounds(file.n, p, n);
        if (!memcmp(file.p+p-8, "fmt ", 4)) {
            if (format || n < 16 || file.p[p] != 1 || file.p[p+1] ||
                file.p[p+2] != 1 || file.p[p+3] || le32(file.p+p+4) != rate ||
                le32(file.p+p+8) != rate*2 || file.p[p+12] != 2 || file.p[p+13] ||
                file.p[p+14] != 16 || file.p[p+15])
                fail("WAV must be mono PCM16 at its original sample rate");
            format = 1;
        } else if (!memcmp(file.p+p-8, "data", 4)) {
            if (data) fail("multiple WAV data chunks");
            data = p; bytes = n;
        }
        bounds(file.n, p, n + (n & 1)); p += n + (n & 1);
    }
    if (!format || !data || bytes != original->n)
        fail("WAV must retain its exported sample count");
    pcm.p = file.p+data; pcm.n = pcm.cap = bytes;
    if (memcmp(pcm.p, original->p, bytes)) {
        if (type == 0) encoded = encode_sample(&pcm, book, length);
        else {
            encoded.p = NULL; encoded.n = encoded.cap = 0; zeros(&encoded, length);
            for (i = 0; i < length; i += 2) { encoded.p[i] = pcm.p[i+1]; encoded.p[i+1] = pcm.p[i]; }
        }
        /* One wave can be referenced at several playback rates. */
        if (memcmp(packing_rom->p+offset, b->rom->p+offset, length) &&
            memcmp(packing_rom->p+offset, encoded.p, length))
            fail("conflicting WAV edits share the same ROM sample");
        memcpy(packing_rom->p+offset, encoded.p, length);
        if (type == 0 && loop) {
            Buf decoded;
            size_t start;
            start = (size_t)be32(control(b, loop, 44));
            start &= ~(size_t)15;
            decoded = audio_pcm(encoded.p, encoded.n, 0, book, book_size);
            bounds(decoded.n / 2, start, 16);
            for (i = 0; i < 32; i += 2) {
                packing_rom->p[b->ctl+loop+12+i] = decoded.p[start*2+i+1];
                packing_rom->p[b->ctl+loop+13+i] = decoded.p[start*2+i];
            }
            free(decoded.p);
        }
        free(encoded.p);
    }
    free(file.p);
}

static size_t sample(Bank *b, size_t wave, size_t rate, const char *rate_source)
{
    const unsigned char *w, *book, *loop;
    size_t i, length, offset, book_offset, book_size, start, end, lp;
    U32 loops;
    unsigned int type;
    Buf pcm;
    char name[96], line[512];
    if (rate < 1000 || rate > 192000) fail("invalid audio sample rate");
    for (i = 0; i < b->count; ++i)
        if (b->samples[i].wave == wave && b->samples[i].rate == rate) return i;
    if (b->count >= MAX_AUDIO_ITEMS) fail("too many audio samples");
    w = control(b, wave, 20); offset = (size_t)be32(w); length = (size_t)be32(w + 4);
    type = w[8];
    if (w[9]) fail("audio wavetable has already-patched pointers");
    bounds(b->tbl_size, offset, length);
    book = NULL; book_size = 0;
    if (!type) {
        book_offset = (size_t)be32(w + 16); book = control(b, book_offset, 8);
        if (be32(book) != 2 || !be32(book + 4) || be32(book + 4) > 16)
            fail("invalid ADPCM book header");
        book_size = 8 + (size_t)be32(book + 4) * 32;
        book = control(b, book_offset, book_size);
    }
    lp = (size_t)be32(w + 12); start = end = 0; loops = 0;
    if (lp) {
        loop = control(b, lp, type == 0 ? 44 : 12);
        start = (size_t)be32(loop); end = (size_t)be32(loop + 4); loops = be32(loop + 8);
    }
    pcm = audio_pcm(rom_at(b->rom, b->tbl + offset, length), length, type, book, book_size);
    i = b->count++;
    b->samples[i].wave = wave; b->samples[i].rate = rate; b->samples[i].id = i;
    sprintf(name, "samples/%04lX.wav", (unsigned long)i);
    if (packing_rom && !packing_binary)
        import_wav(b, name, &pcm, b->tbl + offset, length, rate, type, book, book_size, lp);
    else if (!packing_rom) wav(b->root, name, &pcm, rate, start, end, loops);
    sprintf(line, "%04lX,%08lX,%08lX,%lu,%u,%lu,%s,%lu,%lu,%lu,%lu\n",
            (unsigned long)i, (unsigned long)(b->ctl + wave), (unsigned long)(b->tbl + offset),
            (unsigned long)length, type, (unsigned long)rate, rate_source,
            (unsigned long)(pcm.n / 2), (unsigned long)start, (unsigned long)end, (unsigned long)loops);
    append(&b->listing, line, strlen(line)); free(pcm.p);
    return i;
}

static void instrument(Bank *b, size_t inst, size_t bank_id, long inst_id, size_t rate)
{
    const unsigned char *p, *s, *key;
    size_t count, j, snd, sid, k;
    char line[256];
    if (!inst) return;
    p = control(b, inst, 16); count = a16(p + 14);
    if (p[3]) fail("audio instrument has already-patched pointers");
    control(b, inst, 16 + count * 4);
    for (j = 0; j < count; ++j) {
        snd = (size_t)be32(control(b, inst + 16 + j * 4, 4));
        if (!snd) continue;
        s = control(b, snd, 16);
        if (s[14]) fail("audio sound has already-patched pointers");
        k = (size_t)be32(s + 4); key = control(b, k, 6);
        sid = sample(b, (size_t)be32(s + 8), rate, "bank");
        sprintf(line, "%lu,%ld,%lu,%04lX,%u,%u,%u,%ld\n", (unsigned long)bank_id,
                inst_id, (unsigned long)j, (unsigned long)sid, key[2], key[3], key[4],
                key[5] < 128 ? (long)key[5] : (long)key[5] - 256);
        append(&b->references, line, strlen(line));
    }
}

static void b1(Bank *b)
{
    const unsigned char *p;
    size_t count, i, j, bank, n, rate;
    p = control(b, 0, 4);
    if (a16(p) != 0x4231) fail("expected Nintendo B1 sound bank");
    count = a16(p + 2); control(b, 4, count * 4);
    append(&b->references, "bank,instrument,sound,sample,key_min,key_max,key_base,detune\n", 61);
    for (i = 0; i < count; ++i) {
        bank = (size_t)be32(control(b, 4 + i * 4, 4));
        if (!bank) continue;
        p = control(b, bank, 12); n = a16(p); rate = (size_t)be32(p + 4);
        if (p[2]) fail("audio bank has already-patched pointers");
        control(b, bank, 12 + n * 4);
        instrument(b, (size_t)be32(p + 8), i, -1, rate);
        for (j = 0; j < n; ++j)
            instrument(b, (size_t)be32(control(b, bank + 12 + j * 4, 4)), i, (long)j, rate);
    }
}

static void sequence(Bank *b, size_t id, size_t offset, size_t length, Buf *csv)
{
    char path[80], line[160];
    if (!length) return;
    relative(b, offset, length);
    sprintf(path, "sequences/%04lX.seq", (unsigned long)id);
    raw_section(b, path, b->base + offset, length);
    sprintf(line, "%04lX,%08lX,%lu\n", (unsigned long)id,
            (unsigned long)(b->base + offset), (unsigned long)length);
    append(csv, line, strlen(line));
}

static void music(Bank *b, int old)
{
    size_t count, i, meta, ctl, size, tbl, length, offset, first;
    Buf csv;
    char path[AUDIO_PATH];
    count = old ? a16(rom_at(b->rom, b->base + 2, 2)) : word(b->rom, b->base + 4);
    if (!count || count > 4096) fail("invalid music sequence count");
    meta = (old ? 4 : 64) + count * (old ? 8 : 16);
    relative(b, 0, meta + (old ? count * 16 : 16));
    first = b->end - b->base;
    csv.p = NULL; csv.n = csv.cap = 0;
    append(&csv, "sequence,rom_offset,bytes\n", 26);
    join(path, b->root, "sequences"); directory(path);
    for (i = 0; i < count; ++i) {
        offset = old ? 4 + i * 8 : 64 + i * 16 + 8;
        length = word(b->rom, b->base + offset + 4) & 0x7fffffffUL;
        offset = word(b->rom, b->base + offset);
        if (length && offset < first) first = offset;
        if (!old && rom_at(b->rom, b->base + 64 + i * 16, 4)[3])
            fail("multiple MBF0 bank sets are not supported");
        sequence(b, i, offset, length, &csv);
    }
    ctl = word(b->rom, b->base + meta + (old ? 4 : 0));
    size = word(b->rom, b->base + meta + (old ? 8 : 4));
    tbl = word(b->rom, b->base + meta + (old ? 12 : 8));
    if (old) {
        if (first < tbl) fail("S2 samples overlap sequence data");
        length = first - tbl;
        for (i = 0; i < count; ++i)
            if (word(b->rom, b->base + meta + i * 16 + 4) != ctl ||
                word(b->rom, b->base + meta + i * 16 + 8) != size ||
                word(b->rom, b->base + meta + i * 16 + 12) != tbl)
                fail("multiple S2 bank sets are not supported");
    } else length = word(b->rom, b->base + meta + 12);
    b->ctl = relative(b, ctl, size); b->ctl_size = size;
    b->tbl = wave_table(b, tbl, length); b->tbl_size = length;
    raw_section(b, "bank.ctl", b->ctl, size); save(b->root, "waves.tbl", rom_at(b->rom,b->tbl,length), length);
    save(b->root, "sequences.csv", csv.p, csv.n); free(csv.p);
    b1(b);
}

static void sound_effects(Bank *b, int old)
{
    size_t count, effects, i, j, q, p, rate, id, w, len, off;
    unsigned char *seen;
    char line[256];
    Buf effect_csv;
    effects = old ? a16(rom_at(b->rom, b->base + 2, 2)) : word(b->rom, b->base + 4);
    if (effects > MAX_AUDIO_ITEMS) fail("too many sound effects");
    q = old ? 4 + effects * 8 : 64;
    relative(b, q, old ? 40 : 52);
    count = word(b->rom, b->base + q + 4);
    if (!count || count > MAX_AUDIO_ITEMS) fail("invalid sound sample count");
    b->ctl_size = word(b->rom, b->base + q + 12);
    b->ctl = wave_table(b, word(b->rom, b->base + q + 8), b->ctl_size);
    b->tbl_size = word(b->rom, b->base + q + (old ? 20 : 24));
    b->tbl = wave_table(b, word(b->rom, b->base + q + (old ? 16 : 20)), b->tbl_size);
    control(b, 0, count * 16);
    save(b->root, "bank.ctl", rom_at(b->rom,b->ctl,b->ctl_size), b->ctl_size);
    save(b->root, "waves.tbl", rom_at(b->rom,b->tbl,b->tbl_size), b->tbl_size);
    effect_csv.p = NULL; effect_csv.n = effect_csv.cap = 0;
    append(&b->references, "sound,sample,rate\n", 18);
    if (old) {
        append(&effect_csv, "effect,sound,rate,sample,flags\n", 31);
        seen = alloc(count); memset(seen, 0, count);
        for (i = 0; i < effects; ++i) {
            p = b->base + 4 + i * 8;
            /* MP1 func_8000DF98 masks sample IDs with 0x1FFF;
             * the upper three bits are effect flags. */
            j = a16(rom_at(b->rom, p + 4, 2)) & 0x1fffU;
            rate = a16(rom_at(b->rom, p + 6, 2));
            if (j >= count) fail("T3 effect references nonexistent sample");
            w = (size_t)be32(control(b, j * 16 + 8, 4));
            id = sample(b, w, rate, "effect_table"); seen[j] = 1;
            sprintf(line, "%04lX,%04lX,%lu,%04lX,%08lX\n", (unsigned long)i, (unsigned long)j,
                    (unsigned long)rate, (unsigned long)id, (unsigned long)word(b->rom, p));
            append(&effect_csv, line, strlen(line));
            sprintf(line, "%04lX,%04lX,%lu\n", (unsigned long)j, (unsigned long)id, (unsigned long)rate);
            append(&b->references, line, strlen(line));
        }
        for (j = 0; j < count; ++j) if (!seen[j]) {
            w = (size_t)be32(control(b, j * 16 + 8, 4));
            id = sample(b, w, 32000, "unreferenced_default");
            sprintf(line, "%04lX,%04lX,32000\n", (unsigned long)j, (unsigned long)id);
            append(&b->references, line, strlen(line));
        }
        free(seen);
        off = word(b->rom, b->base + q + 24); len = word(b->rom, b->base + q + 28);
        raw_section(b, "effects.bin", relative(b, off, len), len);
    } else {
        for (j = 0; j < count; ++j) {
            rate = (size_t)be32(control(b, j * 16 + 4, 4));
            w = (size_t)be32(control(b, j * 16 + 8, 4));
            id = sample(b, w, rate, "sample_descriptor");
            sprintf(line, "%04lX,%04lX,%lu\n", (unsigned long)j, (unsigned long)id, (unsigned long)rate);
            append(&b->references, line, strlen(line));
        }
        off = word(b->rom, b->base + q + 32); len = word(b->rom, b->base + q + 36);
        p = relative(b, off, len); bounds(len, 0, effects * 4);
        raw_section(b, "effects.bin", p, len);
        append(&effect_csv, "effect,definition_rom_offset\n", 29);
        for (i = 0; i < effects; ++i) {
            off = word(b->rom, p + i * 4);
            if (!off) continue;
            bounds(len, off, 1);
            sprintf(line, "%04lX,%08lX\n", (unsigned long)i, (unsigned long)(p + off));
            append(&effect_csv, line, strlen(line));
        }
    }
    save(b->root, "effects.csv", effect_csv.p, effect_csv.n); free(effect_csv.p);
}

void dump_sounds(const Buf *rom, int game, const char *root)
{
    static const size_t mp1[] = {0x15396a0UL, 0x1778bc0UL, 0x1832ae0UL, 0x1bb8460UL, 0x1cecc60UL, 0x1ced490UL};
    static const size_t mp2[] = {0x1750450UL, 0x190a090UL, 0x1cbf410UL, 0x1e2a560UL, 0x1e2af98UL};
    static const size_t mp3[] = {0x1881c40UL, 0x1a56870UL, 0x1efd040UL, 0x1efda78UL};
    const size_t *positions;
    size_t n, i, total;
    int is_music;
    Bank b;
    char name[48], path[AUDIO_PATH], line[256];
    Buf listing;
    const char *signature;
    if (game < 1 || game > 3) fail("unsupported audio game");
    if (rom_at(rom, 0x3e, 2)[0] != 'E' || rom->p[0x3f])
        fail("sound profiles support US revision 0 ROMs only");
    positions = game == 1 ? mp1 : game == 2 ? mp2 : mp3;
    n = game == 1 ? 5 : game == 2 ? 4 : 3;
    /* Validate all container signatures before creating any audio output. */
    for (i = 0; i < n; ++i) {
        signature = i == n - 1 ? "FXD0" : game == 1 ? (i < 2 ? "S2" : "T3") : (i == 0 ? "MBF0" : "SBF0");
        if (positions[i] >= positions[i + 1] || positions[i + 1] > rom->n ||
            memcmp(rom_at(rom, positions[i], strlen(signature)), signature, strlen(signature)))
            fail("audio container does not match the US revision 0 profile");
    }
    if (packing_rom && packing_binary == 1) {
        for (i = 0; i < n; ++i) {
            Buf input;
            if (i == n-1) strcpy(name, "reverb.fxd");
            else {
                is_music = game == 1 ? i < 2 : i == 0;
                sprintf(name, "%s_%lu/native.bin", is_music ? "music" : "sfx",
                    (unsigned long)(is_music ? i : i-(game == 1 ? 2 : 1)));
            }
            join(path, root, name); input = read_file(path);
            if (input.n != positions[i+1]-positions[i]) fail("native.bin must retain its exported container size");
            memcpy(packing_rom->p+positions[i], input.p, input.n); free(input.p);
        }
        return;
    }
    directory(root); total = 0;
    listing.p = NULL; listing.n = listing.cap = 0;
    append(&listing, "container,rom_offset,bytes,samples\n", 35);
    for (i = 0; i < n; ++i) {
        memset(&b, 0, sizeof(b)); b.rom = rom;
        b.base = positions[i]; b.end = positions[i + 1];
        if (i == n - 1) {
            save(root, "reverb.fxd", rom_at(rom, b.base, b.end - b.base), b.end - b.base);
            sprintf(line, "reverb.fxd,%08lX,%lu,0\n", (unsigned long)b.base, (unsigned long)(b.end - b.base));
            append(&listing, line, strlen(line)); continue;
        }
        is_music = game == 1 ? i < 2 : i == 0;
        sprintf(name, "%s_%lu", is_music ? "music" : "sfx", (unsigned long)(is_music ? i : i - (game == 1 ? 2 : 1)));
        join(b.root, root, name); directory(b.root);
        join(path, b.root, "samples"); directory(path);
        raw_section(&b, "native.bin", b.base, b.end - b.base);
        b.samples = alloc(MAX_AUDIO_ITEMS * sizeof(Sample));
        {
            const char *heading;
            heading = "sample,wavetable_rom_offset,data_rom_offset,encoded_bytes,codec,rate,rate_source,frames,loop_start,loop_end,loop_count\n";
            append(&b.listing, heading, strlen(heading));
        }
        if (is_music) music(&b, game == 1); else sound_effects(&b, game == 1);
        save(b.root, "samples.csv", b.listing.p, b.listing.n);
        save(b.root, "references.csv", b.references.p, b.references.n);
        sprintf(line, "%s,%08lX,%lu,%lu\n", name, (unsigned long)b.base,
                (unsigned long)(b.end - b.base), (unsigned long)b.count);
        append(&listing, line, strlen(line));
        printf("Audio %s: %lu decoded WAV samples\n", name, (unsigned long)b.count);
        total += b.count; free(b.samples); free(b.listing.p); free(b.references.p);
    }
    save(root, "containers.csv", listing.p, listing.n); free(listing.p);
    if (!packing_rom) printf("Dumped native audio and %lu PCM WAV samples to %s\n", (unsigned long)total, root);
}

void pack_sounds(Buf *rom, int game, const char *root, int binary)
{
    Buf original;
    original.n = original.cap = rom->n; original.p = alloc(rom->n);
    memcpy(original.p, rom->p, rom->n);
    packing_rom = rom; packing_binary = binary;
    dump_sounds(&original, game, root);
    if (binary) {
        /* Parse and decode replacement banks before the ROM can be written. */
        /* Disable copying, keep export writes suppressed. */
        packing_binary = 2;
        dump_sounds(rom, game, root);
    }
    packing_rom = NULL; packing_binary = 0; free(original.p);
}

/* Character injection currently accepts direct T3 samples and simple SBF
 * effects starting with the sample-select command 0x93. Complex programs
 * need an explicit resolver rather than interpreting arbitrary bytes. */
void inject_sound(Buf *rom, int game, size_t bank, size_t effect,
                  const char *root, const char *path)
{
    static const size_t starts[3][3] = {
        {0x1832ae0UL,0x1bb8460UL,0x1cecc60UL},
        {0x190a090UL,0x1cbf410UL,0x1e2a560UL},
        {0x1a56870UL,0x1efd040UL,0x1efd040UL}
    };
    Bank b;
    Buf original, pcm;
    size_t q, count, effects, table, len, def, script, snd, other, i;
    size_t wave, rate, offset, length, book_size, lp, bp;
    const unsigned char *w, *book;
    unsigned int type;
    if (game < 1 || game > 3 || bank >= (game == 3 ? 1U : 2U)) fail("invalid character sound bank");
    memset(&b, 0, sizeof(b)); b.rom = rom;
    b.base = starts[game-1][bank]; b.end = starts[game-1][bank+1];
    rom_at(rom, b.base, b.end-b.base);
    if (memcmp(rom_at(rom,b.base,4), game == 1 ? "T3" : "SBF0", game == 1 ? 2 : 4))
        fail("character sound bank signature mismatch");
    effects = game == 1 ? a16(rom_at(rom,b.base+2,2)) : word(rom,b.base+4);
    if (effect >= effects) fail("character sound effect outside bank");
    q = game == 1 ? 4+effects*8 : 64; relative(&b,q,52);
    count = word(rom,b.base+q+4);
    if (!count || count > MAX_AUDIO_ITEMS) fail("invalid character sound sample count");
    b.ctl_size = word(rom,b.base+q+12);
    b.ctl = wave_table(&b,word(rom,b.base+q+8),b.ctl_size);
    b.tbl_size = word(rom,b.base+q+(game == 1 ? 20 : 24));
    b.tbl = wave_table(&b,word(rom,b.base+q+(game == 1 ? 16 : 20)),b.tbl_size);
    rate = 0;
    if (game == 1) {
        snd = a16(rom_at(rom,b.base+4+effect*8+4,4)) & 0x1fffU;
        rate = a16(rom_at(rom,b.base+4+effect*8+6,2));
        for (i=0;i<effects;++i) if (i != effect &&
            (a16(rom_at(rom,b.base+4+i*8+4,2)) & 0x1fffU) == snd)
            fail("character sound is shared by other effects");
    } else {
        len = word(rom,b.base+q+36);
        table = relative(&b,word(rom,b.base+q+32),len);
        bounds(len,0,effects*4);
        def = word(rom,table+effect*4); bounds(len,def,16);
        if (!def || rom->p[table+def] != 1) fail("character sound requires a single-layer effect");
        script = word(rom,table+def+12); bounds(len,script,3);
        if (rom->p[table+script] != 0x93) fail("unsupported character sound program");
        snd = a16(rom->p+table+script+1);
        /* Conservative raw-program scan also catches later sample switches.
         * False positives reject an import rather than changing shared audio. */
        for (i=0;i+2<len;++i) if (i != script && rom->p[table+i] == 0x93 &&
            a16(rom->p+table+i+1) == snd) fail("character sample has another possible program reference");
        /* Reject every other initial sample selection of this sample. */
        for (i=0;i<effects;++i) if (i != effect) {
            size_t layer, layers;
            def = word(rom,table+i*4); if (!def) continue;
            bounds(len,def,8); layers = rom->p[table+def];
            bounds(len,def,8+layers*8);
            for (layer=0;layer<layers;++layer) {
                script = word(rom,table+def+12+layer*8); bounds(len,script,3);
                other = a16(rom->p+table+script+1);
                if (rom->p[table+script] == 0x93 && other == snd)
                    fail("character sound is shared by other effects");
            }
        }
    }
    if (snd >= count) fail("character sound sample outside bank");
    w = control(&b,snd*16,16);
    if (game != 1) rate = (size_t)be32(w+4);
    wave = (size_t)be32(w+8); w = control(&b,wave,20);
    offset = (size_t)be32(w); length = (size_t)be32(w+4); type = w[8];
    bounds(b.tbl_size,offset,length);
    if (rate < 1000 || rate > 192000) fail("invalid character sample rate");
    for (i=0;i<count;++i) if (i != snd) {
        size_t other_wave, other_offset, other_length;
        other_wave=(size_t)be32(control(&b,i*16+8,4));
        w=control(&b,other_wave,20); other_offset=(size_t)be32(w); other_length=(size_t)be32(w+4);
        bounds(b.tbl_size,other_offset,other_length);
        if (other_wave == wave || (offset < other_offset+other_length && other_offset < offset+length))
            fail("character sample data is shared by another descriptor");
    }
    w=control(&b,wave,20);
    book = NULL; book_size = 0;
    if (type == 0) {
        bp = (size_t)be32(w+16); book = control(&b,bp,8);
        if (be32(book) != 2 || !be32(book+4) || be32(book+4)>16) fail("invalid character ADPCM book");
        book_size = 8+(size_t)be32(book+4)*32; book=control(&b,bp,book_size);
    }
    lp = (size_t)be32(w+12); if (lp) control(&b,lp,type == 0 ? 44 : 12);
    pcm = audio_pcm(rom_at(rom,b.tbl+offset,length),length,type,book,book_size);
    if (strlen(root) >= AUDIO_PATH) fail("character path too long");
    strcpy(b.root,root);
    original.n=original.cap=rom->n; original.p=alloc(rom->n); memcpy(original.p,rom->p,rom->n);
    b.rom=&original; packing_rom=rom;
    import_wav(&b,path,&pcm,b.tbl+offset,length,rate,type,book,book_size,lp);
    packing_rom=NULL; free(original.p); free(pcm.p);
}

/* MP1 character voice imports use the stable exported sample numbering, not
 * T3 effect numbering. PSND preserves encoded samples and predictor books,
 * avoiding another lossy encode when moving a voice between games. */
static void mp2_script_voice_rate(Buf *rom, const Bank *bank,
    size_t descriptor, size_t old_rate, size_t new_rate)
{
    size_t i, table, length, p, delay, rate;
    const PsMp2VoiceRate *entry;
    const unsigned char *script;
    length=word(bank->rom,bank->base+64+36);
    table=relative(bank,word(bank->rom,bank->base+64+32),length);
    for (i=0;i<PS_MP2_VOICE_RATE_COUNT;++i) {
        entry=&ps_mp2_voice_rates[i];
        if (entry->descriptor!=descriptor) continue;
        delay=entry->delayed; bounds(length,entry->offset,6+delay);
        p=table+entry->offset; script=rom_at(bank->rom,p,6+delay);
        if (script[0]!=(delay ? 0x13 : 0x93) ||
            (delay && script[1]!=0x0a) || a16(script+1+delay)!=descriptor ||
            script[3+delay]!=0xb5)
            fail("MP2 voice rate override differs from supported script");
        rate=(size_t)((double)a16(script+4+delay)*(double)new_rate/(double)old_rate+0.5);
        if (rate<1000 || rate>65535) fail("MP2 script voice rate outside supported range");
        rom->p[p+4+delay]=(unsigned char)(rate>>8);
        rom->p[p+5+delay]=(unsigned char)rate;
    }
}

void inject_voice_samples(Buf *rom, int game, const PsVoiceImport *requests, size_t request_count)
{
    Bank b;
    Buf source, table, extra_envelopes, *files, *decoded;
    Sample *samples;
    size_t effects, count, q, i, j, k, n, wave, rate, found, id;
    size_t *waves, *offsets, *lengths, *new_offsets, wave_count;
    size_t *replacement, *rates, *base_rates, *headers;
    unsigned char *seen;
    const unsigned char *w, *book;
    size_t length, book_size, old_book, old_size;
    if (!request_count) return;
    extra_envelopes.p=NULL; extra_envelopes.n=extra_envelopes.cap=0;
    if (game != 1 && game != 2) fail("native exported-sample imports support MP1/MP2 voices");
    memset(&b,0,sizeof(b)); b.base=game==1 ? 0x1832ae0UL : 0x190a090UL; b.end=game==1 ? 0x1bb8460UL : 0x1cbf410UL;
    source.n=source.cap=rom->n; source.p=alloc(rom->n); memcpy(source.p,rom->p,rom->n); b.rom=&source;
    if (memcmp(rom_at(&source,b.base,b.end-b.base),game==1 ? "T3" : "SBF0",game==1 ? 2 : 4)) fail("voice bank signature mismatch");
    effects=game==1 ? a16(source.p+b.base+2) : word(&source,b.base+4);
    if (effects>MAX_AUDIO_ITEMS) fail("invalid voice effect count");
    q=game==1 ? b.base+4+effects*8 : b.base+64;
    count=word(&source,q+4);
    if (!count || count>MAX_AUDIO_ITEMS) fail("invalid MP1 voice sample count");
    b.ctl_size=word(&source,q+12); b.ctl=wave_table(&b,word(&source,q+8),b.ctl_size);
    b.tbl_size=word(&source,q+(game==1 ? 20 : 24)); b.tbl=wave_table(&b,word(&source,q+(game==1 ? 16 : 20)),b.tbl_size);
    control(&b,0,count*16);
    samples=alloc(MAX_AUDIO_ITEMS*sizeof(*samples)); seen=alloc(count); memset(seen,0,count); n=0;
    /* Same first-reference ordering and (wave,rate) deduplication as dump. */
    for (i=0;i<(game==1 ? effects+count : count);++i) {
        if (game==2) { id=i; rate=(size_t)be32(control(&b,id*16+4,4)); }
        else if (i<effects) {
            id=a16(source.p+b.base+4+i*8+4)&0x1fffU;
            rate=a16(source.p+b.base+4+i*8+6);
            if (id>=count) fail("invalid MP1 voice reference");
            seen[id]=1;
        } else { id=i-effects; if (seen[id]) continue; rate=32000; }
        wave=(size_t)be32(control(&b,id*16+8,4));
        for (j=0;j<n;++j) if (samples[j].wave==wave && samples[j].rate==rate) break;
        if (j==n) {
            if (n==MAX_AUDIO_ITEMS) fail("too many MP1 voice variants");
            samples[n].wave=wave; samples[n].rate=rate; ++n;
        }
    }
    waves=alloc(count*sizeof(size_t)); offsets=alloc(count*sizeof(size_t)); lengths=alloc(count*sizeof(size_t));
    new_offsets=alloc(count*sizeof(size_t)); replacement=alloc(count*sizeof(size_t)); rates=alloc(count*sizeof(size_t)); base_rates=alloc(count*sizeof(size_t));
    wave_count=0;
    for (i=0;i<count;++i) {
        wave=(size_t)be32(control(&b,i*16+8,4));
        for (j=0;j<wave_count;++j) if (waves[j]==wave) break;
        if (j<wave_count) continue;
        w=control(&b,wave,20); waves[j]=wave; offsets[j]=(size_t)be32(w); lengths[j]=(size_t)be32(w+4);
        bounds(b.tbl_size,offsets[j],lengths[j]); replacement[j]=request_count; rates[j]=0; base_rates[j]=0; ++wave_count;
    }
    files=alloc(request_count*sizeof(Buf)); decoded=alloc(request_count*sizeof(Buf)); headers=alloc(request_count*sizeof(size_t));
    for (i=0;i<request_count;++i) {
        if (requests[i].bank!=0 || requests[i].sample>=n) fail("unknown MP1 exported voice sample");
        wave=samples[requests[i].sample].wave;
        for (j=0;j<wave_count && waves[j]!=wave;++j) {}
        if (j==wave_count || replacement[j]!=request_count) fail("duplicate voice destination");
        base_rates[j]=samples[requests[i].sample].rate;
        if (!base_rates[j]) fail("invalid original voice rate");
        files[i]=read_file(requests[i].path); bounds(files[i].n,0,24);
        w=files[i].p;
        if (memcmp(w,"PSND",4) || (be32(w+4)!=1 && be32(w+4)!=2) || be32(w+12)!=0) fail("expected PSND v1/v2 ADPCM voice");
        headers[i]=be32(w+4)==2 ? 40 : 24;
        rate=(size_t)be32(w+8); length=(size_t)be32(w+16); book_size=(size_t)be32(w+20);
        if (rate<1000 || rate>65535 || !length) fail("invalid voice rate or length");
        bounds(files[i].n,headers[i],book_size); bounds(files[i].n,headers[i]+book_size,length);
        if (files[i].n!=headers[i]+book_size+length) fail("invalid PSND size");
        decoded[i]=audio_pcm(w+headers[i]+book_size,length,0,w+headers[i],book_size);
        if (!decoded[i].n) fail("empty voice");
        w=control(&b,wave,20);
        if (w[8] || w[9]) fail("target voice is not unpatched ADPCM");
        if (be32(w+12)) control(&b,(size_t)be32(w+12),44); /* Replacement is deliberately nonlooping. */
        old_book=(size_t)be32(w+16); book=control(&b,old_book,8);
        if (be32(book)!=2 || !be32(book+4) || be32(book+4)>16) fail("invalid destination voice book");
        old_size=8+(size_t)be32(book+4)*32; control(&b,old_book,old_size);
        if (book_size!=old_size) fail("voice predictor book must match destination book allocation");
        for (k=0;k<wave_count;++k) if (k!=j) {
            const unsigned char *other;
            size_t ob, os;
            other=control(&b,waves[k],20);
            if (!other[8]) {
                ob=(size_t)be32(other+16); book=control(&b,ob,8);
                if (be32(book+4)>16) fail("invalid shared voice book");
                os=8+(size_t)be32(book+4)*32; control(&b,ob,os);
                if (old_book<ob+os && ob<old_book+old_size) fail("voice book is shared by another sample");
            }
        }
        if (headers[i]==40) {
            const unsigned char *e;
            e=files[i].p+24;
            if (be32(e)>0x7fffffffUL || be32(e+4)>0x7fffffffUL || be32(e+8)>0x7fffffffUL || e[12]>127 || e[13]>127)
                fail("invalid PSND voice envelope");
        }
        replacement[j]=i; rates[j]=rate;
        memcpy(rom->p+b.ctl+old_book,files[i].p+headers[i],book_size);
        put32(rom->p+b.ctl+wave+4,(U32)length);
        put32(rom->p+b.ctl+wave+12,0);
    }
    /* Deduplicate desired envelopes into the existing envelope allocations.
     * Add allocations only when existing slots cannot hold distinct values. */
    {
        size_t *slots, slot_count, used, descriptor, env, slot;
        Buf values;
        const unsigned char *e;
        slots=alloc(count*sizeof(size_t)); slot_count=0; used=0;
        values.p=NULL; values.n=values.cap=0;
        for (descriptor=0;descriptor<count;++descriptor) {
            env=(size_t)be32(control(&b,descriptor*16,4)); control(&b,env,16);
            for (slot=0;slot<slot_count && slots[slot]!=env;++slot) {}
            if (slot==slot_count) {
                for (k=0;k<slot_count;++k)
                    if (env<slots[k]+16 && slots[k]<env+16) fail("overlapping voice envelopes");
                slots[slot_count++]=env;
            }
        }
        for (descriptor=0;descriptor<count;++descriptor) {
            env=(size_t)be32(control(&b,descriptor*16,4)); e=control(&b,env,16);
            wave=(size_t)be32(control(&b,descriptor*16+8,4));
            for (j=0;j<wave_count;++j) if (waves[j]==wave) {
                i=replacement[j];
                if (i<request_count && headers[i]==40) e=files[i].p+24;
                break;
            }
            for (slot=0;slot<used;++slot) if (!memcmp(values.p+slot*16,e,16)) break;
            if (slot==used) {
                if (used==slot_count) {
                    slots[slot_count++]=b.ctl_size+extra_envelopes.n;
                    append(&extra_envelopes,e,16);
                }
                append(&values,e,16); ++used;
            }
            put32(rom->p+b.ctl+descriptor*16,(U32)slots[slot]);
        }
        for (slot=0;slot<used;++slot) if (slots[slot]<b.ctl_size)
            memcpy(rom->p+b.ctl+slots[slot],values.p+slot*16,16);
        free(values.p); free(slots);
    }
    table.p=NULL; table.n=table.cap=0;
    for (j=0;j<wave_count;++j) {
        found=wave_count;
        /* Preserve shared original data without duplicating it. */
        if (replacement[j]==request_count) for (k=0;k<j;++k)
            if (replacement[k]==request_count && offsets[k]==offsets[j] && lengths[k]==lengths[j]) {found=k;break;}
        if (found!=wave_count) new_offsets[j]=new_offsets[found];
        else {
            /* Samples in SDK banks are aligned for DMA. */
            while (table.n&7) byte(&table,0);
            new_offsets[j]=table.n;
            if (replacement[j]==request_count) append(&table,source.p+b.tbl+offsets[j],lengths[j]);
            else {
                i=replacement[j]; book_size=(size_t)be32(files[i].p+20); length=(size_t)be32(files[i].p+16);
                append(&table,files[i].p+headers[i]+book_size,length);
            }
        }
        put32(rom->p+b.ctl+waves[j],(U32)new_offsets[j]);
    }
    if (table.n>b.tbl_size) {
        while (rom->n&15) byte(rom,0);
        b.tbl=rom->n; b.tbl_size=table.n;
        append(rom,table.p,table.n);
        while (rom->n&15) byte(rom,0);
        put32(rom->p+q+(game==1 ? 16 : 20),(U32)(b.tbl-b.base));
        put32(rom->p+q+(game==1 ? 20 : 24),(U32)b.tbl_size);
        printf("Relocated voice wave table to 0x%08lX\n",(unsigned long)b.tbl);
    } else {
        memcpy(rom->p+b.tbl,table.p,table.n);
        memset(rom->p+b.tbl+table.n,0,b.tbl_size-table.n);
    }
    for (i=0;i<(game==1 ? effects : count);++i) {
        id=game==1 ? a16(source.p+b.base+4+i*8+4)&0x1fffU : i; wave=(size_t)be32(control(&b,id*16+8,4));
        for (j=0;j<wave_count;++j) if (waves[j]==wave && rates[j]) {
            size_t old_rate, new_rate;
            old_rate=game==1 ? a16(source.p+b.base+4+i*8+6) : (size_t)be32(control(&b,i*16+4,4));
            new_rate=(size_t)((double)old_rate*(double)rates[j]/(double)base_rates[j]+0.5);
            if (new_rate<1000 || new_rate>65535) fail("transplanted voice rate variant outside supported range");
            if (game==1) {
                rom->p[b.base+4+i*8+6]=(unsigned char)(new_rate>>8);
                rom->p[b.base+4+i*8+7]=(unsigned char)new_rate;
            } else {
                mp2_script_voice_rate(rom,&b,i,old_rate,new_rate);
                put32(rom->p+b.ctl+i*16+4,(U32)new_rate);
            }
            break;
        }
    }
    if (extra_envelopes.n) {
        Buf expanded;
        size_t new_ctl;
        expanded.p=NULL; expanded.n=expanded.cap=0;
        append(&expanded,rom->p+b.ctl,b.ctl_size);
        append(&expanded,extra_envelopes.p,extra_envelopes.n);
        while (rom->n&15) byte(rom,0);
        new_ctl=rom->n; append(rom,expanded.p,expanded.n);
        put32(rom->p+q+8,(U32)(new_ctl-b.base));
        put32(rom->p+q+12,(U32)expanded.n);
        printf("Relocated voice control bank; added %lu envelope bytes\n",(unsigned long)extra_envelopes.n);
        free(expanded.p);
    }
    free(extra_envelopes.p);
    printf("Imported %lu native voices; sample table %lu / %lu bytes\n",(unsigned long)request_count,(unsigned long)table.n,(unsigned long)b.tbl_size);
    for (i=0;i<request_count;++i) { free(files[i].p); free(decoded[i].p); }
    free(files); free(decoded); free(headers); free(table.p); free(source.p); free(samples); free(seen);
    free(waves); free(offsets); free(lengths); free(new_offsets); free(replacement); free(rates); free(base_rates);
}

/* Bound mainfs away from relocated bank-0 storage and report occupied ROM. */
size_t voice_rom_extent(const Buf *rom, int game, size_t archive_base, size_t *archive_end)
{
    size_t base, q, ctl, tbl, cs, ts, extent;
    if (game!=1 && game!=2) return 0;
    base=game==1 ? 0x1832ae0UL : 0x190a090UL;
    if (rom->n<base+64) return 0;
    if (memcmp(rom_at(rom,base,4),game==1 ? "T3" : "SBF0",game==1 ? 2 : 4)) return 0;
    q=game==1 ? base+4+a16(rom_at(rom,base+2,2))*8 : base+64;
    ctl=base+word(rom,q+8); cs=word(rom,q+12);
    tbl=base+word(rom,q+(game==1 ? 16 : 20)); ts=word(rom,q+(game==1 ? 20 : 24));
    rom_at(rom,ctl,cs); rom_at(rom,tbl,ts);
    if (ctl>=archive_base && ctl<*archive_end) *archive_end=ctl;
    if (tbl>=archive_base && tbl<*archive_end) *archive_end=tbl;
    extent=ctl+cs; if (tbl+ts>extent) extent=tbl+ts;
    return extent;
}

void ps_audio_reset(void)
{
    packing_rom=NULL; packing_binary=0;
}
