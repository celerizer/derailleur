#include "ps.h"
#include "character_map.h"
#include <errno.h>
#ifdef _WIN32
#include <direct.h>
#define MKDIR(p) _mkdir(p)
#else
#include <sys/stat.h>
#define MKDIR(p) mkdir(p, 0777)
#endif

#define PATH_SIZE 4096
#define MAX_FILES 65536UL

typedef struct {
    size_t offset, span, used, size;
    unsigned int type;
} Entry;
typedef struct { size_t offset, count; Entry *files; } Directory;
typedef struct {
    size_t base, end, count, total;
    int game;
    Directory *dirs;
} Archive;

static void path_join(char *out, const char *root, const char *leaf)
{
    if (strlen(root) + strlen(leaf) + 2 > PATH_SIZE) fail("path too long");
    strcpy(out, root); strcat(out, "/"); strcat(out, leaf);
}

static void file_path(char *out, const char *root, size_t d, size_t f)
{
    char leaf[64];
    sprintf(leaf, "%04lX/%04lX.bin", (unsigned long)d, (unsigned long)f);
    path_join(out, root, leaf);
}

static void make_dir(const char *path)
{
    if (MKDIR(path)) {
        fprintf(stderr, "%s: %s\n", path, strerror(errno));
        fail("cannot create directory (dump destination must be new)");
    }
}

static unsigned long number(const char *s)
{
    char *end;
    unsigned long n;
    if (!*s || *s == '-' || *s == '+') fail("invalid numeric option");
    errno = 0; n = strtoul(s, &end, 0);
    if (errno || *end || n > LIMIT) fail("invalid or excessive numeric option");
    return n;
}

static int game_id(const Buf *rom)
{
    if (!memcmp(rom->p + 0x3b, "CLB", 3)) return 1;
    if (!memcmp(rom->p + 0x3b, "NMW", 3)) return 2;
    if (!memcmp(rom->p + 0x3b, "NMV", 3)) return 3;
    fail("ROM game ID is not Mario Party 1, 2 or 3"); return 0;
}

static size_t table_count(const Buf *rom, size_t base, size_t end)
{
    U32 count;
    size_t i, off, last;
    if (base > end || end > rom->n || end - base < 4) fail("archive table outside ROM");
    count = be32(rom->p + base);
    if (count > 65536UL || count > (end - base - 4) / 4) fail("invalid archive count");
    last = 4 + (size_t)count * 4;
    for (i = 0; i < count; ++i) {
        off = be32(rom->p + base + 4 + i * 4);
        if ((off & 1) || off < last || off >= end - base) fail("invalid or unordered archive offsets");
        if (i && off == last) fail("duplicate archive offsets are not supported");
        last = off;
    }
    return (size_t)count;
}

static Archive parse(const Buf *rom, size_t base, size_t end, int game)
{
    Archive a;
    Directory *dir;
    Entry *e;
    Buf data;
    size_t d, f, stop, next;
    a.base = base; a.end = end; a.game = game; a.total = 0;
    a.count = table_count(rom, base, end);
    if (!a.count) fail("empty root archive");
    a.dirs = alloc(a.count * sizeof(Directory));
    for (d = 0; d < a.count; ++d) {
        dir = &a.dirs[d];
        dir->offset = base + be32(rom->p + base + 4 + d * 4);
        stop = d + 1 < a.count ? base + be32(rom->p + base + 8 + d * 4) : end;
        dir->count = table_count(rom, dir->offset, stop);
        if (a.total + dir->count > MAX_FILES) fail("too many files");
        a.total += dir->count;
        dir->files = alloc(dir->count * sizeof(Entry));
        for (f = 0; f < dir->count; ++f) {
            e = &dir->files[f];
            e->offset = dir->offset + be32(rom->p + dir->offset + 4 + f * 4);
            next = f + 1 < dir->count ? dir->offset + be32(rom->p + dir->offset + 8 + f * 4) : stop;
            e->span = next - e->offset;
            if (e->span < 8) fail("truncated file header");
            e->size = be32(rom->p + e->offset);
            if (be32(rom->p + e->offset + 4) > (game == 1 ? 1U : 5U)) fail("invalid file compression type");
            e->type = (unsigned int)be32(rom->p + e->offset + 4);
            data = decode(rom->p + e->offset + 8, e->span - 8, e->size, e->type, &e->used);
            free(data.p); e->used += 8;
            if ((e->used + 1) / 2 * 2 > e->span) fail("missing file alignment padding");
        }
    }
    return a;
}

