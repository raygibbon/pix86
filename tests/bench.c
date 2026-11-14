#include <dos.h>
#include <stdio.h>
#include <string.h>
#include "dialog.h"
#include "editor.h"
#include "keyboard.h"
#include "layout.h"
#include "menu.h"
#include "mouse.h"
#include "screen.h"
#include "video.h"

static Editor editor;
static Screen screen;
static unsigned short fill_queue[4096];

static unsigned long bios_ticks(void)
{
    union REGS r;
    r.h.ah=0;
    int86(0x1a,&r,&r);
    return ((unsigned long)r.w.cx<<16)|r.w.dx;
}

static void report(const char *name,unsigned long start,unsigned runs)
{
    unsigned long elapsed=bios_ticks()-start;
    printf("%s: %lu BIOS ticks / %u runs (~%lu ms per run)\n",
           name,elapsed,runs,(elapsed*55UL)/runs);
}

int keyboard_poll(void) { return 13; }
void mouse_poll(MouseState *mouse) { mouse->x=mouse->y=0;mouse->buttons=0; }
void delay(unsigned ms) { (void)ms; }

static int benchmark(int hires)
{
    MouseState mouse;
    unsigned long start;
    unsigned i,flags;
    int command,zoom,x0,y0,x1,y1;
    unsigned char colour;
    int background;
    memset(&mouse,0,sizeof(mouse));
    if(!video_init(hires)) {
        puts("BENCH: video initialization failed.");
        return 1;
    }
    layout_set(video_width(),video_height(),video_hires());
    if(!editor_init(&editor,32,fill_queue)) {
        video_shutdown();
        puts("BENCH: editor initialization failed.");
        return 1;
    }
    view_set(&editor.view,&editor.canvas,view_max_zoom(&editor.canvas));
    dialog_editor(&editor);
    dialog_mouse(0);
    screen_init(&screen,&editor,&mouse,0);
    printf("Display: %dx%d (%s)\n",video_width(),video_height(),
           video_hires()?"VBE 101h":"Mode 13h");

    start=bios_ticks();
    for(i=0;i<5;++i) screen_init(&screen,&editor,&mouse,0);
    report("Initial screen render",start,5);

    start=bios_ticks();
    for(i=0;i<20;++i) {
        menu_key(&screen.menu,KEY_MENU,&command);
        screen_update(&screen,&editor,&screen.menu,0,&mouse,0);
        menu_key(&screen.menu,KEY_MENU,&command);
        screen_update(&screen,&editor,&screen.menu,0,&mouse,0);
    }
    report("Menu open and close",start,20);

    menu_key(&screen.menu,KEY_MENU,&command);
    screen_update(&screen,&editor,&screen.menu,0,&mouse,0);
    start=bios_ticks();
    for(i=0;i<100;++i) {
        menu_key(&screen.menu,KEY_DOWN,&command);
        screen_update(&screen,&editor,&screen.menu,0,&mouse,0);
    }
    report("Menu navigation",start,100);
    menu_key(&screen.menu,27,&command);
    screen_update(&screen,&editor,&screen.menu,0,&mouse,0);

    start=bios_ticks();
    for(i=0;i<10;++i)
        dialog_message("Benchmark","Dialog open and restore",NULL);
    report("Dialog open and restore",start,10);

    start=bios_ticks();
    for(i=0;i<20;++i) {
        zoom=(i&1)?1:view_max_zoom(&editor.canvas);
        view_set(&editor.view,&editor.canvas,zoom);
        screen_update(&screen,&editor,&screen.menu,EDIT_CANVAS,&mouse,0);
    }
    report("Canvas zoom redraw",start,20);

    x0=editor.view.x;
    y0=editor.view.y;
    x1=x0+15*editor.view.zoom;
    y1=y0+10*editor.view.zoom;
    start=bios_ticks();
    for(i=0;i<10;++i) {
        flags=editor_mouse(&editor,x0,y0,1);
        screen_update(&screen,&editor,&screen.menu,flags,&mouse,0);
        flags=editor_mouse(&editor,x1,y1,1);
        screen_update(&screen,&editor,&screen.menu,flags,&mouse,0);
        flags=editor_mouse(&editor,x1,y1,0);
        screen_update(&screen,&editor,&screen.menu,flags,&mouse,0);
    }
    report("Continuous drawing (10 strokes)",start,10);

    start=bios_ticks();
    for(i=0;i<10;++i) {
        colour=15;
        background=0;
        if(!dialog_palette(&colour,&background)) {
            puts("BENCH: palette dialog did not accept.");
            break;
        }
    }
    report("Full palette popup",start,10);

    if(video_failed()) puts("BENCH: video bank access failed.");
    video_shutdown();
    return video_failed()?1:0;
}

int main(int argc,char **argv)
{
    int hires=argc>1 && (strcmp(argv[1],"/640")==0 ||
                         strcmp(argv[1],"/HIRES")==0);
    return benchmark(hires);
}
