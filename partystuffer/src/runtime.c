#define PS_RUNTIME_IMPLEMENTATION
#include "ps.h"
#include <setjmp.h>
#include <stdarg.h>

typedef struct Resource Resource;
struct Resource { void *memory; FILE *stream; Resource *next; };
static Resource *resources;
static int active;
static char error_text[512];
jmp_buf ps_error_jump;

void ps_scope_begin(void)
{
    active=1; error_text[0]=0;
}

void ps_scope_end(void)
{
    Resource *r;
    while (resources) {
        r=resources; resources=r->next;
        if (r->stream) fclose(r->stream);
        else free(r->memory);
        free(r);
    }
    active=0;
}

const char *partystuffer_last_error(void) { return error_text; }

void ps_raise(const char *message)
{
    if (!active) return;
    strncpy(error_text,message,sizeof(error_text)-1);
    error_text[sizeof(error_text)-1]=0;
    longjmp(ps_error_jump,1);
}

static void track(void *p, FILE *f)
{
    Resource *r;
    if (!active) return;
    r=malloc(sizeof(*r));
    if (!r) {
        if (f) fclose(f); else free(p);
        fail("out of memory tracking resources");
    }
    r->memory=p; r->stream=f; r->next=resources; resources=r;
}

void *ps_malloc(size_t n)
{
    void *p;
    p=malloc(n); if (p) track(p,NULL); return p;
}

void *ps_realloc(void *p, size_t n)
{
    Resource *r;
    void *q;
    if (!p) return ps_malloc(n);
    for (r=resources;r;r=r->next) if (!r->stream && r->memory==p) break;
    q=realloc(p,n);
    if (q && r) r->memory=q;
    return q;
}

void ps_free(void *p)
{
    Resource **link, *r;
    if (!p) return;
    for (link=&resources;*link;link=&(*link)->next)
        if (!(*link)->stream && (*link)->memory==p) {
            r=*link; *link=r->next; free(r); break;
        }
    free(p);
}

FILE *ps_fopen(const char *path, const char *mode)
{
    FILE *f;
    f=fopen(path,mode); if (f) track(NULL,f); return f;
}

int ps_fclose(FILE *f)
{
    Resource **link, *r;
    for (link=&resources;*link;link=&(*link)->next)
        if ((*link)->stream==f) {
            r=*link; *link=r->next; free(r); break;
        }
    return fclose(f);
}

int ps_printf(const char *format, ...)
{
    int result;
    va_list args;
    if (active) return 0;
    va_start(args,format); result=vprintf(format,args); va_end(args);
    return result;
}

int ps_fprintf(FILE *stream, const char *format, ...)
{
    int result;
    va_list args;
    if (active && (stream==stdout || stream==stderr)) return 0;
    va_start(args,format); result=vfprintf(stream,format,args); va_end(args);
    return result;
}