static void free_archive(Archive *a)
{
    size_t d;
    for (d = 0; d < a->count; ++d) free(a->dirs[d].files);
    free(a->dirs);
}

/* Bind a dump to its source ROM, ignoring the repairable CRC header fields. */
static U32 identity(Buf *rom)
{
    unsigned char saved[8];
    U32 c;
    memcpy(saved, rom->p + 16, 8); memset(rom->p + 16, 0, 8);
    c = crc32(rom->p, rom->n); memcpy(rom->p + 16, saved, 8);
    return c;
}

static void dump_archive(Buf *rom, const Archive *a, const char *root)
{
    char path[PATH_SIZE], leaf[64], line[256];
    Buf data, manifest, listing;
    const Entry *e;
    size_t d, f, used;
    manifest.p = listing.p = NULL; manifest.n = manifest.cap = listing.n = listing.cap = 0;
    make_dir(root);
    sprintf(line, "PARTYSTUFFER 1\nGAME %d\nBASE %08lX\nEND %08lX\nROMCRC %08lX\n",
            a->game, (unsigned long)a->base, (unsigned long)a->end, (unsigned long)identity(rom));
    append(&manifest, line, strlen(line));
    sprintf(line, "directory,file,rom_offset,decoded_size,compression,stored_bytes\n");
    append(&listing, line, strlen(line));
    for (d = 0; d < a->count; ++d) {
        sprintf(leaf, "%04lX", (unsigned long)d); path_join(path, root, leaf); make_dir(path);
        for (f = 0; f < a->dirs[d].count; ++f) {
            e = &a->dirs[d].files[f];
            data = decode(rom->p + e->offset + 8, e->span - 8, e->size, e->type, &used);
            file_path(path, root, d, f); write_new(path, data.p, data.n); free(data.p);
            sprintf(line, "%04lX,%04lX,%08lX,%lu,%u,%lu\n", (unsigned long)d, (unsigned long)f,
                    (unsigned long)e->offset, (unsigned long)e->size, e->type, (unsigned long)e->used);
            append(&listing, line, strlen(line));
        }
    }
    path_join(path, root, "files.csv"); write_new(path, listing.p, listing.n);
    path_join(path, root, "manifest.txt"); write_new(path, manifest.p, manifest.n);
    free(manifest.p); free(listing.p);
    printf("Dumped %lu directories, %lu files to %s\n", (unsigned long)a->count, (unsigned long)a->total, root);
}

static void check_manifest(Buf *rom, const Archive *a, const char *root)
{
    char path[PATH_SIZE], expected[256];
    Buf b;
    path_join(path, root, "manifest.txt"); b = read_file(path);
    sprintf(expected, "PARTYSTUFFER 1\nGAME %d\nBASE %08lX\nEND %08lX\nROMCRC %08lX\n",
            a->game, (unsigned long)a->base, (unsigned long)a->end, (unsigned long)identity(rom));
    if (b.n != strlen(expected) || memcmp(b.p, expected, b.n)) fail("manifest does not match base ROM and archive bounds");
    free(b.p);
}

/* These are the DataInit argument instructions in the US v1.0 executables.
 * Validate all three words before changing either address half. */
static size_t loader_address(const Buf *rom, int game, int patch, size_t newbase)
{
    static const size_t locations[3] = {0x3c014UL, 0x416e4UL, 0x3619cUL};
    static const U32 calls[3] = {0x0c005118UL, 0x0c005d4cUL, 0x0c0026b0UL};
    size_t p;
    U32 hi, lo, address;
    if (rom->p[0x3e] != 'E' || rom->p[0x3f] != 0)
        fail("relocation is supported only for US revision 0");
    p = locations[game - 1]; hi = be32(rom->p + p); lo = be32(rom->p + p + 8);
    if ((hi & (U32)0xffff0000UL) != (U32)0x3c040000UL ||
        be32(rom->p + p + 4) != calls[game - 1] ||
        (lo & (U32)0xffff0000UL) != (U32)0x24840000UL)
        fail("DataInit instructions differ from the supported executable");
    address = ((hi & 65535U) << 16) + (lo & 65535U);
    if (lo & 32768U) address -= 65536UL;
    if (patch) {
        put32(rom->p + p, (U32)0x3c040000UL | (U32)((newbase + 32768UL) >> 16));
        put32(rom->p + p + 8, (U32)0x24840000UL | (U32)(newbase & 65535U));
    }
    return (size_t)address;
}

