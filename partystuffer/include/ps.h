#ifndef PS_H
#define PS_H
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <stddef.h>
#if CHAR_BIT != 8
#error Eight-bit bytes required
#endif
#if UINT_MAX == 0xffffffffUL
typedef unsigned int U32;
#elif ULONG_MAX == 0xffffffffUL
typedef unsigned long U32;
#else
#error A 32-bit unsigned integer type is required
#endif
#define LIMIT (64UL * 1024UL * 1024UL)
typedef struct { unsigned char *p; size_t n, cap; } Buf;
#define fail ps_internal_fail
#define alloc ps_internal_alloc
#define be32 ps_internal_be32
#define put32 ps_internal_put32
#define read_file ps_internal_read_file
#define write_new ps_internal_write_new
#define append ps_internal_append
#define byte ps_internal_byte
#define zeros ps_internal_zeros
#define align2 ps_internal_align2
#define crc32 ps_internal_crc32
#define rom_order ps_internal_rom_order
#define swap_order ps_internal_swap_order
#define cic_detect ps_internal_cic_detect
#define checksum ps_internal_checksum
#define decode ps_internal_decode
#define encode ps_internal_encode
#define inject_voice_samples ps_internal_inject_voice_samples
#define inject_character_name ps_internal_inject_character_name
#define inject_sound ps_internal_inject_sound
#define pack_sounds ps_internal_pack_sounds
#define dump_sounds ps_internal_dump_sounds
#define audio_pcm ps_internal_audio_pcm
#define voice_rom_extent ps_internal_voice_rom_extent
void fail(const char *message);
void *alloc(size_t n);
U32 be32(const unsigned char *p);
void put32(unsigned char *p, U32 x);
Buf read_file(const char *path);
void write_new(const char *path, const unsigned char *p, size_t n);
void append(Buf *b, const void *p, size_t n);
void byte(Buf *b, unsigned int x);
void zeros(Buf *b, size_t n);
void align2(Buf *b);
U32 crc32(const unsigned char *p, size_t n);
int rom_order(Buf *rom);
void swap_order(Buf *rom, int order);
int cic_detect(const Buf *rom);
void checksum(Buf *rom, int cic, U32 *a, U32 *b);
Buf decode(const unsigned char *p, size_t n, size_t size, unsigned int type, size_t *used);
Buf encode(const unsigned char *p, size_t n, unsigned int type);
typedef struct { size_t bank, sample; const char *path; } PsVoiceImport;
void inject_voice_samples(Buf *rom, int game, const PsVoiceImport *requests, size_t count);
void inject_character_name(Buf *rom, int game, size_t character, const char *name);
void inject_sound(Buf *rom, int game, size_t bank, size_t effect, const char *root, const char *path);
void pack_sounds(Buf *rom, int game, const char *root, int binary);
void dump_sounds(const Buf *rom, int game, const char *root);
Buf audio_pcm(const unsigned char *data, size_t length, unsigned int type,
              const unsigned char *book, size_t book_size);
size_t voice_rom_extent(const Buf *rom, int game, size_t archive_base, size_t *archive_end);
int ps_cli_main(int argc, char **argv);
Buf ps_build_character(const char *source, const char *folder, unsigned int character);
void ps_audio_reset(void);
void ps_raise(const char *message);
void *ps_malloc(size_t n);
void *ps_realloc(void *p, size_t n);
void ps_free(void *p);
FILE *ps_fopen(const char *path, const char *mode);
int ps_fclose(FILE *f);
int ps_printf(const char *format, ...);
int ps_fprintf(FILE *stream, const char *format, ...);
#ifndef PS_RUNTIME_IMPLEMENTATION
#define malloc ps_malloc
#define realloc ps_realloc
#define free ps_free
#define fopen ps_fopen
#define fclose ps_fclose
#define printf ps_printf
#define fprintf ps_fprintf
#endif
#endif
