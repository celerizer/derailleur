#include "ps.h"

/* US rev0 resident GMes results-name pointer tables and owned string pools.
 * RAM = ROM + 0x7FFFF400. No overlay, dialogue or image data is searched.
 * MP1: 72D90.c func_80077838 / D_800C624C (WINS).
 * MP2: 69950 func_8006BEE8_6CAE8 / D_800CD028_CDC28 (WON).
 * MP3: gamemes func_8003DE60_3EA60 / D_800A1410_A2010 (WON).
 */
void inject_character_name(Buf *rom, int game, size_t character, const char *name)
{
    static const size_t tables[3]={0xC6E4CUL,0xCDC28UL,0xA2010UL};
    static const size_t starts[3]={0xCC644UL,0xD3EDCUL,0xA8300UL};
    static const size_t ends[3]={0xCC670UL,0xD3F0CUL,0xA8344UL};
    unsigned char pool[128];
    char names[8][32], upper[8];
    size_t table, start, end, count, i, j, off, used, len;
    U32 address;
    if (game<1 || game>3 || rom->n<0x40 || rom->p[0x3e]!='E' || rom->p[0x3f])
        fail("character name profiles require US revision 0");
    len=strlen(name);
    if (!len || len>7 || name[0]==' ' || name[len-1]==' ')
        fail("results name must be 1-7 letters/spaces, without edge spaces");
    for (i=0;i<len;++i) {
        unsigned char c;
        c=(unsigned char)name[i]; if (c>='a' && c<='z') c=(unsigned char)(c-'a'+'A');
        if ((c<'A' || c>'Z') && c!=' ') fail("results name supports letters and spaces only");
        upper[i]=(char)c;
    }
    upper[len]=0;
    table=tables[game-1]; start=starts[game-1]; end=ends[game-1]; count=game==3 ? 8 : 6;
    if (character>=count || end+6>rom->n || table+count*4>rom->n)
        fail("results name table outside ROM");
    if (memcmp(rom->p+end,game==1 ? "WINS\xC4" : "WON\xC4 ",5))
        fail("results name profile signature mismatch");
    for (i=0;i<count;++i) {
        address=be32(rom->p+table+i*4);
        if (address<0x7ffff400UL) fail("invalid results name pointer");
        off=(size_t)(address-0x7ffff400UL);
        if (off<start || off>=end) fail("results name pointer outside its pool");
        for (j=0;j<sizeof(names[i]);++j) {
            unsigned char c;
            if (off+j>=end) fail("unterminated results name");
            c=rom->p[off+j]; names[i][j]=(char)c; if (!c) break;
            if ((c<'A' || c>'Z') && c!=' ' && c!='\t') fail("unexpected results name encoding");
        }
        if (j==sizeof(names[i])) fail("results name too long");
    }
    /* MP2/3 use a seven-character centered field; a seven-letter name needs
     * no padding. Retain every other name's exact spacing/control bytes. */
    if (game==1) strcpy(names[character],upper);
    else {
        memset(names[character],' ',7); names[character][7]=0;
        memcpy(names[character]+(7-len)/2,upper,len);
    }
    if (game==2) {
        /* Keep the original eight-byte slots and canonical pointer table.
         * Read all names first so older repacked pools can also be repaired. */
        for (i=0;i<count;++i) {
            if (strlen(names[i])!=7) fail("MP2 results name must occupy seven characters");
            off=end-(i+1)*8;
            memcpy(rom->p+off,names[i],8);
            put32(rom->p+table+i*4,(U32)(0x7ffff400UL+off));
        }
        printf("Results name: %s\n",upper);
        return;
    }
    used=0; memset(pool,0,sizeof(pool));
    for (i=0;i<count;++i) {
        len=strlen(names[i])+1;
        if (len>end-start-used) fail("results names exceed their string pool");
        memcpy(pool+used,names[i],len); used+=len;
    }
    used=0;
    for (i=0;i<count;++i) {
        put32(rom->p+table+i*4,(U32)(0x7ffff400UL+start+used));
        used+=strlen(names[i])+1;
    }
    memcpy(rom->p+start,pool,end-start);
    printf("Results name: %s\n",upper);
}