static void pack_archive(Buf *rom, Archive *a, const char *root, int compact, int relocate, char **overrides, size_t target)
{
    Buf rebuilt, original, edit, packed, verified;
    unsigned char header[8];
    char path[PATH_SIZE];
    const Entry *e;
    size_t d, f, dirpos, used, changed, newbase, newsize;
    int fits, same;
    if (!overrides) check_manifest(rom, a, root);
    if (relocate && loader_address(rom, a->game, 0, 0) != a->base)
        fail("archive bounds do not match the game loader address");
    rebuilt.p = NULL; rebuilt.n = rebuilt.cap = 0;
    zeros(&rebuilt, 4 + 4 * a->count); put32(rebuilt.p, (U32)a->count);
    changed = 0; fits = !compact && !relocate;
    /* Stage all changes in a separate ROM buffer. No output exists until all
     * files have been read, encoded, and the archive capacity has been checked. */
    for (d = 0; d < a->count; ++d) {
        dirpos = rebuilt.n;
        put32(rebuilt.p + 4 + 4 * d, (U32)dirpos);
        zeros(&rebuilt, 4 + 4 * a->dirs[d].count);
        put32(rebuilt.p + dirpos, (U32)a->dirs[d].count);
        for (f = 0; f < a->dirs[d].count; ++f) {
            e = &a->dirs[d].files[f];
            if (overrides) {
                if (d == target && overrides[f]) edit = read_file(overrides[f]);
                else edit = decode(rom->p + e->offset + 8, e->span - 8, e->size, e->type, &used);
            } else { file_path(path, root, d, f); edit = read_file(path); }
            original = decode(rom->p + e->offset + 8, e->span - 8, e->size, e->type, &used);
            same = edit.n == original.n && (!edit.n || !memcmp(edit.p, original.p, edit.n));
            free(original.p);
            put32(rebuilt.p + dirpos + 4 + 4 * f, (U32)(rebuilt.n - dirpos));
            if (same) append(&rebuilt, rom->p + e->offset, e->used);
            else {
                ++changed;
                packed = encode(edit.p, edit.n, e->type);
                verified = decode(packed.p, packed.n, edit.n, e->type, &used);
                if (verified.n != edit.n || (edit.n && memcmp(verified.p, edit.p, edit.n))) fail("internal compression verification failed");
                free(verified.p);
                put32(header, (U32)edit.n); put32(header + 4, (U32)e->type);
                append(&rebuilt, header, 8); append(&rebuilt, packed.p, packed.n);
                if (8 + packed.n + (packed.n & 1) > e->span) fits = 0;
                else if (!relocate) {
                    if (e->offset+8+packed.n+(packed.n&1)>rom->n)
                        zeros(rom,e->offset+8+packed.n+(packed.n&1)-rom->n);
                    memcpy(rom->p + e->offset, header, 8);
                    if (packed.n) memcpy(rom->p + e->offset + 8, packed.p, packed.n);
                    if (packed.n & 1) rom->p[e->offset + 8 + packed.n] = 0;
                }
                free(packed.p);
            }
            free(edit.p); align2(&rebuilt);
        }
    }
    if (overrides && !fits && rebuilt.n > a->end-a->base) {
        if (loader_address(rom,a->game,0,0) != a->base) fail("cannot relocate nonstandard character archive");
        relocate=1;
    }
    if (relocate) {
        newbase = rom->n;
        if (rebuilt.n > LIMIT - newbase) fail("relocated ROM would exceed 64 MiB");
        newsize = 0x100000UL;
        while (newsize < newbase + rebuilt.n) newsize *= 2;
        append(rom, rebuilt.p, rebuilt.n);
        zeros(rom, newsize - rom->n);
        loader_address(rom, a->game, 1, newbase);
        a->base = newbase; a->end = newsize;
        printf("Relocated mainfs to 0x%08lX; expanded ROM to %lu bytes\n",
                (unsigned long)newbase, (unsigned long)newsize);
    } else if (!fits) {
        if (rebuilt.n > a->end - a->base) {
            fprintf(stderr, "Archive needs %lu bytes; available %lu (over by %lu).\n",
                    (unsigned long)rebuilt.n, (unsigned long)(a->end - a->base),
                    (unsigned long)(rebuilt.n - (a->end - a->base)));
            fail("repacked archive would overwrite neighboring ROM data");
        }
        {
            size_t old_end, d, f, used_end;
            if (a->base+rebuilt.n>rom->n) zeros(rom,a->base+rebuilt.n-rom->n);
            old_end=a->base;
            for (d=0;d<a->count;++d) for (f=0;f<a->dirs[d].count;++f) {
                Entry *old;
                old=&a->dirs[d].files[f];
                used_end=old->offset+((old->used+1)&~(size_t)1);
                if (used_end>old_end) old_end=used_end;
            }
            memcpy(rom->p + a->base, rebuilt.p, rebuilt.n);
            if (old_end>a->base+rebuilt.n)
                memset(rom->p+a->base+rebuilt.n,0,old_end-a->base-rebuilt.n);
        }
    }
    printf("Packed %lu files (%lu changed); %s; compact size %lu / %lu bytes\n",
            (unsigned long)a->total, (unsigned long)changed, fits ? "original offsets preserved" : "offset tables rebuilt",
            (unsigned long)rebuilt.n, (unsigned long)(a->end - a->base));
    free(rebuilt.p);
}

