#include <string.h>
#include <stdio.h>
#include "timer.h"
#include "screen.h"
#include "cursor.h"
#include "video.h"
#include "font.h"
#include "theme.h"
#include "workpal.h"
#include "profile.h"
static void status(Screen *s,const Editor *e,const MouseState *mouse,int has_mouse)
{
    char text[UI_STATUS_CHARS+1],part[UI_STATUS_CHARS+1],performance[25],colours[12];
    unsigned i,start,n,oldlen,len;
    PROFILE_COUNT(status_calls);
    if(e->playing) {
        unsigned long render_ms,dropped;
        unsigned fps,target;
        render_ms=s->playback_render_ms>999?999:s->playback_render_ms;
        dropped=s->playback_dropped>9999UL?9999UL:s->playback_dropped;
        fps=s->playback_fps>99?99:s->playback_fps;
        target=(1000U+e->document->duration[e->history.active]/2)/
               e->document->duration[e->history.active];
        if(target>99) target=99;
        sprintf(performance,"R%3lums D%4lu F%2u/%2u",render_ms,dropped,fps,target);
        if(!s->playback_active) memcpy(performance,"R --ms",6);
        else if(!timer_precise() && !render_ms) memcpy(performance,"R<55ms",6);
        memset(text,' ',UI_STATUS_CHARS); text[UI_STATUS_CHARS]='\0';
        memcpy(text,performance,strlen(performance));
        sprintf(colours,"F%03u B%03u",(unsigned)e->foreground,(unsigned)e->background);
        memcpy(text+NAV_RIGHT/FONT_WIDTH,colours,9);
    } else render_status_text(e,mouse->x,mouse->y,has_mouse,s->filename,text);
    len=(unsigned)strlen(text); oldlen=(unsigned)strlen(s->status);
    n=len>oldlen?len:oldlen; i=0;
    while(i<n) {
        if((i<len?text[i]:' ')==(i<oldlen?s->status[i]:' ')) { ++i; continue; }
        start=i;
        do { part[i-start]=i<len?text[i]:' '; ++i; }
        while(i<n && (i<len?text[i]:' ')!=(i<oldlen?s->status[i]:' '));
        part[i-start]='\0';
        cursor_protect((int)start*FONT_WIDTH,UI_STATUS_Y,(int)(i-start)*FONT_WIDTH,FONT_HEIGHT);
        video_text_bg((int)start*FONT_WIDTH,UI_STATUS_Y,part,UI_INK,UI_BAR);
        PROFILE_ADD(status_chars,i-start);
    }
    strcpy(s->status,text);
    if(s->nav_count!=e->document->frames || s->nav_active!=e->history.active) {
        render_nav(e);
        s->nav_count=e->document->frames; s->nav_active=e->history.active;
    }
}
void screen_filename(Screen *s,const char *path)
{
    const char *p,*name;
    name=path;
    for(p=path;*p;++p) if(*p=='\\' || *p=='/' || *p==':') name=p+1;
    strncpy(s->filename,name,12); s->filename[12]='\0';
}
void screen_init(Screen *s,const Editor *e,const MouseState *mouse,int has_mouse)
{
    int i;
    workpal_remember(e->foreground); workpal_remember(e->background);
    for(i=0;i<16;++i) s->working[i]=workpal_colour(i);
    s->flyout.group=s->flyout.hover=-1; s->nav_count=0; s->first_frame=e->first_frame;
    timer_precise();
    s->playback_active=0; s->playback_render_ms=s->playback_fps=0; s->playing=e->playing;
    s->playback_dropped=s->playback_window=s->playback_frames=0;
    s->timeline=e->timeline;
    s->groups[0]=e->group_tool[0]; s->groups[1]=e->group_tool[1];
    s->active_group=e->selection.state && e->group_tool[1]==TOOL_ALL?1:tool_group(e->tool);
    menu_init(&s->menu); s->tool=e->tool; s->alpha=e->transparent_eraser; s->foreground=e->foreground; s->background=e->background;
    s->status[0]=s->filename[0]='\0';
    cursor_hide(); video_unclip(); video_clear(UI_DESK);
    render_ui(e); render_canvas(e); render_timeline(e); menu_draw_bar(&s->menu);
    video_rect(0,UI_STATUS_Y,UI_WIDTH,FONT_HEIGHT,UI_BAR); status(s,e,mouse,has_mouse);
}
static void update(Screen *s,const Editor *e,const Menu *m,const Flyout *flyout,unsigned flags,const MouseState *mouse,int has_mouse)
{
    Rect old,next,parts[4];
    int n,i,active,playback_render,first_playback,timeline_rebuild;
    unsigned long render_start,render_end;
    unsigned long elapsed,now;
    PROFILE_START(screen_start);
    timeline_rebuild=s->nav_count!=e->document->frames ||
                     s->timeline!=e->timeline ||
                     (e->timeline && s->first_frame!=e->first_frame);
    if(s->playing!=e->playing) {
        s->playing=e->playing; s->playback_active=0;
        s->playback_render_ms=s->playback_fps=0;
        s->playback_dropped=e->playback_dropped; s->playback_frames=0;
    }
    if(s->foreground!=e->foreground) workpal_remember(e->foreground);
    if(s->background!=e->background) workpal_remember(e->background);
    if(s->background!=e->background && e->preview_link) flags|=EDIT_CANVAS;
    playback_render=(flags&EDIT_PLAYBACK) &&
                    !(flags&(EDIT_CANVAS|EDIT_STROKE|EDIT_SELECTION));
    first_playback=playback_render && !s->playback_active;
    render_start=playback_render?timer_now():0;
    if(s->flyout.group>=0 && s->flyout.group!=flyout->group) {
        flyout_bounds(s->flyout.group,&old); render_restore(e,&old);
    }
    if(s->menu.active && !m->active) {
        old.x=old.y=0; old.w=UI_WIDTH; old.h=MENU_BAR_HEIGHT; render_restore(e,&old);
    }
    menu_bounds(&s->menu,&old); menu_bounds(m,&next);
    if(s->menu.active && (!m->active || m->heading!=s->menu.heading)) {
        n=rect_subtract(&old,&next,parts);
        for(i=0;i<n;++i) render_restore(e,&parts[i]);
    }
    if(flags&EDIT_UI) render_ui_change(e,s->tool,s->foreground,s->background,s->working,s->alpha);
    active=e->selection.state && e->group_tool[1]==TOOL_ALL?1:tool_group(e->tool);
    for(i=0;i<TOOL_GROUPS;++i)
        if(s->groups[i]!=e->group_tool[i] || s->active_group!=active) render_group(e,i);
    if(playback_render) {
        render_playback_canvas(e,e->playback_previous);
        if(timeline_rebuild) render_timeline(e);
        else render_timeline_playback(e,e->playback_previous);
    } else if(flags&EDIT_CANVAS) {
        if((flags&EDIT_FRAME_SWITCH) && !timeline_rebuild)
            render_playback_canvas(e,s->nav_active);
        else {
            cursor_protect(DRAW_X-1,DRAW_Y-1,DRAW_WIDTH+3,UI_STATUS_Y-DRAW_Y+1);
            render_canvas(e);
        }
    } else if(flags&(EDIT_STROKE|EDIT_CELLS)) {
        render_cells(e);
    } else if(flags&EDIT_SELECTION) {
        cursor_protect(e->view.x+e->dirty_x0*e->view.zoom,e->view.y+e->dirty_y0*e->view.zoom,
                       (e->dirty_x1-e->dirty_x0+1)*e->view.zoom,(e->dirty_y1-e->dirty_y0+1)*e->view.zoom);
        render_stroke(e);
    }
    if(!playback_render) {
        if((flags&EDIT_TIMELINE) && timeline_rebuild) render_timeline(e);
        else if(flags&EDIT_FRAME_SWITCH) render_timeline_playback(e,s->nav_active);
        else if(flags&(EDIT_STROKE|EDIT_CELLS|EDIT_THUMBNAIL))
            render_timeline_thumbnail(e);
        else if(flags&EDIT_TIMELINE) render_timeline(e);
    }
    menu_draw_change(m,&s->menu);
    if(flyout->group>=0 && (s->flyout.group!=flyout->group || s->flyout.hover!=flyout->hover ||
       flags&(EDIT_CANVAS|EDIT_STROKE|EDIT_SELECTION|EDIT_UI))) render_flyout(e,flyout);
    status(s,e,mouse,has_mouse);
    for(i=0;i<16;++i) s->working[i]=workpal_colour(i);
    s->groups[0]=e->group_tool[0]; s->groups[1]=e->group_tool[1]; s->active_group=active; s->flyout=*flyout;
    s->first_frame=e->first_frame; s->timeline=e->timeline;
    s->menu=*m; s->tool=e->tool; s->alpha=e->transparent_eraser; s->foreground=e->foreground; s->background=e->background;
    if(has_mouse) cursor_show(mouse->x,mouse->y);
    if(playback_render) {
        render_end=timer_now();
        s->playback_dropped=e->playback_dropped;
        if(first_playback) {
            s->playback_active=1; s->playback_window=render_start;
            s->playback_frames=0; s->playback_fps=0;
        }
        {
            elapsed=(unsigned long)render_end-(unsigned long)render_start;
            s->playback_render_ms=(unsigned)((elapsed+1192UL)/1193UL);
            now=(unsigned long)render_end;
            ++s->playback_frames;
            elapsed=now-s->playback_window;
            if(elapsed>=TIMER_HZ) {
                s->playback_fps=(unsigned)((s->playback_frames*TIMER_HZ+elapsed/2)/elapsed);
                s->playback_window=now; s->playback_frames=0;
            }
        }
    }
    PROFILE_STOP(screen,screen_start);
}

void screen_update(Screen *s,const Editor *e,const Menu *m,unsigned flags,const MouseState *mouse,int has_mouse)
{
    static const Flyout closed={-1,-1};
    update(s,e,m,&closed,flags,mouse,has_mouse);
}
void screen_present(Screen *s,const Editor *e,const UiState *ui,unsigned flags,const MouseState *mouse,int has_mouse)
{ update(s,e,&ui->menu,&ui->flyout,flags,mouse,has_mouse); }
