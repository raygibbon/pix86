#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "screen.h"
#include "video.h"
#include "cursor.h"
#include "theme.h"
static Editor e;
static Screen s;
static UiState ui;
static MouseState mouse;
static unsigned short queue[4096];
static unsigned char pixels[640*480],baseline[640*480],indices[4096],mask[512];
static Rect clip;
static unsigned long writes,reads;
static unsigned clears;
static Rect watched[2];
static int watching;
static unsigned long outside;
static int contains(const Rect *r,int x,int y)
{ return x>=r->x && y>=r->y && x<r->x+r->w && y<r->y+r->h; }
void video_clip(int x,int y,int w,int h) { clip.x=x; clip.y=y; clip.w=w; clip.h=h; }
void video_unclip(void) { video_clip(0,0,UI_WIDTH,UI_HEIGHT); }
void video_pixel(int x,int y,unsigned char c)
{
    if(x<clip.x || y<clip.y || x>=clip.x+clip.w || y>=clip.y+clip.h) return;
    assert(x>=0 && x<UI_WIDTH && y>=0 && y<UI_HEIGHT);
    if(watching && !contains(&watched[0],x,y) && !contains(&watched[1],x,y)) ++outside;
    pixels[y*UI_WIDTH+x]=c; ++writes;
}
unsigned char video_read(int x,int y)
{
    ++reads;
    return x<0 || y<0 || x>=UI_WIDTH || y>=UI_HEIGHT?0:pixels[y*UI_WIDTH+x];
}
void video_rect(int x,int y,int w,int h,unsigned char c)
{
    int xx,yy;
    for(yy=y;yy<y+h;++yy) for(xx=x;xx<x+w;++xx) video_pixel(xx,yy,c);
}
void video_clear(unsigned char c) { ++clears; video_rect(0,0,UI_WIDTH,UI_HEIGHT,c); }
static unsigned event(int key,int x,int y,unsigned buttons)
{
    int command,consumed,was_active;
    unsigned flags;
    mouse.x=x; mouse.y=y; mouse.buttons=buttons; was_active=ui.menu.active;
    consumed=ui_input(&ui,&e,key,&mouse,&command); flags=0;
    if(consumed) editor_finish(&e);
    screen_present(&s,&e,&ui,0,&mouse,0);
    if(command>0) flags=editor_key(&e,command);
    if(ui_pointer(&ui,&mouse,consumed,was_active)) flags|=editor_mouse(&e,x,y,buttons);
    else e.previous_buttons=buttons;
    screen_present(&s,&e,&ui,flags,&mouse,0); return flags;
}
static void fresh(unsigned w,unsigned h)
{
    unsigned i;
    cursor_hide(); e.timeline=0; assert(editor_new_document(&e,w,h,1,141));
    for(i=0;i<w*h;++i) e.pixels[i]=(unsigned char)i;
    memset(e.mask,0,w*h/8); e.mask[0]=240;
    history_init(&e.history,&e.canvas); editor_mark_saved(&e);
    e.tool=TOOL_PENCIL; e.group_tool[0]=0; e.group_tool[1]=4;
    e.grid=1; e.guide=3; ui_init(&ui); mouse.x=mouse.y=0; mouse.buttons=0;
    screen_init(&s,&e,&mouse,0); screen_filename(&s,"CHECK.P86");
    screen_present(&s,&e,&ui,0,&mouse,0);
    memcpy(indices,e.pixels,w*h); memcpy(mask,e.mask,w*h/8);
    assert(!ui.menu.active && ui.flyout.group<0 && !e.timeline); clears=0;
}
static void snapshot(void) { memcpy(baseline,pixels,(unsigned long)UI_WIDTH*UI_HEIGHT); }
static void exact(void)
{
    assert(!memcmp(baseline,pixels,(unsigned long)UI_WIDTH*UI_HEIGHT));
    assert(!memcmp(indices,e.pixels,e.history.size) && !memcmp(mask,e.mask,e.history.size/8));
    assert(!editor_is_dirty(&e) && !clears);
}
static void open_group(int group)
{
    int x,y;
    x=TOOL_X+2; y=TOOL_Y+group*TOOL_STEP+2;
    flyout_bounds(group,&watched[0]); watched[1].w=watched[1].h=0;
    watching=!ui.menu.active && ui.flyout.group<0; outside=0;
    event(-1,x,y,2); event(-1,x,y,0); assert(ui.flyout.group==group);
    watching=0; assert(!outside);
}
static void overlays(void)
{
    int h,i,x,y;
    Rect r;
    unsigned long before;
    View view;
    fresh(16,16); assert(e.view.y<MENU_BAR_HEIGHT); snapshot(); view=e.view;
    for(h=0;h<MENU_COUNT;++h) {
        watched[0].x=watched[0].y=0; watched[0].w=UI_WIDTH; watched[0].h=MENU_BAR_HEIGHT;
        { Menu next=ui.menu; next.active=1; next.heading=0; menu_bounds(&next,&watched[1]); }
        watching=1; outside=0; event(KEY_MENU,0,0,0); watching=0; assert(!outside);
        for(i=0;i<h;++i) event(KEY_RIGHT,0,0,0);
        assert(ui.menu.active && ui.menu.heading==h);
        before=writes;
        for(i=0;i<20;++i) event(-1,0,0,0);
        assert(before==writes);
        menu_bounds(&ui.menu,&watched[1]); watching=1; outside=0;
        event(27,0,0,0); watching=0; assert(!outside); exact(); assert(!memcmp(&view,&e.view,sizeof(view)));
    }
    event(KEY_F10,0,0,0); assert(ui.menu.heading==MENU_COUNT-1);
    event(KEY_LEFT,0,0,0); event(27,0,0,0); exact();
    event(KEY_ALT_E,0,0,0); event(27,0,0,0); exact();
    event(KEY_MENU,0,0,0);
    event(-1,e.view.x,e.view.y+10*e.view.zoom,1);
    event(-1,e.view.x+e.view.zoom,e.view.y+10*e.view.zoom,1);
    event(-1,e.view.x+e.view.zoom,e.view.y+10*e.view.zoom,0); exact();
    /* Top-row artwork remains ordinary drawing input when menus are hidden. */
    assert(pixels[e.view.y*UI_WIDTH+e.view.x]==e.pixels[0]);
    event(-1,e.view.x,e.view.y,1); event(-1,e.view.x,e.view.y,0);
    assert(!ui.menu.active && e.pixels[0]==e.foreground && e.history.position==1);
    event(CMD_UNDO,0,0,0); exact();
    x=UI_RIGHT_X+2; y=UI_STATUS_Y-20; cursor_show(x,y); reads=0;
    event(KEY_MENU,x,y,0); event(27,x,y,0); assert(!reads); cursor_hide(); exact();
    fresh(32,24); snapshot(); view=e.view;
    for(h=0;h<TOOL_GROUPS;++h) {
        open_group(h); before=writes;
        for(i=0;i<20;++i) event(-1,0,0,0);
        assert(before==writes);
        flyout_bounds(h,&r); before=writes;
        event(-1,r.x+2,r.y+2,0); assert(writes>before && ui.flyout.hover==0);
        watched[0]=r; watched[1].w=watched[1].h=0; watching=1; outside=0;
        event(27,0,0,0); watching=0; assert(!outside); exact(); assert(!memcmp(&view,&e.view,sizeof(view)));
        open_group(h);
        event(-1,e.view.x,e.view.y,1); assert(ui.flyout.group<0);
        event(-1,e.view.x+e.view.zoom,e.view.y,1);
        event(-1,e.view.x+e.view.zoom,e.view.y,0); exact();
    }
    /* Grid, masks, guides, marquee and floating preview restore together. */
    event(CMD_SELECT_ALL,0,0,0); event(CMD_MOVE,0,0,0); event(KEY_RIGHT,0,0,0);
    assert(selection_floating(&e)); snapshot();
    open_group(1); event(27,0,0,0); exact(); assert(e.selection.dx==1);
    open_group(1); event(KEY_MENU,0,0,0);
    assert(ui.menu.active && ui.flyout.group<0); event(27,0,0,0); exact();
    event(KEY_MENU,0,0,0); open_group(1);
    assert(!ui.menu.active); event(27,0,0,0); exact();
    open_group(1); { Rect r; flyout_bounds(1,&r);
        event(-1,r.x+2+2*FLY_CELL,r.y+2,1); event(-1,0,0,0);
    }
    exact(); assert(selection_floating(&e) && e.selection.dx==1);
    event(CMD_EXIT,0,0,0); assert(!selection_floating(&e));
    /* The optional expanded view retains identical overlay restoration. */
    event(CMD_TIMELINE,0,0,0); snapshot();
    event(KEY_MENU,0,0,0); event(27,0,0,0); exact();
    open_group(0); event(27,0,0,0); exact();
}
static void frames(void)
{
    int i,x,previous;
    unsigned first;
    fresh(32,24);
    for(i=1;i<16;++i) editor_key(&e,CMD_FRAME_ADD);
    editor_mark_saved(&e); memcpy(indices,e.pixels,e.history.size); memcpy(mask,e.mask,e.history.size/8);
    screen_present(&s,&e,&ui,EDIT_CANVAS|EDIT_TIMELINE,&mouse,0);
    previous=e.history.position;
    for(i=0;i<16;++i) {
        editor_select(&e,(unsigned)i); screen_present(&s,&e,&ui,EDIT_CANVAS,&mouse,0);
        first=nav_first((unsigned)i,16);
        x=NAV_X+NAV_ARROW+(i-(int)first)*NAV_CELL;
        assert(x+2*FONT_WIDTH<=NAV_RIGHT-NAV_ARROW);
        assert(pixels[UI_STATUS_Y*UI_WIDTH+x+FONT_WIDTH-1]==UI_SELECT);
        assert(e.history.position==(unsigned)previous && !e.timeline);
        memcpy(indices,e.pixels,e.history.size); memcpy(mask,e.mask,e.history.size/8);
        snapshot(); event(KEY_MENU,0,0,0); event(27,0,0,0); exact();
    }
    event(-1,NAV_X,UI_STATUS_Y,1); event(-1,NAV_X,UI_STATUS_Y,0);
    assert(e.history.active==14);
    first=nav_first(14,16); x=NAV_X+NAV_ARROW;
    event(-1,x,UI_STATUS_Y,1); event(-1,x,UI_STATUS_Y,0);
    assert(e.history.active==first && e.history.position==(unsigned)previous);
}
int main(void)
{
    int mode;
    assert(editor_init(&e,16,queue));
    for(mode=0;mode<2;++mode) {
        layout_set(mode?640:320,mode?480:200); video_unclip(); overlays(); frames();
    }
    editor_dispose(&e);
    puts("PASS: hidden menus, precise overlay restoration, flyout ownership, floating selection and compact frames in both modes.");
    return 0;
}
