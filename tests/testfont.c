#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>
#include "video.h"
#include "font.h"
static unsigned char screen[64000], reference[64000];
static unsigned writes;
void video_pixel(int x, int y, unsigned char colour)
{
    assert(x >= 0 && x < 320 && y >= 0 && y < 200);
    screen[y*320+x] = colour;
    ++writes;
}
static void clipping(int dx, int dy)
{
    int x, y, sx, sy;
    memset(screen, 0, sizeof(screen));
    video_text(dx, dy, "Abg 09!\\", 173);
    for (y = 0; y < 200; ++y)
        for (x = 0; x < 320; ++x) {
            sx = x-dx+10; sy = y-dy+10;
            assert(screen[y*320+x] == (sx >= 0 && sx < 320 && sy >= 0 && sy < 200 ?
                   reference[sy*320+sx] : 0));
        }
}
int main(void)
{
    int code, x, y;
    unsigned before;
    char text[2];
    unsigned char upper[48];
    text[1] = '\0';
    for (code = 32; code < 127; ++code) {
        memset(screen, 0, sizeof(screen)); writes = 0;
        text[0] = (char)code;
        video_text(0, 0, text, 173);
        assert(code == 32 ? writes == 0 : writes > 0);
        for (y = 0; y < 8; ++y) {
            assert(screen[y*320+5] == 0); /* spacing column */
            for (x = 6; x < 320; ++x) assert(!screen[y*320+x]);
        }
    }
    memset(screen,0,sizeof(screen)); video_text(0,0,"A",173);
    for (y=0;y<8;++y) memcpy(upper+y*6,screen+y*320,6);
    memset(screen,0,sizeof(screen)); video_text(0,0,"a",173);
    before=0;
    for (y=0;y<8;++y) if (memcmp(upper+y*6,screen+y*320,6)) ++before;
    assert(before); /* real lowercase, not uppercase substitution */
    memset(screen,0,sizeof(screen)); video_text(10,10,"Abg 09!\\",173);
    memcpy(reference,screen,sizeof(screen));
    clipping(-3,-2); clipping(300,196); clipping(-40,80); clipping(0,0);
    before=writes;
    video_text(INT_MIN,INT_MIN,"A",1); video_text(INT_MAX,INT_MAX,"A",1);
    video_text(0,INT_MAX,"A",1); video_text(INT_MIN,0,"A",1);
    video_text(0,-8,"A",1); video_text(320,0,"A",1);
    video_text(0,0,NULL,1); assert(before==writes);
    assert(font_width("Save As")==7*FONT_WIDTH && font_width(NULL)==0);
    memset(screen,99,sizeof(screen));
    video_text_box(3,4,7,5,"ABCD",173,2);
    for(y=0;y<200;++y) for(x=0;x<320;++x) {
        if(x>=3 && x<10 && y>=4 && y<9) assert(screen[y*320+x]==2 || screen[y*320+x]==173);
        else assert(screen[y*320+x]==99);
    }
    video_text_bg(20,20,"  ",1,2);
    for(y=20;y<28;++y) for(x=20;x<32;++x) assert(screen[y*320+x]==2);
    memset(screen,0,sizeof(screen)); video_text(0,0,"?",173); memcpy(reference,screen,sizeof(screen));
    memset(screen,0,sizeof(screen)); text[0]=(char)255; video_text(0,0,text,173);
    assert(!memcmp(screen,reference,sizeof(screen)));
    puts("PASS: original 6x8 glyphs, lowercase, spacing, clipping and extreme coordinates.");
    return 0;
}
