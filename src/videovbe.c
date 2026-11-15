#include <dos.h>
#include <i86.h>
#include <string.h>
#include "video.h"
#include "videoint.h"
#include "profile.h"

#define VBE_MODE_640_480_256 0x0101

extern void __far *_fmemcpy(void __far *,const void __far *,size_t);
extern void __far *_fmemset(void __far *,int,size_t);

static unsigned short vbe_segment, vbe_granularity, vbe_window_size;
static unsigned short vbe_stride, current_bank;
static unsigned long current_bank_start;
static int bank_valid, video_fault;
static unsigned char vbe_controller[512];
static unsigned char vbe_mode_info[256];

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
        (vbe_mode_info[2] & 7) != 7 || !granularity ||
        !window_size || window_size > 64 || granularity > window_size ||
        !segment)
        return 0;
    vbe_segment = segment;
    vbe_granularity = granularity;
    vbe_window_size = window_size;
    vbe_stride = stride;
    return 1;
}

static void restore_mode(unsigned char mode)
{
    union REGS r;
    r.w.ax = mode;
    int86(0x10, &r, &r);
}

int videovbe_start(unsigned char previous_mode)
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
    if (r.w.ax != 0x004f) {
        restore_mode(previous_mode);
        return 0;
    }
    r.w.ax = 0x4f03;
    int86(0x10, &r, &r);
    if (r.w.ax != 0x004f || r.w.bx != VBE_MODE_640_480_256) {
        restore_mode(previous_mode);
        return 0;
    }
    bank_valid = video_fault = 0;
    return 1;
}

void videovbe_stop(void)
{
    bank_valid = video_fault = 0;
}

int videovbe_failed(void)
{
    return video_fault;
}

static unsigned char __far *vbe_address(unsigned long offset)
{
    unsigned long granularity, window_bytes, bank_start;
    unsigned short bank, window_offset;
    union REGS r;
    if (offset >= (unsigned long)vbe_stride * VIDEO_MAX_HEIGHT) {
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
        PROFILE_COUNT(vbe_bank_switches);
    }
    window_offset = (unsigned short)(offset - current_bank_start);
    return (unsigned char __far *)MK_FP(vbe_segment, window_offset);
}

void videovbe_pixel(int x, int y, unsigned char colour)
{
    unsigned char __far *address;
    address = vbe_address((unsigned long)y * vbe_stride + (unsigned)x);
    if (address)
        *address = colour;
}

unsigned char videovbe_read(int x, int y)
{
    unsigned char __far *address;
    address = vbe_address((unsigned long)y * vbe_stride + (unsigned)x);
    return address ? *address : 0;
}

void videovbe_rect(int x, int y, int width, int height, unsigned char colour)
{
    long offset, remaining, run;
    int xx, yy;
    for (yy = y; yy < y + height; ++yy) {
        offset = (long)yy * vbe_stride;
        xx = x;
        while (xx < x + width) {
            unsigned char __far *address;
            address = vbe_address((unsigned long)(offset + xx));
            if (!address)
                break;
            remaining = (long)vbe_window_size * 1024L -
                        ((offset + xx) - (long)current_bank_start);
            run = x + width - xx;
            if (run > remaining)
                run = remaining;
            _fmemset(address,(int)colour,(size_t)run);
            xx+=(int)run;
        }
    }
}

void videovbe_span(int x,int y,const unsigned char *pixels,int count)
{
    unsigned long offset,remaining;
    unsigned run;
    unsigned char __far *address;
    offset=(unsigned long)y*vbe_stride+(unsigned)x;
    while(count>0) {
        address=vbe_address(offset);
        if(!address) return;
        remaining=(unsigned long)vbe_window_size*1024UL-(offset-current_bank_start);
        run=(unsigned)count;
        if((unsigned long)run>remaining) run=(unsigned)remaining;
        _fmemcpy(address,pixels,run);
        pixels+=run; offset+=run; count-=(int)run;
    }
}
