#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "command.h"
#include "cursor.h"
#include "editor.h"
#include "layout.h"
#include "menu.h"
#include "mouse.h"
#include "profile.h"
#include "timer.h"
#include "screen.h"
#include "theme.h"
#include "video.h"

#define PROFILE_RENDERS 32

static Editor editor;
static unsigned short fill_queue[4096];
static Screen screen;
static Menu menu;
static MouseState mouse;

static void report_delta(const char *label,const ProfileStats *before)
{
    printf("DOS %s writes=%lu full=%lu partial=%lu cells=%lu compares=%lu ",
           label,pix86_profile.framebuffer_pixel_writes-before->framebuffer_pixel_writes,
           pix86_profile.canvas_full-before->canvas_full,
           pix86_profile.canvas_partial-before->canvas_partial,
           pix86_profile.canvas_cells-before->canvas_cells,
           pix86_profile.canvas_compares-before->canvas_compares);
    printf("timeline=%lu/%lu screen=%lu ticks in %lu samples\n",
           pix86_profile.timeline_full-before->timeline_full,
           pix86_profile.timeline_partial-before->timeline_partial,
           pix86_profile.screen_ticks-before->screen_ticks,
           pix86_profile.screen_samples-before->screen_samples);
}

static void setup(unsigned size)
{
    editor_dispose(&editor);
    if(!editor_init(&editor,size,fill_queue)) {
        puts("PERFDOS: editor initialization failed.");
        video_shutdown();
        exit(1);
    }
    editor.timeline=1;
    view_fit(&editor.view,&editor.canvas,1,0);
    menu_init(&menu);
    mouse.x=mouse.y=0; mouse.buttons=0;
    screen_init(&screen,&editor,&mouse,0);
}

static void benchmark_size(unsigned size)
{
    ProfileStats before;
    unsigned i,flags;
    unsigned long count;
    int x0,y0,x1,y1;
    char label[32];

    setup(size);
    before=pix86_profile;
    for(i=0;i<PROFILE_RENDERS;++i)
        screen_update(&screen,&editor,&menu,EDIT_CANVAS,&mouse,0);
    sprintf(label,"%ux%u full x%u",size,size,PROFILE_RENDERS);
    report_delta(label,&before);

    editor_store(&editor);
    flags=editor_key(&editor,CMD_FRAME_DUP);
    screen_update(&screen,&editor,&menu,flags,&mouse,0);
    count=editor.history.size;
    for(i=0;i<count;++i)
        if(i%4) editor.pixels[i]=(unsigned char)(editor.pixels[i]+15);
    editor_store(&editor);
    flags=editor_select(&editor,0);
    screen_update(&screen,&editor,&menu,flags,&mouse,0);
    before=pix86_profile;
    for(i=0;i<PROFILE_RENDERS;++i) {
        flags=editor_select(&editor,i%2?0:1);
        screen_update(&screen,&editor,&menu,flags,&mouse,0);
    }
    sprintf(label,"%ux%u 75%% frame diff x%u",size,size,PROFILE_RENDERS);
    report_delta(label,&before);

    editor_key(&editor,CMD_PLAY);
    screen_update(&screen,&editor,&menu,EDIT_TIMELINE|EDIT_STATUS,&mouse,0);
    before=pix86_profile;
    for(i=0;i<PROFILE_RENDERS;++i) {
        flags=editor_tick(&editor,editor.document->duration[editor.history.active]);
        screen_update(&screen,&editor,&menu,flags,&mouse,0);
    }
    sprintf(label,"%ux%u playback x%u",size,size,PROFILE_RENDERS);
    report_delta(label,&before);
    editor_key(&editor,CMD_PLAY);
    screen_update(&screen,&editor,&menu,EDIT_TIMELINE|EDIT_STATUS,&mouse,0);

    editor.tool=TOOL_PENCIL;
    editor.foreground=200;
    x0=editor.view.x+(int)editor.view.zoom;
    y0=editor.view.y+(int)editor.view.zoom;
    x1=x0+((int)size/2)*(int)editor.view.zoom;
    y1=y0+((int)size/2)*(int)editor.view.zoom;
    before=pix86_profile;
    flags=editor_mouse(&editor,x0,y0,EDIT_LEFT);
    screen_update(&screen,&editor,&menu,flags,&mouse,0);
    flags=editor_mouse(&editor,x1,y1,EDIT_LEFT);
    screen_update(&screen,&editor,&menu,flags,&mouse,0);
    sprintf(label,"%ux%u diagonal stroke",size,size);
    report_delta(label,&before);
}