/* Package paths are relative and whitespace-free. Reject traversal so a
 * package's asset names resolve inside the selected character directory. */
static void package_path(char *out, const char *root, const char *leaf)
{
    const char *p;
    if (!*leaf || *leaf == '/' || strchr(leaf, '\\') || strchr(leaf, ':')) fail("invalid character asset path");
    for (p=leaf; *p; ++p)
        if ((p == leaf || p[-1] == '/') && p[0] == '.' && p[1] == '.' && (p[2] == '/' || !p[2]))
            fail("character asset path escapes package");
    path_join(out,root,leaf);
}

static void trim_character_padding(Buf *rom, Archive *a)
{
    {
        Archive packed;
        size_t archive_end, occupied, d, f, finish;
        archive_end=a->end; if (archive_end>rom->n) archive_end=rom->n;
        occupied=voice_rom_extent(rom,a->game,a->base,&archive_end);
        packed=parse(rom,a->base,archive_end,a->game);
        if (occupied<0x2000000UL) occupied=0x2000000UL;
        for (d=0;d<packed.count;++d) {
            finish=packed.dirs[d].offset+4+packed.dirs[d].count*4;
            if (finish>occupied) occupied=finish;
            for (f=0;f<packed.dirs[d].count;++f) {
                Entry *entry;
                entry=&packed.dirs[d].files[f];
                finish=(entry->offset+entry->used+15)&~(size_t)15;
                if (finish>occupied) occupied=finish;
            }
        }
        free_archive(&packed);
        /* Reuse only verified zero padding beyond all known relocated data. */
        for (finish=occupied;finish<rom->n && !rom->p[finish];++finish) {}
        if (finish==rom->n && occupied<rom->n) rom->n=occupied;
    }
}

