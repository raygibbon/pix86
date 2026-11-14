#include <dos.h>
#include <i86.h>
#include <conio.h>
#include <string.h>
#include "video.h"

#define VBE_MODE_640_480_256 0x0101
#define VBE_WINDOW_SEGMENT 0xa000

static unsigned char previous_mode;
static unsigned char previous_page;
static unsigned short previous_cursor;
static unsigned short previous_shape;
static int active, hires, video_fault;
static int screen_width = VIDEO_WIDTH, screen_height = VIDEO_HEIGHT;
static int clip_x, clip_y, clip_right = VIDEO_WIDTH, clip_bottom = VIDEO_HEIGHT;
static unsigned short vbe_segment, vbe_granularity, vbe_window_size;
static unsigned short vbe_stride, current_bank;
static unsigned long current_bank_start;
static int bank_valid;
static unsigned char vbe_controller[512];
static unsigned char vbe_mode_info[256];
static unsigned char __far *vga = (unsigned char __far *)0xa0000000UL;

static void set_mode(unsigned char mode)
{
    union REGS r;
    r.w.ax = mode;
    int86(0x10, &r, &r);
}

static unsigned short word_at(const unsigned char *data, unsigned offset)
{
    return (unsigned short)(data[offset] | ((unsigned short)data[offset + 1] << 8));
}

static int vbe_query(unsigned short function, unsigned short mode,
                     unsigned char *buffer)
{
    union REGS r;
    struct SREGS s;
    r.w.ax = function;
    r.w.cx = mode;
    r.w.di = _FP_OFF(buffer);
    segread(&s);
    s.es = _FP_SEG(buffer);
    int86x(0x10, &r, &r, &s);
    return r.w.ax == 0x004f;
}

static int vbe_mode_supported(void)
{
    unsigned short attributes, granularity, window_size, segment, stride;
    if (memcmp(vbe_controller, "VESA", 4) != 0 ||
        word_at(vbe_controller, 4) < 0x0102)
        return 0;
    if (!vbe_query(0x4f01, VBE_MODE_640_480_256, vbe_mode_info))
        return 0;
    attributes = word_at(vbe_mode_info, 0);
    granularity = word_at(vbe_mode_info, 4);
    window_size = word_at(vbe_mode_info, 6);
    segment = word_at(vbe_mode_info, 8);
    stride = word_at(vbe_mode_info, 16);
    if ((attributes & 0x0019) != 0x0019 ||
        word_at(vbe_mode_info, 18) != VIDEO_MAX_WIDTH ||
        word_at(vbe_mode_info, 20) != VIDEO_MAX_HEIGHT ||
        vbe_mode_info[24] != 1 || vbe_mode_info[25] != 8 ||
        vbe_mode_info[27] != 4 || stride < VIDEO_MAX_WIDTH ||
        (vbe_mode_info[2] & 5) != 5 || !granularity ||
        !window_size || window_size > 64 || granularity > window_size ||
        !segment)
        return 0;
    vbe_segment = segment;
    vbe_granularity = granularity;
    vbe_window_size = window_size;
    vbe_stride = stride;
    return 1;
}

