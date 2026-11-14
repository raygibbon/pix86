#include <string.h>
#include "screen.h"
#include "cursor.h"
#include "video.h"
#include "font.h"
#include "theme.h"
static void status(Screen *s,const Editor *e,const Menu *m,const MouseState *mouse,int has_mouse)
{
    char text[UI_STATUS_CHARS+1],part[UI_STATUS_CHARS+1];
    unsigned i,start,n,oldlen,len;
    if(m->active) strcpy(text,"Arrows move  Enter choose  Esc/Ctrl+\\ close");
    else render_status_text(e,mouse->x,mouse->y,has_mouse,s->filename,text);
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
    }
    strcpy(s->status,text);
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
    menu_init(&s->menu); s->tool=e->tool; s->foreground=e->foreground; s->background=e->background;
    s->status[0]=s->filename[0]='\0';
    cursor_hide(); video_unclip(); video_clear(UI_DESK);
    render_ui(e); render_canvas(e); menu_draw_bar(&s->menu);
    video_rect(0,UI_STATUS_Y,UI_WIDTH,FONT_HEIGHT,UI_BAR); status(s,e,&s->menu,mouse,has_mouse);
}
void screen_update(Screen *s,const Editor *e,const Menu *m,unsigned flags,const MouseState *mouse,int has_mouse)
{
    Rect old,next,parts[4];
    int n,i;
    menu_bounds(&s->menu,&old); menu_bounds(m,&next);
    if(s->menu.active && (!m->active || m->heading!=s->menu.heading)) {
        n=rect_subtract(&old,&next,parts);
        for(i=0;i<n;++i) render_restore(e,&parts[i]);
    }
    if(flags&EDIT_UI) render_ui_change(e,s->tool,s->foreground,s->background);
    if(flags&EDIT_CANVAS) {
        cursor_protect(DRAW_X-1,DRAW_Y-1,DRAW_WIDTH+2,DRAW_HEIGHT+2); render_canvas(e);
    } else if(flags&EDIT_STROKE) {
        cursor_protect(e->view.x+e->dirty_x0*e->view.zoom,e->view.y+e->dirty_y0*e->view.zoom,
                       (e->dirty_x1-e->dirty_x0+1)*e->view.zoom,(e->dirty_y1-e->dirty_y0+1)*e->view.zoom);
        render_stroke(e);
    }
    menu_draw_change(m,&s->menu);
    status(s,e,m,mouse,has_mouse);
    s->menu=*m; s->tool=e->tool; s->foreground=e->foreground; s->background=e->background;
    if(has_mouse) cursor_show(mouse->x,mouse->y);
}