static void inject_character(Buf *rom, Archive *a, const char *root, size_t character, int relocate)
{
    char manifest[PATH_SIZE], line[PATH_SIZE], game[16], key[256], leaf[2048], extra[2];
    char full[PATH_SIZE], display_name[8];
    FILE *fp;
    char **paths;
    size_t target, i, count, sound_count, file;
    signed index;
    int fields, matched, optional_sounds;
    unsigned char *seen;
    Buf data;
    PsVoiceImport *voices;
    size_t voice_count;
    if (character >= PS_CHARACTER_SLOT_COUNT || ps_character_slots[character].character_id != (signed)character)
        fail("unsupported target character ID");
    index = ps_character_slots[character].directory[a->game-1];
    if (index < 0 || (size_t)index >= a->count) fail("target character is absent in this game");
    target=(size_t)index;
    paths=alloc(a->dirs[target].count*sizeof(*paths)); memset(paths,0,a->dirs[target].count*sizeof(*paths));
    seen=alloc(PS_CHARACTER_SOUND_COUNT); memset(seen,0,PS_CHARACTER_SOUND_COUNT);
    path_join(manifest,root,"character.txt"); fp=fopen(manifest,"r");
    if (!fp) fail("cannot open character package character.txt");
    if (!fgets(line,sizeof(line),fp) || (strcmp(line,"PARTYSTUFFER_CHARACTER 1\n") && strcmp(line,"PARTYSTUFFER_CHARACTER 1\r\n")))
        fail("unsupported character package header");
    voices=alloc(PS_CHARACTER_SOUND_COUNT*sizeof(*voices)); voice_count=0;
    display_name[0]=0; optional_sounds=0;
    count=sound_count=0;
    while (fgets(line,sizeof(line),fp)) {
        char *p;
        if (!strchr(line,'\n') && !feof(fp)) fail("character manifest line too long");
        p=strchr(line,'#'); if (p) *p=0;
        p=line; while (*p==' ' || *p=='\t') ++p;
        if (!strncmp(p,"name",4) && (p[4]==' ' || p[4]=='\t')) {
            size_t n;
            if (display_name[0]) fail("duplicate character name");
            p+=4; while (*p==' ' || *p=='\t') ++p;
            n=strlen(p); while (n && (p[n-1]=='\n' || p[n-1]=='\r' || p[n-1]==' ' || p[n-1]=='\t')) --n;
            if (!n || n>=sizeof(display_name)) fail("manifest name must contain 1-7 letters/spaces");
            memcpy(display_name,p,n); display_name[n]=0; continue;
        }
        fields=sscanf(line,"%15s %255s %2047s %1s",game,key,leaf,extra);
        if (fields == EOF) continue;
        if (fields==2 && !strcmp(game,"voices") && !strcmp(key,"optional")) { optional_sounds=1; continue; }
        if (fields==3 && (!strcmp(game,"assets") || !strcmp(game,"assets-partial"))) {
            size_t m, added;
            int partial;
            FILE *probe;
            const PsCharacterFile *asset;
            signed slot;
            char asset_root[PATH_SIZE], asset_leaf[512];
            if (strcmp(key,"mp1") && strcmp(key,"mp2") && strcmp(key,"mp3")) fail("invalid asset-bank game");
            if (key[2]-'0' != a->game) continue;
            package_path(asset_root,root,leaf); added=0; partial=!strcmp(game,"assets-partial");
            for (m=0;m<PS_CHARACTER_FILE_COUNT;++m) {
                asset=&ps_character_files[m];
                slot=a->game==1 ? asset->mp1 : a->game==2 ? asset->mp2 : asset->mp3;
                if (slot<0) continue;
                file=(size_t)slot;
                sprintf(asset_leaf,"%s.bin",asset->name); package_path(full,asset_root,asset_leaf);
                if (partial) {
                    probe=fopen(full,"rb");
                    if (!probe) {
                        if (errno==ENOENT) continue;
                        fail("cannot open partial asset-bank entry");
                    }
                    if (fclose(probe)) fail("cannot close partial asset-bank entry");
                }
                if (file>=a->dirs[target].count || paths[file]) fail("duplicate or out-of-range asset-bank entry");
                data=read_file(full);
                if (asset->kind==PS_ASSET_ANIMATION && (data.n<32 || memcmp(data.p,"MTNX",4))) fail("asset bank contains invalid animation");
                if (asset->kind==PS_ASSET_MODEL && (data.n<16 || memcmp(data.p,"FORM",4) || memcmp(data.p+8,"HBINMODE",8))) fail("asset bank contains invalid model");
                free(data.p); paths[file]=alloc(strlen(full)+1); strcpy(paths[file],full); ++count; ++added;
            }
            if (!partial && added!=a->dirs[target].count) fail("asset bank must cover the complete target directory");
            printf("Selected %s game-local asset bank: %lu files\n",partial ? "partial" : "complete",(unsigned long)added);
            continue;
        }
        if (fields == 3 && !strcmp(game,"target")) {
            if (strcmp(key,"mp1") && strcmp(key,"mp2") && strcmp(key,"mp3")) fail("invalid package target game");
            if (key[2]-'0' == a->game && number(leaf) != character) fail("package rig is restricted to a different target character");
            continue;
        }
        if (fields != 3 || (strcmp(game,"mp1") && strcmp(game,"mp2") && strcmp(game,"mp3")))
            fail("expected: mp1|mp2|mp3 asset.name relative/path");
        if (game[2]-'0' != a->game) continue;
        package_path(full,root,leaf); matched=0;
        for (i=0;i<PS_CHARACTER_FILE_COUNT;++i) if (!strcmp(key,ps_character_files[i].name)) {
            matched=1;
            index=a->game == 1 ? ps_character_files[i].mp1 : a->game == 2 ? ps_character_files[i].mp2 : ps_character_files[i].mp3;
            if (index < 0) fail("character asset has no known mapping in target game");
            if (ps_character_files[i].inferred_mask & (1U << (a->game-1)))
                fail("character asset mapping is inferred; verify and update character_map.h first");
            file=(size_t)index;
            if (file >= a->dirs[target].count || paths[file]) fail("duplicate or out-of-range character asset");
            data=read_file(full);
            if (ps_character_files[i].kind == PS_ASSET_ANIMATION && (data.n < 32 || memcmp(data.p,"MTNX",4)))
                fail("character animation must be a decoded target-game MTNX file");
            if (ps_character_files[i].kind == PS_ASSET_MODEL && (data.n < 16 || memcmp(data.p,"FORM",4) || memcmp(data.p+8,"HBINMODE",8)))
                fail("character model must be a decoded target-game HBIN model");
            free(data.p); paths[file]=alloc(strlen(full)+1); strcpy(paths[file],full); ++count;
            printf("Character %s -> %04lX/%04lX.bin\n",key,(unsigned long)target,(unsigned long)file);
            break;
        }
        if (matched) continue;
        for (i=0;i<PS_CHARACTER_SOUND_COUNT;++i) if (!strcmp(key,ps_character_sounds[i].name)) {
            const PsCharacterSoundRef *ref;
            matched=1; ref=&ps_character_sounds[i].game[a->game-1];
            if (seen[i]) fail("duplicate character sound");
            seen[i]=1;
            if (ref->bank < 0 || character >= PS_SOUND_CHARACTER_COUNT || ref->id[character] < 0) {
                if (!optional_sounds) fail("character sound has no known mapping in target game");
                printf("No target slot for %s; skipped\n",key); break;
            }
            if (ps_character_sounds[i].id_kind == PS_SOUND_EXPORTED_SAMPLE) {
                char *copy;
                size_t previous;
                for (previous=0;previous<voice_count;++previous)
                    if (voices[previous].bank==(size_t)ref->bank && voices[previous].sample==(size_t)ref->id[character]) break;
                if (previous<voice_count && optional_sounds) {
                    printf("Shared target slot for %s; earlier manifest voice takes precedence\n",key); break;
                }
                copy=alloc(strlen(full)+1); strcpy(copy,full);
                voices[voice_count].bank=(size_t)ref->bank;
                voices[voice_count].sample=(size_t)ref->id[character];
                voices[voice_count++].path=copy;
            } else inject_sound(rom,a->game,(size_t)ref->bank,(size_t)ref->id[character],root,leaf);
            ++sound_count; printf("Character %s selected\n",key); break;
        }
        if (!matched) { fprintf(stderr,"Unknown character key: %s\n",key); fail("unknown character asset name"); }
    }
    if (ferror(fp)) fail("cannot read character manifest");
    fclose(fp); free(seen);
    if (!count && !sound_count && !display_name[0]) fail("character package has no entries for this game");
    if (display_name[0]) inject_character_name(rom,a->game,character,display_name);
    trim_character_padding(rom,a);
    if (count) pack_archive(rom,a,root,0,relocate,paths,target);
    trim_character_padding(rom,a);
    inject_voice_samples(rom,a->game,voices,voice_count);
    for (i=0;i<voice_count;++i) free((void *)voices[i].path);
    free(voices);
    for (i=0;i<a->dirs[target].count;++i) free(paths[i]);
    free(paths);
    printf("Injected %lu files and %lu sounds into %s; unspecified assets retained\n",
        (unsigned long)count,(unsigned long)sound_count,ps_character_slots[character].name);
}

