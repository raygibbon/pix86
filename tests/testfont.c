#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>
#include "video.h"
#include "font.h"
static unsigned char screen[VIDEO_MAX_WIDTH*VIDEO_MAX_HEIGHT];
static unsigned char reference[VIDEO_MAX_WIDTH*VIDEO_MAX_HEIGHT];
static unsigned writes;
static int test_width=320,test_height=200;
int video_width(void) { return test_width; }
int video_height(void) { return test_height; }
void video_pixel(int x, int y, unsigned char colour)
{
    assert(x >= 0 && x < test_width && y >= 0 && y < test_height);
    screen[y*test_width+x] = colour;
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
    int code, x, y, different;
    unsigned before;
    char text[2];
    unsigned char upper[128];
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
    /* Every lowercase letter differs from its uppercase, and filename
       ambiguities remain distinguishable at the native cell resolution. */
    for(code='A';code<='Z';++code) {
        memset(screen,0,sizeof(screen)); text[0]=(char)code; video_text(0,0,text,173);
        for(y=0;y<8;++y) memcpy(upper+y*6,screen+y*320,6);
        memset(screen,0,sizeof(screen)); text[0]=(char)(code+32); video_text(0,0,text,173);
        before=0;
        for(y=0;y<8;++y) if(memcmp(upper+y*6,screen+y*320,6)) ++before;
        assert(before);
    }
    {
        const char *pairs="0O1I1lIl.,:;";
        for(code=0;pairs[code];code+=2) {
            memset(screen,0,sizeof(screen)); text[0]=pairs[code]; video_text(0,0,text,173);
            memcpy(reference,screen,sizeof(screen));
            memset(screen,0,sizeof(screen)); text[0]=pairs[code+1]; video_text(0,0,text,173);
            assert(memcmp(reference,screen,sizeof(screen)));
        }
    }
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
    font_set_mode(1);
    test_width=640; test_height=480;
    assert(FONT_WIDTH==8 && FONT_HEIGHT==16 && font_width("Save As")==56);
    memset(screen,0,sizeof(screen)); writes=0;
    video_text(400,350,"A",173);
    assert(writes>0);
    memset(screen,99,sizeof(screen)); writes=0;
    video_text_bg(400,350,"A",173,2);
    assert(writes==128);
    for(y=350;y<366;++y) for(x=400;x<408;++x)
        assert(screen[y*640+x]==2 || screen[y*640+x]==173);
    assert(screen[349*640+400]==99 && screen[350*640+399]==99);
    for (code=32;code<127;++code) {
        memset(screen,0,sizeof(screen)); writes=0; text[0]=(char)code;
        video_text(0,0,text,173);
        assert(code==32 ? writes==0 : writes>0);
        for(y=0;y<16;++y) for(x=8;x<640;++x) assert(screen[y*640+x]==0);
        for(y=16;y<480;++y) for(x=0;x<640;++x) assert(screen[y*640+x]==0);
    }
    memset(screen,0,sizeof(screen)); video_text(0,0,"A",173);
    for(y=0;y<16;++y) memcpy(upper+y*8,screen+y*640,8);
    memset(screen,0,sizeof(screen)); video_text(0,0,"a",173);
    different=0;
    for(y=0;y<16;++y) if(memcmp(upper+y*8,screen+y*640,8)) ++different;
    assert(different);
    memset(screen,0,sizeof(screen)); video_text(0,0,"0O1Il!.,:",173);
    for(y=0;y<16;++y) for(x=0;x<9*8;++x)
        if(screen[y*640+x]) assert(screen[y*640+x]==173);
    font_set_mode(0);
    puts("PASS: original 8x16 stroke font renders all printable ASCII and distinct lowercase.");
    return 0;
}
