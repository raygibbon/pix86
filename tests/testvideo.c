#include <assert.h>
#include <dos.h>
#include <stdio.h>
#include "video.h"

static int current_mode(void)
{
    union REGS r;
    r.h.ah = 0x0f;
    int86(0x10, &r, &r);
    return r.h.al & 0x7f;
}

int main(int argc, char **argv)
{
    int before, hires, hires_active, width, height, x, y;
    unsigned colour;
    unsigned char row[640];
    (void)argv;
    before = current_mode();
    hires = argc > 1;
    video_request(hires);
    if (!video_init()) {
        puts("FAIL: could not initialize Mode 13h fallback.");
        return 1;
    }
    width = video_width();
    height = video_height();
    hires_active = video_hires();
    if (hires_active) {
        assert(width == 640 && height == 480);
        video_pixel(255,102,17);
        video_pixel(256,102,231);
        assert(video_read(255,102)==17 && video_read(256,102)==231);
        video_rect(252,101,12,3,109);
        for (y=101;y<104;++y)
            for (x=252;x<264;++x) assert(video_read(x,y)==109);
        for (colour=0;colour<256;++colour) {
            video_pixel((int)colour,200,(unsigned char)colour);
            assert(video_read((int)colour,200)==colour);
        }
    } else {
        assert(width == 320 && height == 200);
        for (colour=0;colour<256;++colour) {
            video_pixel((int)colour,100,(unsigned char)colour);
            assert(video_read((int)colour,100)==colour);
        }
        video_rect(37,103,19,4,173);
        for(y=103;y<107;++y) for(x=37;x<56;++x)
            assert(video_read(x,y)==173);
    }
    for(x=0;x<width;++x) row[x]=(unsigned char)x;
    /* VBE row 102 crosses the 64 KiB bank boundary at x=256. */
    video_span(0,102,row,width);
    for(x=0;x<width;++x) assert(video_read(x,102)==(unsigned char)x);
    video_rect(0,103,width,1,201);
    video_clip(4,103,8,1);
    video_span(0,103,row,20);
    video_unclip();
    for(x=0;x<20;++x)
        assert(video_read(x,103)==(x>=4 && x<12?(unsigned char)x:201));
    video_span(-4,104,row,12);
    for(x=0;x<8;++x) assert(video_read(x,104)==row[x+4]);
    video_span(width-4,height-1,row,20);
    for(x=0;x<4;++x) assert(video_read(width-4+x,height-1)==row[x]);
    video_pixel(width-1,height-1,255);
    assert(video_read(width-1,height-1)==255);
    video_pixel(3,4,98);
    video_clip(4,4,3,3);
    video_pixel(3,4,99);
    video_pixel(4,4,99);
    assert(video_read(3,4)==98 && video_read(4,4)==99);
    video_unclip();
    video_shutdown();
    assert(current_mode()==before);
    if (hires && !hires_active)
        puts("PASS: VBE unavailable; returned to Mode 13h and restored DOS video mode.");
    else if (hires_active)
        puts("PASS: VBE 640x480x256 bank boundaries, pixels, palette indices and restoration.");
    else
        puts("PASS: Mode 13h pixel bounds, palette indices and restoration.");
    return 0;
}