static void usage(void)
{
    puts("partystuffer - Mario Party 1/2/3 N64 main filesystem tool\n"
         "Usage:\n"
         "  partystuffer inject-character ROM PACKAGE CHARACTER_ID NEW_ROM [--relocate]\n"
         "  partystuffer info ROM [options]\n"
         "  partystuffer dump ROM NEW_DIRECTORY [options]\n"
         "  partystuffer dump-sounds ROM NEW_DIRECTORY [--cic NUMBER]\n"
         "  partystuffer pack-sounds ROM DIRECTORY NEW_ROM (--bin|--wav) [--cic NUMBER]\n"
         "  partystuffer pack BASE_ROM DUMP_DIRECTORY NEW_ROM [options]\n"
         "  partystuffer checksum ROM NEW_ROM [--cic NUMBER]\n"
         );
    puts("Options:\n"
         "  --offset NUMBER --end NUMBER   Explicit archive bounds (end exclusive)\n"
         "  --cic NUMBER                   Override boot CIC identification\n"
         "  --compact                      Rebuild all offsets when packing\n"
         "  --relocate                     Append mainfs, expand ROM (US v1.0)\n"
         "  --sounds                       Include native audio and PCM WAVs in dump\n"
         );
    puts(
         "Numbers accept decimal or 0x-prefixed hex. Output paths must be new.\n"
         "US revision 0 profiles are automatic; other versions need explicit bounds.\n"
         "ROM byte order is detected and preserved. Pack always fixes header CRCs.");
}

