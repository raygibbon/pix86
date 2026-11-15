/* Independent DOS diagnostic BMP viewer, test tool only, not linked into Pix86. */
#include <stdio.h>
#include <string.h>
#include <bios.h>
#include <conio.h>
#include "video.h"
static unsigned char row[320];
static unsigned u16(const unsigned char *p)
{ return (unsigned)p[0] | (unsigned)p[1]<<8; }
static unsigned long u32(const unsigned char *p)
{ return (unsigned long)u16(p) | (unsigned long)u16(p+2)<<16; }
int main(int argc,char **argv)
{
    FILE *f;
    unsigned char h[54],q[4];
    unsigned w,height,i,x,y,pad;
    int ok;
    if(argc!=2) { puts("BMPVIEW FILE.BMP (8-bit BI_RGB, up to 320x200)"); return 1; }
    f=fopen(argv[1],"rb"); if(!f) return 1;
    if(fread(h,1,54,f)!=54 || h[0]!='B' || h[1]!='M' || u32(h+14)!=40 ||
       u32(h+10)!=1078 || u16(h+26)!=1 || u16(h+28)!=8 || u32(h+30)!=0 ||
       !u32(h+18) || u32(h+18)>320 || !u32(h+22) || u32(h+22)>200) { fclose(f); return 1; }
    w=(unsigned)u32(h+18); height=(unsigned)u32(h+22); pad=(4-w%4)%4;
    if(!video_init()) { fclose(f); return 1; }
    outp(0x3c8,0);
    ok=1;
    for(i=0;i<256;++i) {
        if(fread(q,1,4,f)!=4) { ok=0; break; }
        outp(0x3c9,((unsigned)q[2]*63U+127U)/255U);
        outp(0x3c9,((unsigned)q[1]*63U+127U)/255U);
        outp(0x3c9,((unsigned)q[0]*63U+127U)/255U);
    }
    video_clear(0);
    for(y=height;ok && y>0;--y) {
        if(fread(row,1,w,f)!=w) { ok=0; break; }
        for(x=0;x<w;++x) video_pixel((320-w)/2+x,(200-height)/2+y-1,row[x]);
        for(i=0;i<pad;++i) if(fgetc(f)==EOF) ok=0;
    }
    if(fclose(f)) ok=0;
    if(ok) (void)_bios_keybrd(_KEYBRD_READ);
    video_shutdown();
    puts(ok?"PASS: independent DOS BMP decode/display.":"FAIL: truncated BMP.");
    return ok?0:1;
}
