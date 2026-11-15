#include <dos.h>
#include <conio.h>
#include "video.h"
#include "videoint.h"
#include "profile.h"

static unsigned char previous_mode;
static unsigned char previous_page;
static unsigned short previous_cursor;
static unsigned short previous_shape;
static int active, hires;
static int screen_width = VIDEO_WIDTH, screen_height = VIDEO_HEIGHT;
static int clip_x, clip_y, clip_right = VIDEO_WIDTH, clip_bottom = VIDEO_HEIGHT;

static void set_mode(unsigned char mode)
{
    union REGS r;
    r.w.ax = mode;
    int86(0x10, &r, &r);
}

static void restore_display(void)
{
    union REGS r;
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
}

static void set_palette(void)
{
    int i;
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
}

static int requested_hires;
void video_request(int value) { requested_hires=value; }
int video_init(void)
{
    union REGS r;
    if (active)
        return 1;
    r.h.ah = 0x0f;
    int86(0x10, &r, &r);
    previous_mode = r.h.al & 0x7f;
    previous_page = r.h.bh;
    if (previous_mode > 3 && previous_mode != 7)
        return 0;
    r.h.ah = 3;
    r.h.bh = previous_page;
    int86(0x10, &r, &r);
    previous_cursor = r.w.dx;
    previous_shape = r.w.cx;

    hires = requested_hires && videovbe_start(previous_mode);
    if (!hires && !videovga_start()) {
        restore_display();
        return 0;
    }
    active = 1;
    screen_width = hires ? VIDEO_MAX_WIDTH : VIDEO_WIDTH;
    screen_height = hires ? VIDEO_MAX_HEIGHT : VIDEO_HEIGHT;
    set_palette();
    video_unclip();
    return 1;
}

int video_hires(void) { return hires; }
int video_failed(void) { return hires && videovbe_failed(); }
int video_width(void) { return screen_width; }
int video_height(void) { return screen_height; }

void video_shutdown(void)
{
    if (!active)
        return;
    if (hires)
        videovbe_stop();
    restore_display();
    active = hires = 0;
    screen_width = VIDEO_WIDTH;
    screen_height = VIDEO_HEIGHT;
    video_unclip();
}

void video_unclip(void)
{
    clip_x = clip_y = 0;
    clip_right = screen_width;
    clip_bottom = screen_height;
}

void video_clip(int x, int y, int width, int height)
{
    long right, bottom;
    right = (long)x + width;
    bottom = (long)y + height;
    clip_x = x < 0 ? 0 : x;
    clip_y = y < 0 ? 0 : y;
    clip_right = (int)(right > screen_width ? screen_width : right < 0 ? 0 : right);
    clip_bottom = (int)(bottom > screen_height ? screen_height : bottom < 0 ? 0 : bottom);
    if (width <= 0 || height <= 0)
        clip_right = clip_bottom = 0;
}

void video_span(int x,int y,const unsigned char *pixels,int count)
{
    int skip;
    if(y<clip_y || y>=clip_bottom || count<=0 || x>=clip_right) return;
    if(x<clip_x) {
        if((long)x+count<=clip_x) return;
        skip=clip_x-x;
        if(skip>=count) return;
        pixels+=skip; count-=skip; x=clip_x;
    }
    if(count>clip_right-x) count=clip_right-x;
    PROFILE_ADD(framebuffer_pixel_writes,count);
    PROFILE_COUNT(scanlines);
    if(hires) videovbe_span(x,y,pixels,count);
    else videovga_span(x,y,pixels,count);
}

void video_pixel(int x, int y, unsigned char colour)
{
    if (x < clip_x || x >= clip_right || y < clip_y || y >= clip_bottom)
        return;
    PROFILE_COUNT(framebuffer_pixel_writes);
    if (hires)
        videovbe_pixel(x, y, colour);
    else
        videovga_pixel(x, y, colour);
}

unsigned char video_read(int x, int y)
{
    if (x < 0 || x >= screen_width || y < 0 || y >= screen_height)
        return 0;
    return hires ? videovbe_read(x, y) : videovga_read(x, y);
}

void video_rect(int x, int y, int width, int height, unsigned char colour)
{
    long right, bottom;
    right = (long)x + width;
    bottom = (long)y + height;
    if (width <= 0 || height <= 0)
        return;
    if (right > clip_right) right = clip_right;
    if (bottom > clip_bottom) bottom = clip_bottom;
    if (x < clip_x) x = clip_x;
    if (y < clip_y) y = clip_y;
    if (x >= right || y >= bottom)
        return;
    PROFILE_COUNT(rectangles);
    PROFILE_ADD(scanlines,bottom-y);
    PROFILE_ADD(framebuffer_pixel_writes,
                (unsigned long)(right-x)*(unsigned long)(bottom-y));
    if (hires)
        videovbe_rect(x, y, (int)(right - x), (int)(bottom - y), colour);
    else
        videovga_rect(x, y, (int)(right - x), (int)(bottom - y), colour);
}

void video_clear(unsigned char colour)
{
    video_rect(0, 0, screen_width, screen_height, colour);
}