static void benchmark_transition(unsigned width,unsigned height,unsigned percent,
                                int scattered,int zoom_one,int solid_colour)
{
    unsigned size,changed,i,zoom,strategy;
    unsigned long elapsed,start;
    ProfileStats before;
    char label[40];

    editor_dispose(&editor);
    if(!editor_init(&editor,width,fill_queue)) exit(1);
    if(height!=width && !editor_new_shape(&editor,width,height)) exit(1);
    editor.timeline=0;
    view_fit(&editor.view,&editor.canvas,0,0);
    if(zoom_one) view_set(&editor.view,&editor.canvas,1);
    zoom=editor.view.zoom;
    size=width*height;
    for(i=0;i<size;++i)
        editor.pixels[i]=(unsigned char)(solid_colour?32U:i%239U+1U);
    if(solid_colour==3) memset(editor.mask,255,size/8);
    editor_store(&editor);
    editor.document->frames=2;
    anim_copy(editor.document->pixels+size,editor.pixels,size);
    anim_set(editor.document->mask+size/8,0,size/8);
    if(solid_colour==2) anim_set(editor.document->mask+size/8,255,size/8);
    if(solid_colour==4) { editor.grid=1; editor.guide=8; }
    changed=(unsigned)((unsigned long)size*percent/100UL);
    for(i=0;i<size;++i) {
        int selected;
        selected=scattered?((i*37UL)%size<changed):(i<changed);
        if(selected)
            editor.document->pixels[size+i]=(unsigned char)(solid_colour?200U:
                                                               editor.pixels[i]^0x55U);
    }
    editor.history.active=0;
    editor_restore(&editor);
    video_clear(UI_DESK);
    render_canvas(&editor);
    editor.history.active=1;
    editor_restore(&editor);

    for(strategy=0;strategy<4;++strategy) {
#ifdef PIX86_BASELINE
        if(strategy) continue;
#endif
        pix86_render_strategy=strategy;
        before=pix86_profile;
        start=profile_now();
        for(i=0;i<8;++i) render_playback_canvas(&editor,0);
        elapsed=profile_now()-start;
        sprintf(label,"%ux%u %u%% %s/%s z%u",width,height,percent,
                solid_colour==2?"to-alpha":solid_colour==3?"to-opaque":
                solid_colour==4?"grid":solid_colour?"flat":"texture",scattered?"scatter":"solid",zoom);
        printf("BENCH %s strategy=%u n=8 pit=%lu us/frame=%lu rects=%lu\n",
               label,strategy,elapsed,(elapsed/8)*1000UL/1193UL,
               pix86_profile.rectangles-before.rectangles);
    }
    pix86_render_strategy=0;
}

static void benchmark_percentages(void)
{
    static const unsigned shapes[4][2]={
        {16,16},{16,24},{32,32},{64,64}
    };
    static const unsigned percentages[5]={0,25,50,75,100};
    unsigned shape,percent;
    for(shape=0;shape<4;++shape)
        benchmark_transition(shapes[shape][0],shapes[shape][1],1,1,0,0);
    for(shape=0;shape<4;++shape)
        for(percent=0;percent<5;++percent) {
            benchmark_transition(shapes[shape][0],shapes[shape][1],
                                 percentages[percent],0,0,0);
            benchmark_transition(shapes[shape][0],shapes[shape][1],
                                 percentages[percent],1,0,0);
        }
    for(shape=0;shape<4;++shape)
        for(percent=1;percent<5;++percent)
            benchmark_transition(shapes[shape][0],shapes[shape][1],
                                 percentages[percent],0,0,1);
    for(shape=0;shape<4;++shape) {
        benchmark_transition(shapes[shape][0],shapes[shape][1],100,0,0,2);
        benchmark_transition(shapes[shape][0],shapes[shape][1],100,0,0,3);
        benchmark_transition(shapes[shape][0],shapes[shape][1],100,0,0,4);
    }
    benchmark_transition(16,16,75,0,1,1);
    benchmark_transition(64,64,75,0,1,1);
}

