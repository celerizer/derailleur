#ifndef _WIN32
#define _POSIX_C_SOURCE 200809L
#endif
#include "ps.h"
#include "partystuffer.h"
#include <setjmp.h>
#include <errno.h>
#include <sys/stat.h>
#ifdef _WIN32
#include <windows.h>
#include <io.h>
#include <fcntl.h>
#include <process.h>
#else
#include <unistd.h>
#endif
extern jmp_buf ps_error_jump;
void ps_scope_begin(void);
void ps_scope_end(void);

/* The temporary file lives beside the target, so rename never crosses devices.
 * Explicitly close/remove it before raising any error. */
static void replace_output(const char *target, const Buf *rom)
{
    char *temporary;
    size_t length;
    FILE *stream;
    int fd, bad;
#ifdef _WIN32
    unsigned int attempt;
#else
    struct stat previous;
    int exists;
#endif
    length=strlen(target);
    if (length>LIMIT-64) fail("target filename too long");
    temporary=alloc(length+64);
#ifdef _WIN32
    fd=-1;
    for (attempt=0;attempt<1000;++attempt) {
        sprintf(temporary,"%s.partystuffer-%lu-%u.tmp",target,(unsigned long)_getpid(),attempt);
        fd=_open(temporary,_O_WRONLY|_O_CREAT|_O_EXCL|_O_BINARY,_S_IREAD|_S_IWRITE);
        if (fd>=0 || errno!=EEXIST) break;
    }
    if (fd<0) fail("cannot create temporary ROM beside target");
    stream=_fdopen(fd,"wb");
    if (!stream) { _close(fd); remove(temporary); fail("cannot open temporary ROM stream"); }
#else
    strcpy(temporary,target); strcat(temporary,".partystuffer-XXXXXX");
    exists=stat(target,&previous)==0;
    fd=mkstemp(temporary);
    if (fd<0) fail("cannot create temporary ROM beside target");
    if (exists && fchmod(fd,previous.st_mode&0777)) {
        close(fd); remove(temporary); fail("cannot preserve target permissions");
    }
    stream=fdopen(fd,"wb");
    if (!stream) { close(fd); remove(temporary); fail("cannot open temporary ROM stream"); }
#endif
    bad=fwrite(rom->p,1,rom->n,stream)!=rom->n;
    if (fflush(stream)) bad=1;
#ifdef _WIN32
    if (_commit(fd)) bad=1;
#else
    if (fsync(fd)) bad=1;
#endif
    if (fclose(stream)) bad=1;
    if (bad) { remove(temporary); fail("cannot write complete temporary ROM"); }
#ifdef _WIN32
    if (!MoveFileExA(temporary,target,MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH)) {
#else
    if (rename(temporary,target)) {
#endif
        remove(temporary); fail("cannot replace target ROM");
    }
    free(temporary);
}

int partystuffer_inject_character(const char *source_rom,
    const char *target_rom, const char *mod_folder, unsigned int character_id)
{
    Buf rom;
    ps_scope_begin();
    if (setjmp(ps_error_jump)) {
        ps_audio_reset(); ps_scope_end(); return PARTYSTUFFER_ERROR;
    }
    ps_audio_reset();
    if (!source_rom || !*source_rom || !target_rom || !*target_rom || !mod_folder || !*mod_folder)
        fail("source ROM, target ROM and mod folder are required");
    rom=ps_build_character(source_rom,mod_folder,character_id);
    replace_output(target_rom,&rom);
    free(rom.p);
    ps_audio_reset(); ps_scope_end(); return PARTYSTUFFER_OK;
}
