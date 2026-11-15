#include <dos.h>
#include <string.h>
#include "video.h"
#include "videoint.h"

extern void __far *_fmemcpy(void __far *,const void __far *,size_t);
extern void __far *_fmemset(void __far *,int,size_t);

static unsigned char __far *vga = (unsigned char __far *)0xa0000000UL;

int videovga_start(void)
{
    union REGS r;
    r.w.ax = 0x0013;
    int86(0x10, &r, &r);
    r.h.ah = 0x0f;
    int86(0x10, &r, &r);
    return (r.h.al & 0x7f) == 0x13;
}

void videovga_pixel(int x, int y, unsigned char colour)
{
    vga[(unsigned)y * VIDEO_WIDTH + (unsigned)x] = colour;
}

unsigned char videovga_read(int x, int y)
{
    return vga[(unsigned)y * VIDEO_WIDTH + (unsigned)x];
}

void videovga_rect(int x, int y, int width, int height, unsigned char colour)
{
    int yy;
    unsigned row_offset;
    unsigned char __far *row;
    for (yy = y; yy < y + height; ++yy) {
        row_offset=(unsigned)yy*VIDEO_WIDTH+(unsigned)x;
        row=vga+row_offset;
        _fmemset(row,(int)colour,(size_t)width);
    }
}

void videovga_span(int x,int y,const unsigned char *pixels,int count)
{
    _fmemcpy(vga+(unsigned)y*VIDEO_WIDTH+(unsigned)x,pixels,(size_t)count);
}
