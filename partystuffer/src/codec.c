#include "ps.h"

static unsigned int take(const unsigned char *p, size_t n, size_t *i)
{
    if (*i >= n) fail("truncated compressed file");
    return p[(*i)++];
}

Buf decode(const unsigned char *p, size_t n, size_t size, unsigned int type, size_t *used)
{
    Buf out;
    unsigned char window[1024];
    size_t i, pos, count, dist, k;
    unsigned int wp, a, b, flags, bits, value;
    U32 code;
    if (type > 5) fail("unsupported compression type");
    out.n = out.cap = size; out.p = alloc(size);
    i = pos = 0; wp = 958; flags = bits = 0; code = 0;
    memset(window, 0, sizeof(window));
    if (type == 0) {
        if (size > n) fail("truncated raw file");
        if (size) memcpy(out.p, p, size);
        *used = size; return out;
    }
    if (type >= 2 && type <= 4) {
        if (n < 4) fail("truncated slide header");
        i = 4;
    }
    while (pos < size) {
        if (type == 1) {
            flags >>= 1;
            if (!(flags & 256)) flags = take(p, n, &i) | 0xff00;
            if (flags & 1) {
                value = take(p, n, &i);
                out.p[pos++] = window[wp] = (unsigned char)value;
                wp = (wp + 1) & 1023;
            } else {
                a = take(p, n, &i); b = take(p, n, &i);
                dist = a | ((b & 192) << 2); count = (b & 63) + 3;
                if (count > size - pos) fail("LZSS run exceeds decoded size");
                for (k = 0; k < count; ++k) {
                    value = window[(dist + k) & 1023];
                    out.p[pos++] = window[wp] = (unsigned char)value;
                    wp = (wp + 1) & 1023;
                }
            }
        } else if (type <= 4) {
            if (!bits) {
                if (n - i < 4) fail("truncated slide flags");
                code = be32(p + i); i += 4; bits = 32;
            }
            if (code & (U32)0x80000000UL) out.p[pos++] = (unsigned char)take(p, n, &i);
            else {
                a = take(p, n, &i); b = take(p, n, &i);
                dist = ((a & 15) << 8 | b) + 1;
                count = (a >> 4) ? (a >> 4) + 2 : take(p, n, &i) + 18;
                if (count > size - pos) fail("slide run exceeds decoded size");
                if (type != 2 && dist > pos) fail("invalid fast-slide distance");
                for (k = 0; k < count; ++k) {
                    value = dist > pos ? 0 : out.p[pos - dist];
                    out.p[pos++] = (unsigned char)value;
                }
            }
            code <<= 1; --bits;
        } else {
            a = take(p, n, &i); count = a & 127;
            if (!count || count > size - pos) fail("invalid RLE run length");
            if (a & 128) {
                for (k = 0; k < count; ++k) out.p[pos++] = (unsigned char)take(p, n, &i);
            } else {
                value = take(p, n, &i);
                memset(out.p + pos, (int)value, count); pos += count;
            }
        }
    }
    *used = i; return out;
}

/* A bounded hash chain makes greedy LZ matching fast without unbounded RAM.
 * Absolute positions distinguish old entries after ring-buffer wraparound. */
#define HASH_SIZE 4096
static unsigned int hash3(const unsigned char *p)
{
    return ((unsigned int)p[0] * 251U + (unsigned int)p[1] * 31U + p[2]) & (HASH_SIZE - 1);
}

Buf encode(const unsigned char *p, size_t n, unsigned int type)
{
    Buf out;
    long heads[HASH_SIZE], prev[4096], candidate;
    size_t pos, k, j, len, best, distance, bestdist, maxlen, window, flagpos, group, run, literal;
    unsigned int h, tokens, tokenlimit, probes, value, source;
    U32 flags;
    out.p = NULL; out.n = out.cap = 0;
    if (type > 5) fail("unsupported compression type");
    if (type == 0) { append(&out, p, n); return out; }
    if (type == 5) {
        pos = 0;
        while (pos < n) {
            run = 1;
            while (run < 127 && run < n - pos && p[pos + run] == p[pos]) ++run;
            if (run >= 3) { byte(&out, (unsigned int)run); byte(&out, p[pos]); pos += run; }
            else {
                literal = pos; pos += run;
                while (pos < n && pos - literal < 127) {
                    if (n - pos >= 3 && p[pos] == p[pos+1] && p[pos] == p[pos+2]) break;
                    ++pos;
                }
                byte(&out, 128U | (unsigned int)(pos - literal));
                append(&out, p + literal, pos - literal);
            }
        }
        return out;
    }
    for (k = 0; k < HASH_SIZE; ++k) heads[k] = -1;
    for (k = 0; k < 4096; ++k) prev[k] = -1;
    window = type == 1 ? 1024 : 4096;
    maxlen = type == 1 ? 66 : 273;
    tokenlimit = type == 1 ? 8 : 32;
    if (type != 1) { zeros(&out, 4); put32(out.p, (U32)n); }
    pos = 0;
    while (pos < n) {
        flagpos = out.n; zeros(&out, type == 1 ? 1 : 4);
        flags = 0;
        for (tokens = 0; tokens < tokenlimit && pos < n; ++tokens) {
            best = 0; bestdist = 0;
            len = n - pos < maxlen ? n - pos : maxlen;
            if (len >= 3) {
                h = hash3(p + pos); candidate = heads[h]; probes = 0;
                while (candidate >= 0 && (size_t)candidate < pos && probes++ < 256) {
                    distance = pos - (size_t)candidate;
                    if (distance > window) break;
                    for (j = 0; j < len && p[(size_t)candidate + j] == p[pos + j]; ++j) { }
                    if (j > best) { best = j; bestdist = distance; if (j == len) break; }
                    candidate = prev[(size_t)candidate & 4095];
                }
                /* LZSS starts with a zero-filled dictionary. */
                if (type == 1 && pos < 1024 && p[pos] == 0) {
                    for (j = 0; j < len && j < 1024 - pos && p[pos+j] == 0; ++j) { }
                    if (j > best) { best = j; bestdist = 1024; }
                }
            }
            if (best >= 3) {
                if (type == 1) {
                    source = (unsigned int)((958 + pos - bestdist) & 1023);
                    byte(&out, source & 255);
                    byte(&out, ((source >> 2) & 192) | (unsigned int)(best - 3));
                } else {
                    value = (unsigned int)(bestdist - 1);
                    if (best < 18) byte(&out, ((unsigned int)(best - 2) << 4) | (value >> 8));
                    else byte(&out, value >> 8);
                    byte(&out, value & 255);
                    if (best >= 18) byte(&out, (unsigned int)(best - 18));
                }
                group = best;
            } else {
                flags |= type == 1 ? (U32)1 << tokens : (U32)1 << (31 - tokens);
                byte(&out, p[pos]); group = 1;
            }
            for (k = 0; k < group; ++k) {
                if (n - pos >= 3) {
                    h = hash3(p + pos); prev[pos & 4095] = heads[h]; heads[h] = (long)pos;
                }
                ++pos;
            }
        }
        if (type == 1) out.p[flagpos] = (unsigned char)flags;
        else put32(out.p + flagpos, flags);
    }
    return out;
}