int ps_cli_main(int argc, char **argv)
{
    static const size_t bases[3] = {0x31c7e0UL, 0x41dd30UL, 0x557e20UL};
    static const size_t ends[3] = {0xfcb860UL, 0x1142dd0UL, 0x1209850UL};
    Buf rom;
    Archive a;
    U32 crc1, crc2;
    size_t base, end, character;
    int i, first, mode, order, game, cic, compact, relocate, hasbase, hasend, sounds, audio_mode;
    char sound_path[PATH_SIZE];
    if (argc == 2 && (!strcmp(argv[1], "--help") || !strcmp(argv[1], "-h"))) { usage(); return 0; }
    if (argc < 3) { usage(); return 1; }
    if (!strcmp(argv[1], "info")) { mode = 0; first = 3; }
    else if (!strcmp(argv[1], "dump")) { mode = 1; first = 4; }
    else if (!strcmp(argv[1], "pack")) { mode = 2; first = 5; }
    else if (!strcmp(argv[1], "checksum")) { mode = 3; first = 4; }
    else if (!strcmp(argv[1], "inject-character")) { mode = 6; first = 6; }
    else if (!strcmp(argv[1], "pack-sounds")) { mode = 5; first = 5; }
    else if (!strcmp(argv[1], "dump-sounds")) { mode = 4; first = 4; }
    else { usage(); return 1; }
    if (argc < first) { usage(); return 1; }
    character = mode == 6 ? (size_t)number(argv[4]) : 0;
    audio_mode = -1;
    base = end = 0; cic = compact = relocate = hasbase = hasend = sounds = 0;
    for (i = first; i < argc; ++i) {
        if ((!strcmp(argv[i], "--bin") || !strcmp(argv[i], "--wav")) && mode == 5) {
            if (audio_mode != -1) fail("choose exactly one of --bin or --wav");
            audio_mode = !strcmp(argv[i], "--bin");
        }
        else if (!strcmp(argv[i], "--compact") && mode == 2) compact = 1;
        else if (!strcmp(argv[i], "--relocate") && (mode == 2 || mode == 6)) relocate = 1;
        else if (!strcmp(argv[i], "--sounds") && mode == 1) sounds = 1;
        else if (!strcmp(argv[i], "--offset") && (mode < 3 || mode == 6) && i + 1 < argc) { base = (size_t)number(argv[++i]); hasbase = 1; }
        else if (!strcmp(argv[i], "--end") && (mode < 3 || mode == 6) && i + 1 < argc) { end = (size_t)number(argv[++i]); hasend = 1; }
        else if (!strcmp(argv[i], "--cic") && i + 1 < argc) cic = (int)number(argv[++i]);
        else fail("unknown option or missing option value; use --help");
    }
    rom = read_file(argv[2]); order = rom_order(&rom);
    if (!cic) cic = cic_detect(&rom);
    checksum(&rom, cic, &crc1, &crc2);
    printf("Byte order: %s; CIC %d; CRC1 %08lX; CRC2 %08lX (%s)\n",
            order == 0 ? "z64" : order == 1 ? "v64" : "n64", cic,
            (unsigned long)crc1, (unsigned long)crc2,
            be32(rom.p + 16) == crc1 && be32(rom.p + 20) == crc2 ? "valid" : "repair needed");
    if (mode == 5) {
        if (audio_mode == -1) fail("pack-sounds requires --bin or --wav");
        pack_sounds(&rom, game_id(&rom), argv[3], audio_mode);
    }
    if (mode == 4) dump_sounds(&rom, game_id(&rom), argv[3]);
    if (mode < 3 || mode == 6) {
        game = game_id(&rom);
        if (hasbase != hasend) fail("--offset and --end must be supplied together");
        if (!hasbase) {
            if (rom.p[0x3e] != 'E' || rom.p[0x3f] != 0) fail("no built-in profile for this region/revision; provide --offset and --end");
            base = loader_address(&rom, game, 0, 0);
            if (base == bases[game-1]) end = ends[game-1];
            else if (base >= 0x2000000UL && base < rom.n) end = rom.n;
            else fail("nonstandard loader address; provide explicit archive bounds");
        }
        voice_rom_extent(&rom,game,base,&end);
        a = parse(&rom, base, end, game);
        printf("Mario Party %d: mainfs [0x%08lX, 0x%08lX), %lu directories, %lu files\n",
                game, (unsigned long)base, (unsigned long)end, (unsigned long)a.count, (unsigned long)a.total);
        if (mode == 6) inject_character(&rom, &a, argv[3], character, relocate);
        if (mode == 1) dump_archive(&rom, &a, argv[3]);
        if (sounds) {
            path_join(sound_path, argv[3], "sounds");
            dump_sounds(&rom, game, sound_path);
        }
        if (mode == 2) {
            pack_archive(&rom, &a, argv[3], compact, relocate, NULL, 0);
            base = a.base; end = a.end;
            free_archive(&a); a = parse(&rom, base, end, game);
        }
        free_archive(&a);
    }
    if (mode==6) {
        size_t padded;
        padded=0x100000UL;
        while (padded<rom.n && padded<LIMIT) padded*=2;
        if (padded<rom.n) fail("injected ROM exceeds maximum size");
        zeros(&rom,padded-rom.n);
    }
    if (mode == 2 || mode == 3 || mode == 5 || mode == 6) {
        checksum(&rom, cic, &crc1, &crc2); put32(rom.p + 16, crc1); put32(rom.p + 20, crc2);
        swap_order(&rom, order); write_new(argv[mode == 6 ? 5 : (mode == 2 || mode == 5) ? 4 : 3], rom.p, rom.n);
        printf("Wrote %s with corrected header checksums\n", argv[mode == 6 ? 5 : (mode == 2 || mode == 5) ? 4 : 3]);
    }
    free(rom.p); return 0;
}

