#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "screen.h"
#include "dialog.h"
#include "keyboard.h"
#include "cursor.h"
#include "video.h"
#include "font.h"
#include "theme.h"
#include "ui.h"
#include "command.h"
#include "workpal.h"
static Editor editor;
static unsigned short queue[4096];
static unsigned char pixels[64000],baseline[64000];
static Rect clip={0,0,320,200},watch;
static unsigned long writes,reads,outside;
static unsigned clears,black_boxes,interior_fills;
static int watching,mode,shutdowns,initializations;
static Rect dialog_box;
typedef struct Event { int key,x,y; unsigned buttons; } Event;
static Event events[256];
static int count,current,checking_idle;
static unsigned long last_writes;
static void (*onpoll)(void);
int pix86_run(int argc,char **argv);
static int contains(const Rect *r,int x,int y)
{ return x>=r->x && y>=r->y && x<r->x+r->w && y<r->y+r->h; }
void video_clip(int x,int y,int w,int h) { clip.x=x;clip.y=y;clip.w=w;clip.h=h; }
void video_unclip(void) { video_clip(0,0,320,200); }
void video_pixel(int x,int y,unsigned char colour)
{
    if(x<0 || y<0 || x>=320 || y>=200 || !contains(&clip,x,y)) return;
    if(watching && !contains(&watch,x,y)) ++outside;
    pixels[y*320+x]=colour; ++writes;
}
unsigned char video_read(int x,int y)
{
    ++reads;
    return x>=0 && y>=0 && x<320 && y<200?pixels[y*320+x]:0;
}
void video_rect(int x,int y,int w,int h,unsigned char colour)
{
    int xx,yy;
    if(x==0 && y==0 && w==320 && h==200) ++clears;
    if(w>100 && h>20 && colour==UI_INK) ++black_boxes;
    if(x==dialog_box.x+1 && y==dialog_box.y+1 && w==dialog_box.w-2 && h==dialog_box.h-2) ++interior_fills;
    for(yy=y;yy<y+h;++yy) for(xx=x;xx<x+w;++xx) video_pixel(xx,yy,colour);
}
void video_clear(unsigned char colour) { video_rect(0,0,320,200,colour); }
int video_init(void) { assert(!mode); mode=1; ++initializations; return 1; }
void video_shutdown(void) { assert(mode && current==count-1); mode=0; ++shutdowns; }
void delay(unsigned ms) { (void)ms; }
int mouse_init(void) { return 1; }
void mouse_shutdown(void) { }
void mouse_poll(MouseState *m)
{
    m->x=current<0?0:events[current].x; m->y=current<0?0:events[current].y;
    m->buttons=current<0?0:events[current].buttons;
}
int keyboard_poll(void)
{
    if(checking_idle && current>=0 && (events[current].key==-1 || events[current].key==KEY_F2))
        assert(last_writes==writes);
    if(onpoll) onpoll();
    last_writes=writes;
    assert(++current<count); return events[current].key;
}
static void start(void)
{
    cursor_hide(); count=0; current=-1; checking_idle=0; onpoll=NULL;
    watching=0; outside=0; writes=reads=0; clears=black_boxes=interior_fills=0;
}
static void add(int key)
{
    assert(count<256); events[count].key=key; events[count].x=events[count].y=0;
    events[count++].buttons=0;
}
static void mouse_event(int x,int y,unsigned buttons)
{ add(-1); events[count-1].x=x; events[count-1].y=y; events[count-1].buttons=buttons; }
static void clean_baseline(Screen *s,Menu *m,MouseState *mouse)
{
    editor_dispose(&editor);
    assert(editor_init(&editor,16,queue));
    /* High contrast artwork makes erased/mis-restored canvas pixels visible. */
    memset(editor.pixels,231,256);
    menu_init(m); mouse->x=mouse->y=0; mouse->buttons=0;
    screen_init(s,&editor,mouse,0);
    memcpy(baseline,pixels,sizeof(pixels)); clears=0;
}
static void geometry(void)
{
    DialogLayout l;
    Rect a,b,out[4];
    int kind,n,i,x,y,h,row,sum;
    char longtext[160];
    memset(longtext,'X',159); longtext[159]='\0';
    for(kind=0;kind<4;++kind) {
        dialog_layout(&l,kind,"Example",kind==DIALOG_FILE?DIALOG_FILE_HINT:longtext);
        assert(l.box.x*2+l.box.w==320 && l.box.y*2+l.box.h==200);
        assert(l.box.x>=0 && l.box.y>=0 && l.box.w<=248 && l.box.h<150);
        assert(l.accept.y+l.accept.h<l.box.y+l.box.h);
        assert(l.accept.x+l.accept.w<l.cancel.x && l.cancel.x+l.cancel.w<l.box.x+l.box.w);
        assert(l.body.x+l.body.w<l.box.x+l.box.w);
    }
    for(h=0;h<MENU_COUNT;++h) {
        for(row=0;row<menu_items(h);++row) {
            const MenuItem *item=menu_item(h,row);
            assert(font_width(item->label)+font_width(item->shortcut)+8<=menu_drop_width(h));
        }
        assert(menu_left(h)+menu_drop_width(h)<=320);
    }
    a.x=10;a.y=20;a.w=40;a.h=30;b.x=20;b.y=10;b.w=20;b.h=30;
    n=rect_subtract(&a,&b,out); assert(n<=4);
    for(y=0;y<80;++y) for(x=0;x<80;++x) {
        sum=0;for(i=0;i<n;++i) sum+=contains(&out[i],x,y);
        assert(sum==(contains(&a,x,y) && !contains(&b,x,y)));
    }
}
static void menus(void)
{
    Screen s;Menu m;MouseState mouse;
    int key,h,i,command;
    unsigned long before;
    start(); clean_baseline(&s,&m,&mouse);
    for(h=0;h<MENU_COUNT;++h) {
        menu_key(&m,KEY_MENU,&command); /* opens File */
        for(i=0;i<h;++i) menu_key(&m,KEY_RIGHT,&command);
        screen_update(&s,&editor,&m,0,&mouse,0);
        before=writes;
        for(i=0;i<50;++i) screen_update(&s,&editor,&m,EDIT_STATUS,&mouse,0);
        assert(before==writes);
        for(i=0;i<menu_items(h);++i) {
            int olditem;
            olditem=m.item; before=writes;
            menu_bounds(&m,&watch); ++watch.x; ++watch.y; watch.w-=2; watch.h-=2;
            watching=1; outside=0;
            menu_key(&m,KEY_DOWN,&command);
            screen_update(&s,&editor,&m,0,&mouse,0);
            watching=0; assert(!outside);
            assert(writes-before==(olditem==m.item?0UL:2UL*(unsigned)(watch.w)*MENU_ROW_HEIGHT));
        }
        for(i=0;i<MENU_COUNT;++i) {
            menu_key(&m,KEY_RIGHT,&command); screen_update(&s,&editor,&m,0,&mouse,0);
        }
        key=27; menu_key(&m,key,&command); assert(!command && !m.active);
        screen_update(&s,&editor,&m,0,&mouse,0);
        assert(!memcmp(pixels,baseline,sizeof(pixels)));
        assert(clears==0 && black_boxes==0);
    }
    before=writes;
    for(i=0;i<100;++i) screen_update(&s,&editor,&m,EDIT_STATUS,&mouse,0);
    assert(before==writes);
    /* An unchanged cursor neither reads nor rewrites VGA. */
    cursor_show(310,180); before=writes; reads=0;
    cursor_show(310,180); cursor_protect(0,0,100,20);
    assert(before==writes && reads==0);
    cursor_hide();
}
static void field_poll(void)
{
    if(current<0) {
        watching=1; outside=0;
    } else assert(outside==0);
}
static void dialogs(void)
{
    Screen s;Menu m;MouseState mouse;
    DialogLayout l;char output[FILE_PATH_MAX];
    start();clean_baseline(&s,&m,&mouse); dialog_mouse(0);dialog_editor(&editor);
    dialog_layout(&l,DIALOG_FILE,"Save",DIALOG_FILE_HINT); dialog_box=l.box; watch=l.field;
    add(-1);add(-1);add(KEY_F2);add('Z');add(KEY_LEFT);add(13);
    checking_idle=1;onpoll=field_poll;
    assert(dialog_filename("Save","OLD.P86",output));
    watching=0;onpoll=NULL;
    assert(!strcmp(output,"Z") && interior_fills==1 && black_boxes==0 && clears==0);
    assert(!memcmp(pixels,baseline,sizeof(pixels)));
    start();dialog_layout(&l,DIALOG_CONFIRM,"Unsaved","Discard?");dialog_box=l.box;
    add(-1);add(-1);add(KEY_F2);add(9);add(9);add(27);checking_idle=1;
    assert(!dialog_confirm("Unsaved","Discard?"));
    assert(interior_fills==1 && !black_boxes && !clears);
    assert(!memcmp(pixels,baseline,sizeof(pixels)));
}
static void palette_poll(void)
{
    int i,x,y;
    if(current>=0) return;
    /* Check all 256 actual rendered colour interiors before consuming input. */
    for(i=0;i<256;++i) {
        x=FULLPAL_X+i%16*FULLPAL_CELL_W+4;
        y=FULLPAL_Y+i/16*FULLPAL_CELL_H+3;
        assert(pixels[y*320+x]==i);
    }
}
static void palette_overlay(void)
{
    Screen s; Menu m; MouseState mouse;
    unsigned char colour; int background;
    unsigned long before;
    int i;
    start();workpal_init(); clean_baseline(&s,&m,&mouse);
    dialog_editor(&editor);dialog_mouse(0); colour=15;background=0;
    add(-1);add(-1);add(KEY_F2);add(KEY_LEFT);add(KEY_RIGHT);add(9);add(KEY_SHIFT_TAB);add(27);
    onpoll=palette_poll;checking_idle=1;
    assert(!dialog_palette(&colour,&background));onpoll=NULL;
    assert(colour==15 && background==0 && !clears && !black_boxes);
    assert(!memcmp(pixels,baseline,sizeof(pixels)));
    start();add(13);dialog_font();
    assert(!memcmp(pixels,baseline,sizeof(pixels)) && !clears);
    for(i=0;i<16;++i) assert(workpal_colour(i)==s.working[i]);
    editor.foreground=255;editor.background=232;
    screen_update(&s,&editor,&m,EDIT_UI,&mouse,0);
    assert(workpal_colour(14)==255 && workpal_colour(15)==232);
    for(i=0;i<16;++i) {
        int x,y;
        x=PALETTE_X+i%PALETTE_COLS*PALETTE_CELL_W;
        y=PALETTE_Y+i/PALETTE_COLS*PALETTE_CELL_H;
        assert(pixels[(y+1)*320+x+1]==workpal_colour(i));
    }
    for(i=0;i<4;++i) {
        int y;
        editor.tool=i;screen_update(&s,&editor,&m,EDIT_UI,&mouse,0);
        y=TOOL_Y+i*TOOL_STEP;
        assert(pixels[y*320+TOOL_X]==UI_PAPER);
        if(i) assert(pixels[(y-TOOL_STEP)*320+TOOL_X]==UI_PANEL);
    }
    before=writes;
    screen_update(&s,&editor,&m,EDIT_UI,&mouse,0); assert(before==writes);
    assert(!clears);
}
static void routing_status(void)
{
    UiState ui; MouseState mouse;
    Screen s; Menu m; char text[UI_STATUS_CHARS+1];
    int command;
    start(); clean_baseline(&s,&m,&mouse); ui_init(&ui);
    assert(keyboard_command('U')==CMD_UNDO && keyboard_command('r')==CMD_REDO);
    assert(keyboard_command('N')==CMD_NEW && keyboard_command('=')==CMD_ZOOM_IN);
    assert(keyboard_command(KEY_DELETE)==CMD_FRAME_DELETE && keyboard_command(-1)==CMD_NONE);
    assert(ui_input(&ui,KEY_MENU,&mouse,&command) && !command && ui.menu.active);
    assert(ui_input(&ui,CMD_EXIT,&mouse,&command) && !command && !ui.menu.active);
    assert(!ui_pointer(&ui,&mouse,1,1));
    assert(!ui_input(&ui,'P',&mouse,&command) && command==CMD_PENCIL);
    assert(ui_pointer(&ui,&mouse,0,0));
    mouse.buttons=1; ui_resume(&ui,&mouse);
    assert(!ui_pointer(&ui,&mouse,0,0));
    mouse.buttons=0; assert(!ui_pointer(&ui,&mouse,0,0));
    assert(ui_pointer(&ui,&mouse,0,0));
    ui_input(&ui,-1,&mouse,&command); /* poll the dismissal release */
    mouse.x=PALETTE_X;mouse.y=PALETTE_MORE_Y;mouse.buttons=1;
    assert(ui_input(&ui,-1,&mouse,&command) && command==CMD_PALETTE);
    mouse.buttons=0;ui_input(&ui,-1,&mouse,&command);
    mouse.buttons=2;
    assert(ui_input(&ui,-1,&mouse,&command) && command==CMD_BG);
    mouse.buttons=0;
    screen_filename(&s,"C:\\ART\\LONGNAME.P86");
    assert(!strcmp(s.filename,"LONGNAME.P86"));
    render_status_text(&editor,editor.view.x,editor.view.y,1,s.filename,text);
    assert(strlen(text)<=UI_STATUS_CHARS && !strncmp(text,"LONGNAME.P86",12));
    assert(strstr(text,"16x16") && strstr(text,"X00Y00") && strstr(text,"Pen"));
    assert(strstr(text,"F015") && strstr(text,"B000") && strstr(text,"Z11"));
    screen_update(&s,&editor,&m,EDIT_STATUS,&mouse,0);
    assert(strstr(s.status,"LONGNAME.P86"));
    screen_filename(&s,""); screen_update(&s,&editor,&m,EDIT_STATUS,&mouse,0);
    assert(strstr(s.status,"UNTITLED"));
}
static void timeline(void)
{
    static unsigned char closed[64000];
    Screen s; Menu m; MouseState mouse; UiState ui;
    unsigned long before;
    unsigned flags;
    int command,h;
    start(); clean_baseline(&s,&m,&mouse); ui_init(&ui);
    memcpy(closed,pixels,sizeof(pixels));
    flags=editor_key(&editor,CMD_TIMELINE);
    screen_update(&s,&editor,&m,flags,&mouse,0);
    assert(editor.view.y+(int)editor.view.height*editor.view.zoom<TIMELINE_Y);
    flags=editor_key(&editor,CMD_TIMELINE);
    screen_update(&s,&editor,&m,flags,&mouse,0);
    assert(!memcmp(pixels,closed,sizeof(pixels)));
    flags=editor_key(&editor,CMD_TIMELINE);
    screen_update(&s,&editor,&m,flags,&mouse,0);
    memcpy(baseline,pixels,sizeof(pixels)); before=writes;
    screen_update(&s,&editor,&m,0,&mouse,0); assert(writes==before);
    for(h=0;h<MENU_COUNT;++h) {
        m.active=1; m.heading=h; m.item=0;
        screen_update(&s,&editor,&m,0,&mouse,0);
        m.active=0; screen_update(&s,&editor,&m,0,&mouse,0);
        assert(!memcmp(pixels,baseline,sizeof(pixels)));
    }
    start(); add(27); dialog_editor(&editor); dialog_font();
    assert(!memcmp(pixels,baseline,sizeof(pixels)));
    mouse.x=DRAW_X+2*30; mouse.y=TIMELINE_Y+4; mouse.buttons=1;
    assert(ui_timeline(&ui,1,0,1,&mouse,&command) && command==CMD_FRAME_ADD);
    flags=editor_key(&editor,command); assert(editor.document->frames==2);
    assert(!ui_pointer(&ui,&mouse,1,0));
    mouse.x=editor.view.x; mouse.y=editor.view.y;
    assert(!ui_pointer(&ui,&mouse,0,0));
    mouse.buttons=0; assert(!ui_pointer(&ui,&mouse,0,0));
    assert(ui_pointer(&ui,&mouse,0,0));
    assert(editor.pixels[0]==0);
    mouse.x=DRAW_X+2; mouse.y=TIMELINE_Y+20; mouse.buttons=1;
    assert(ui_timeline(&ui,1,0,2,&mouse,&command) && command==CMD_SELECT_FRAME);
    flags=editor_key(&editor,command);
    screen_update(&s,&editor,&m,flags,&mouse,0);
    assert(editor.pixels[0]==231);
    assert(!clears);
}
static void export_overlay(void)
{
    Screen s; Menu m; MouseState mouse;
    ExportOptions o;
    char path[FILE_PATH_MAX];
    start(); clean_baseline(&s,&m,&mouse);
    memset(&o,0,sizeof(o));
    o.mode=EXPORT_CURRENT; o.layout=SHEET_HORIZONTAL; o.columns=4;
    dialog_editor(&editor);
    checking_idle=1;
    add(-1); add(-1); add(KEY_F2); add(-1); add(27);
    assert(!dialog_export("PLAYER.BMP",path,&o));
    assert(!memcmp(pixels,baseline,sizeof(pixels)) && !clears);
}
static void type(const char *text) { while(*text) add((unsigned char)*text++); }
static void run(int mouse)
{
    char *argv[4];
    argv[0]="PIX86"; argv[1]="16"; argv[2]="/NOMOUSE";argv[3]=NULL;
    initializations=shutdowns=0;
    assert(pix86_run(mouse?2:3,argv)==0);
    assert(initializations==1 && shutdowns==1 && clears==1 && !mode);
}
static void application(void)
{
    FILE *f;P86Image image;
    start();add(KEY_MENU);add(KEY_DOWN);add(27);add(KEY_F2);add(27);
    add('n');add(27);add(KEY_F3);add(27);add(KEY_MENU);add(KEY_RIGHT);add(13);add(27);
    run(0);
    start();add(CMD_PALETTE);add(KEY_END);add(13);add(KEY_MENU);add(27);
    add(KEY_F2);add(27);add(27);add(13); /* default cancel dirty Exit */
    add('n');add(27);add(KEY_F3);add(27);add(27);add('y');run(1);
    remove("PAINT.P86");
    start();add(KEY_F2);type("PAINT");add(13);add(13);
    add(CMD_PALETTE);add(KEY_END);add(13);
    mouse_event(108,30,1);mouse_event(108,30,0);
    add(KEY_F2);add('y');add(13);
    add('n');add('3');add(KEY_F3);type("PAINT");add(13);add(13);add(27);run(1);
    f=fopen("PAINT.P86","rb");assert(f);assert(file_decode(f,&image)==FILE_OK);fclose(f);
    assert(image.width==16 && image.foreground==255 && image.pixels[17]==255);
    remove("PAINT.P86");
    remove("OUT.BMP");
    start(); add(CMD_EXPORT); add(27); add(27); run(1);
    assert(file_exists("OUT.BMP")==0);
    start(); add(CMD_EXPORT); type("OUT"); add(13); add(13);
    /* Existing-file overwrite defaults to Cancel and remains clean. */
    add(CMD_EXPORT); type("OUT"); add(13); add(13);
    add(CMD_EXPORT); type("OUT"); add(13); add('y'); add(13); add(27); run(1);
    f=fopen("OUT.BMP","rb"); assert(f!=NULL);
    assert(fgetc(f)=='B' && fgetc(f)=='M'); fclose(f); remove("OUT.BMP");
}
int main(void)
{
    geometry(); menus(); dialogs(); palette_overlay(); routing_status(); timeline(); export_overlay(); application();
    puts("PASS: dirty rectangles, idle stability, exact overlay/cursor restoration, one startup clear and deliberate exits/file workflow.");
    editor_dispose(&editor);
    return 0;
}
