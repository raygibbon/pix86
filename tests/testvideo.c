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
    (void)argv;
    before = current_mode();
    hires = argc > 1;
    if (!video_init(hires)) {
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
    }
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