/* Shared injection engine: no output is opened until this completes. */
Buf ps_build_character(const char *source, const char *folder, unsigned int character)
{
    static const size_t bases[3]={0x31c7e0UL,0x41dd30UL,0x557e20UL};
    static const size_t ends[3]={0xfcb860UL,0x1142dd0UL,0x1209850UL};
    Buf rom;
    Archive a;
    size_t base, end, padded;
    int game, order, cic;
    U32 crc1, crc2;
    rom=read_file(source); order=rom_order(&rom); cic=cic_detect(&rom);
    game=game_id(&rom);
    if (rom.p[0x3e]!='E' || rom.p[0x3f]) fail("library injection requires US revision 0");
    base=loader_address(&rom,game,0,0);
    if (base==bases[game-1]) end=ends[game-1];
    else if (base>=0x2000000UL && base<rom.n) end=rom.n;
    else { fail("nonstandard loader address"); end=0; }
    voice_rom_extent(&rom,game,base,&end);
    a=parse(&rom,base,end,game);
    inject_character(&rom,&a,folder,(size_t)character,0);
    free_archive(&a);
    padded=0x100000UL;
    while (padded<rom.n && padded<LIMIT) padded*=2;
    if (padded<rom.n) fail("injected ROM exceeds maximum size");
    zeros(&rom,padded-rom.n);
    checksum(&rom,cic,&crc1,&crc2);
    put32(rom.p+16,crc1); put32(rom.p+20,crc2);
    swap_order(&rom,order);
    return rom;
}