static void benchmark_playback(void)
{
    unsigned i,flags,displayed,expected;
    unsigned long start,last,now,ms,fps100;
    setup(64);
    editor.timeline=0;
    view_fit(&editor.view,&editor.canvas,0,0);
    for(i=0;i<editor.history.size;++i) editor.pixels[i]=(unsigned char)i;
    editor_store(&editor); editor.document->frames=2;
    for(i=0;i<editor.history.size;++i)
        editor.document->pixels[editor.history.size+i]=(unsigned char)(i^0x55);
    anim_set(editor.document->mask+editor.history.size/8,0,editor.history.size/8);
    editor.document->duration[0]=editor.document->duration[1]=33;
    screen_init(&screen,&editor,&mouse,0);
    flags=editor_key(&editor,CMD_PLAY);
    screen_update(&screen,&editor,&menu,flags,&mouse,0);
    start=last=timer_now(); displayed=0; expected=0;
    do {
        now=timer_now(); ms=TIMER_MS(now-last); last+=ms*1193UL;
        flags=editor_tick(&editor,(unsigned)ms);
        screen_update(&screen,&editor,&menu,flags,&mouse,0);
        if(flags&EDIT_PLAYBACK) {
            ++displayed; expected^=1;
            if(editor.history.active!=expected) {
                puts("FAIL: playback frame sequence"); break;
            }
        }
    } while(timer_now()-start<3UL*TIMER_HZ);
    now=timer_now()-start;
    fps100=(unsigned long)displayed*TIMER_HZ/(now/100);
    printf("PLAY 64x64 dense z%d frames=%u pit=%lu target=30.30 fps=%lu.%02lu missed=%lu render=%ums\n",
           editor.view.zoom,displayed,now,
           fps100/100,
           fps100%100,
           editor.playback_dropped,screen.playback_render_ms);
    editor_key(&editor,CMD_PLAY);
}

int main(int argc,char **argv)
{
    int hires;
    unsigned sizes[3];
    hires=argc>1 && strcmp(argv[1],"/HIRES")==0;
    sizes[0]=16; sizes[1]=32; sizes[2]=64;
    video_request(hires);
    if(!video_init()) {
        puts("PERFDOS: video initialization failed.");
        return 1;
    }
    layout_set(video_width(),video_height());
    printf("PERFDOS mode=%s %dx%d\n",video_hires()?"VBE":"VGA",
           video_width(),video_height());
    mouse_init();
    {
        unsigned i;
        unsigned long previous,current;
        timer_precise(); previous=timer_now();
        for(i=0;i<10000;++i) {
            current=timer_now();
            if(current-previous>TIMER_HZ) {
                video_shutdown(); printf("FAIL: timer prev=%lu now=%lu precise=%d\n",previous,current,timer_precise()); return 1;
            }
            previous=current;
        }
        puts("PASS: 10000 successive timer samples monotonic");
    }
    if((argc>1 && strcmp(argv[1],"/PLAYBACK")==0) ||
       (argc>2 && strcmp(argv[2],"/PLAYBACK")==0)) {
        benchmark_playback();
        mouse_shutdown(); video_shutdown(); editor_dispose(&editor); return 0;
    }
    benchmark_size(sizes[0]);
    benchmark_size(sizes[1]);
    benchmark_size(sizes[2]);
    benchmark_percentages();
    benchmark_playback();
    {
        ProfileStats before;
        unsigned i;
        before=pix86_profile;
        for(i=0;i<32;++i) {
            PROFILE_START(input_start);
            mouse_poll(&mouse);
            screen_update(&screen,&editor,&menu,EDIT_STATUS,&mouse,0);
            PROFILE_STOP(input,input_start);
        }
        report_delta("mouse poll to present x32",&before);
    }
    mouse_shutdown();
    video_shutdown();
    editor_dispose(&editor);
    profile_report();
    return 0;
}