static int set_vbe_mode(void)
{
    union REGS r;
    struct SREGS s;
    r.w.ax = 0x4f00;
    r.w.di = _FP_OFF(vbe_controller);
    segread(&s);
    s.es = _FP_SEG(vbe_controller);
    int86x(0x10, &r, &r, &s);
    if (r.w.ax != 0x004f || !vbe_mode_supported())
        return 0;
    r.w.ax = 0x4f02;
    r.w.bx = VBE_MODE_640_480_256;
    int86(0x10, &r, &r);
    if (r.w.ax != 0x004f)
        return 0;
    r.w.ax = 0x4f03;
    int86(0x10, &r, &r);
    if (r.w.ax != 0x004f || r.w.bx != VBE_MODE_640_480_256) {
        set_mode(previous_mode);
        return 0;
    }
    hires = 1;
    screen_width = VIDEO_MAX_WIDTH;
    screen_height = VIDEO_MAX_HEIGHT;
    bank_valid = 0;
    return 1;
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

int video_init(int request_hires)
{
    union REGS r;
    if (active)
        return 1;
    video_fault = 0;
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

    hires = 0;
    if (request_hires && set_vbe_mode()) {
        active = 1;
        set_palette();
    } else {
        set_mode(0x13);
        active = 1;
        screen_width = VIDEO_WIDTH;
        screen_height = VIDEO_HEIGHT;
        r.h.ah = 0x0f;
        int86(0x10, &r, &r);
        if ((r.h.al & 0x7f) != 0x13) {
            video_shutdown();
            return 0;
        }
        set_palette();
    }
    video_unclip();
    return 1;
}

int video_hires(void) { return hires; }
int video_failed(void) { return video_fault; }
int video_width(void) { return screen_width; }
int video_height(void) { return screen_height; }

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
    active = hires = bank_valid = 0;
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

static unsigned char __far *vbe_address(unsigned long offset)
{
    unsigned long granularity, window_bytes, bank_start;
    unsigned short bank, window_offset;
    union REGS r;
    if(offset >= (unsigned long)vbe_stride * screen_height) {
        video_fault = 1;
        return (unsigned char __far *)0;
    }
    window_bytes = (unsigned long)vbe_window_size * 1024UL;
    if (bank_valid && offset >= current_bank_start &&
        offset - current_bank_start < window_bytes) {
        window_offset = (unsigned short)(offset - current_bank_start);
        return (unsigned char __far *)MK_FP(vbe_segment, window_offset);
    }
    granularity = (unsigned long)vbe_granularity * 1024UL;
    bank = (unsigned short)(offset / granularity);
    bank_start = (unsigned long)bank * granularity;
    if (!bank_valid || bank != current_bank) {
        r.w.ax = 0x4f05;
        r.w.bx = 0;
        r.w.dx = bank;
        int86(0x10, &r, &r);
        if (r.w.ax != 0x004f) {
            bank_valid = 0;
            video_fault = 1;
            return (unsigned char __far *)0;
        }
        current_bank = bank;
        current_bank_start = bank_start;
        bank_valid = 1;
    }
    window_offset = (unsigned short)(offset - current_bank_start);
    return (unsigned char __far *)MK_FP(vbe_segment, window_offset);
}

void video_pixel(int x, int y, unsigned char colour)
{
    if (x < clip_x || x >= clip_right || y < clip_y || y >= clip_bottom)
        return;
    if (hires) {
        unsigned char __far *address;
        address = vbe_address((unsigned long)y * vbe_stride + (unsigned)x);
        if (address)
            *address = colour;
    } else {
        vga[(unsigned)y * VIDEO_WIDTH + (unsigned)x] = colour;
    }
}

unsigned char video_read(int x, int y)
{
    unsigned char __far *address;
    if (x < 0 || x >= screen_width || y < 0 || y >= screen_height)
        return 0;
    if (!hires)
        return vga[(unsigned)y * VIDEO_WIDTH + (unsigned)x];
    address = vbe_address((unsigned long)y * vbe_stride + (unsigned)x);
    return address ? *address : 0;
}

void video_rect(int x, int y, int width, int height, unsigned char colour)
{
    long right, bottom, offset, remaining, run;
    int xx, yy;
    if (width <= 0 || height <= 0)
        return;
    right = (long)x + width;
    bottom = (long)y + height;
    if (right > clip_right) right = clip_right;
    if (bottom > clip_bottom) bottom = clip_bottom;
    if (x < clip_x) x = clip_x;
    if (y < clip_y) y = clip_y;
    if (x >= right || y >= bottom)
        return;
    for (yy = y; yy < bottom; ++yy) {
        offset = (long)yy * (hires ? vbe_stride : VIDEO_WIDTH);
        if (hires) {
            xx = x;
            while (xx < right) {
                unsigned char __far *address;
                address = vbe_address((unsigned long)(offset + xx));
                if (!address)
                    break;
                remaining = (long)vbe_window_size * 1024L -
                            ((offset + xx) - (long)current_bank_start);
                run = right - xx;
                if (run > remaining) run = remaining;
                while (run-- > 0) *address++ = colour;
                xx += (int)(right - xx < remaining ? right - xx : remaining);
            }
        } else {
            for (xx = x; xx < right; ++xx) {
                vga[(unsigned)yy * VIDEO_WIDTH + (unsigned)xx] = colour;
            }
        }
    }
}

void video_clear(unsigned char colour)
{
    video_rect(0, 0, screen_width, screen_height, colour);
}
