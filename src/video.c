#include <dos.h>
#include <i86.h>
#include <conio.h>
#include "video.h"

static unsigned char previous_mode;
static unsigned char previous_page;
static unsigned short previous_cursor;
static unsigned short previous_shape;
static int active;
static unsigned char __far *vga = (unsigned char __far *)0xa0000000UL;

static void set_mode(unsigned char mode)
{
    union REGS r;
    r.w.ax = mode;
    int86(0x10, &r, &r);
}

int video_init(void)
{
    union REGS r;
    int i;
    if (active)
        return 1;
    r.h.ah = 0x0f;
    int86(0x10, &r, &r);
    previous_mode = r.h.al & 0x7f;
    previous_page = r.h.bh;
    /* Only enter from text modes; returning to graphics is unsupported. */
    if (previous_mode > 3 && previous_mode != 7)
        return 0;
    r.h.ah = 3;
    r.h.bh = previous_page;
    int86(0x10, &r, &r);
    previous_cursor = r.w.dx;
    previous_shape = r.w.cx;
    set_mode(0x13);
    active = 1;
    r.h.ah = 0x0f;
    int86(0x10, &r, &r);
    if ((r.h.al & 0x7f) != 0x13) {
        video_shutdown();
        return 0;
    }
    /* Original restrained palette: greys, then a small RGB colour cube. */
    outp(0x3c8, 0);
    for (i = 0; i < 256; ++i) {
        if (i < 16) {
            outp(0x3c9, i * 4);
            outp(0x3c9, i * 4);
            outp(0x3c9, i * 4);
        } else {
            outp(0x3c9, ((i - 16) % 6) * 12);
            outp(0x3c9, (((i - 16) / 6) % 6) * 12);
            outp(0x3c9, (((i - 16) / 36) % 6) * 12);
        }
    }
    return 1;
}

void video_shutdown(void)
{
    union REGS r;
    if (!active)
        return;
    set_mode(previous_mode);
    r.h.ah = 5;
    r.h.al = previous_page;
    int86(0x10, &r, &r);
    r.h.ah = 1;
    r.w.cx = previous_shape;
    int86(0x10, &r, &r);
    r.h.ah = 2;
    r.h.bh = previous_page;
    r.w.dx = previous_cursor;
    int86(0x10, &r, &r);
    active = 0;
}

void video_pixel(int x, int y, unsigned char colour)
{
    if (x >= 0 && x < VIDEO_WIDTH && y >= 0 && y < VIDEO_HEIGHT)
        vga[(unsigned)y * VIDEO_WIDTH + (unsigned)x] = colour;
}

unsigned char video_read(int x, int y)
{
    if (x < 0 || x >= VIDEO_WIDTH || y < 0 || y >= VIDEO_HEIGHT)
        return 0;
    return vga[(unsigned)y * VIDEO_WIDTH + (unsigned)x];
}

void video_rect(int x, int y, int width, int height, unsigned char colour)
{
    long right, bottom;
    int xx, yy;
    unsigned offset;
    if (width <= 0 || height <= 0)
        return;
    right = (long)x + width;
    bottom = (long)y + height;
    if (right > VIDEO_WIDTH) right = VIDEO_WIDTH;
    if (bottom > VIDEO_HEIGHT) bottom = VIDEO_HEIGHT;
    if (x < 0) x = 0;
    if (y < 0) y = 0;
    for (yy = y; yy < bottom; ++yy) {
        offset = (unsigned)yy * VIDEO_WIDTH;
        for (xx = x; xx < right; ++xx)
            vga[offset + (unsigned)xx] = colour;
    }
}

void video_clear(unsigned char colour)
{
    video_rect(0, 0, VIDEO_WIDTH, VIDEO_HEIGHT, colour);
}
