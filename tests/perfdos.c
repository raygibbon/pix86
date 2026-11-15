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
#include "screen.h"
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
    benchmark_size(sizes[0]);
    benchmark_size(sizes[1]);
    benchmark_size(sizes[2]);
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
