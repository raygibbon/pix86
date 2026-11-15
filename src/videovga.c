#include <dos.h>
#include "video.h"
#include "videoint.h"

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
    int xx, yy;
    for (yy = y; yy < y + height; ++yy)
        for (xx = x; xx < x + width; ++xx)
            videovga_pixel(xx, yy, colour);
}
