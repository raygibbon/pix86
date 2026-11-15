#include <string.h>
#include "editor.h"
#include "command.h"
#include "select.h"

void selection_reset(Editor *e) { memset(&e->selection,0,sizeof(e->selection)); }
int selection_floating(const Editor *e) { return e->selection.state>=SEL_FLOAT; }
static int clamp(int n,int low,int high) { return n<low?low:n>high?high:n; }
static void damage(Editor *e,int x,int y,int w,int h)
{
    int right,bottom;
    right=x+w-1; bottom=y+h-1;
    if(x<0) x=0;
    if(y<0) y=0;
    if(right>=(int)e->canvas.width) right=(int)e->canvas.width-1;
    if(bottom>=(int)e->canvas.height) bottom=(int)e->canvas.height-1;
    if(x>right || y>bottom) return;
    if(x<e->dirty_x0) e->dirty_x0=x;
    if(y<e->dirty_y0) e->dirty_y0=y;
    if(right>e->dirty_x1) e->dirty_x1=right;
    if(bottom>e->dirty_y1) e->dirty_y1=bottom;
}
static void begin_damage(Editor *e)
{
    Selection *s;
    s=&e->selection;
    e->dirty_x0=(int)e->canvas.width; e->dirty_y0=(int)e->canvas.height;
    e->dirty_x1=e->dirty_y1=-1;
    if(s->state) damage(e,s->x+s->dx,s->y+s->dy,s->w,s->h);
}
static unsigned end_damage(Editor *e)
{
    Selection *s;
    s=&e->selection;
    if(s->state) damage(e,s->x+s->dx,s->y+s->dy,s->w,s->h);
    return EDIT_STATUS|(e->dirty_x1>=e->dirty_x0?EDIT_SELECTION:0);
}
/* The existing 8192-byte fill workspace is idle while a selection floats.
   First 4096 bytes capture indices; next 512 hold the packed source mask. */
static void floating(Editor *e)
{
    Selection *s;
    unsigned char *pixels,*mask;
    unsigned x,y,i;
    s=&e->selection;
    if(s->state!=SEL_ACTIVE) return;
    editor_finish(e); e->playing=0;
    pixels=(unsigned char *)e->fill_queue; mask=pixels+4096;
    memset(mask,0,512);
    for(y=0;y<(unsigned)s->h;++y) for(x=0;x<(unsigned)s->w;++x) {
        i=y*s->w+x;
        pixels[i]=e->pixels[(s->y+y)*e->canvas.width+s->x+x];
        if(canvas_transparent(&e->canvas,s->x+x,s->y+y)) mask[i/8]|=(unsigned char)(1U<<(i%8));
    }
    s->state=SEL_FLOAT;
}
void selection_sample(const Editor *e,int x,int y,unsigned char *colour,int *transparent)
{
    const Selection *s;
    const unsigned char *pixels,*mask;
    int sx,sy;
    unsigned i;
    s=&e->selection;
    *colour=e->pixels[(unsigned)y*e->canvas.width+x];
    *transparent=canvas_transparent(&e->canvas,(unsigned)x,(unsigned)y);
    if(!selection_floating(e) || (!s->dx && !s->dy)) return;
    if(x>=s->x && x<s->x+s->w && y>=s->y && y<s->y+s->h) {
        if(e->document->transparent) *transparent=1;
        else { *colour=e->background; *transparent=0; }
    }
    sx=x-s->x-s->dx; sy=y-s->y-s->dy;
    if(sx<0 || sy<0 || sx>=s->w || sy>=s->h) return;
    pixels=(const unsigned char *)e->fill_queue; mask=pixels+4096;
    i=(unsigned)sy*s->w+sx;
    /* Explicit mask holes composite over the cleared/original destination. */
    if(!((mask[i/8]>>(i%8))&1)) { *colour=pixels[i]; *transparent=0; }
}
static void apply(Editor *e)
{
    Selection *s;
    unsigned x,y;
    unsigned char colour;
    int transparent;
    s=&e->selection;
    /* Sampling reads original destination, captured source and metadata only;
       each destination is written once, so overlaps never feed later samples. */
    for(y=0;y<e->canvas.height;++y) for(x=0;x<e->canvas.width;++x) {
        selection_sample(e,(int)x,(int)y,&colour,&transparent);
        e->pixels[y*e->canvas.width+x]=colour;
        canvas_mask_set(&e->canvas,x,y,transparent);
    }
    s->state=SEL_ACTIVE; s->x+=s->dx; s->y+=s->dy; s->dx=s->dy=0;
    /* Keep only the visible part selected after committing clipped moves. */
    if(s->x<0) { s->w+=s->x; s->x=0; }
    if(s->y<0) { s->h+=s->y; s->y=0; }
    if(s->x+s->w>(int)e->canvas.width) s->w=(int)e->canvas.width-s->x;
    if(s->y+s->h>(int)e->canvas.height) s->h=(int)e->canvas.height-s->y;
    if(s->w<=0 || s->h<=0) selection_reset(e);
    history_commit(&e->history,&e->canvas);
}
int selection_key(Editor *e,int key,unsigned *flags)
{
    Selection *s;
    unsigned x,y;
    s=&e->selection;
    if(key==CMD_SELECT_ALL && !selection_floating(e)) {
        editor_finish(e); e->playing=0; begin_damage(e); selection_reset(e);
        s->state=SEL_ACTIVE; s->w=(int)e->canvas.width; s->h=(int)e->canvas.height;
        *flags=end_damage(e); return 1;
    }
    if((key==CMD_EXIT || key==CMD_DESELECT) && s->state) {
        begin_damage(e);
        if(selection_floating(e)) damage(e,s->x,s->y,s->w,s->h);
        selection_reset(e); *flags=end_damage(e); return 1;
    }
    if(key==CMD_MOVE && s->state==SEL_ACTIVE) {
        floating(e); *flags=EDIT_STATUS|EDIT_UI; e->tool=TOOL_MOVE; return 1;
    }
    if((key==KEY_LEFT || key==KEY_RIGHT || key==KEY_UP || key==KEY_DOWN) && s->state>=SEL_ACTIVE) {
        begin_damage(e); floating(e);
        if(key==KEY_LEFT) --s->dx;
        if(key==KEY_RIGHT) ++s->dx;
        if(key==KEY_UP) --s->dy;
        if(key==KEY_DOWN) ++s->dy;
        s->dx=clamp(s->dx,-s->x-s->w,(int)e->canvas.width-s->x);
        s->dy=clamp(s->dy,-s->y-s->h,(int)e->canvas.height-s->y);
        *flags=end_damage(e); return 1;
    }
    if(key==CMD_APPLY && selection_floating(e)) {
        begin_damage(e); damage(e,s->x,s->y,s->w,s->h); apply(e);
        *flags=end_damage(e)|EDIT_TIMELINE; return 1;
    }
    if(selection_floating(e)) { *flags=EDIT_STATUS; return 1; }
    if(key==CMD_FRAME_DELETE && s->state==SEL_ACTIVE) {
        begin_damage(e);
        for(y=(unsigned)s->y;y<(unsigned)(s->y+s->h);++y)
            for(x=(unsigned)s->x;x<(unsigned)(s->x+s->w);++x) {
                if(e->document->transparent) canvas_mask_set(&e->canvas,x,y,1);
                else canvas_set(&e->canvas,x,y,e->background);
            }
        history_commit(&e->history,&e->canvas); *flags=end_damage(e)|EDIT_TIMELINE; return 1;
    }
    return 0;
}
static int mapped(int value,int origin,int zoom,int size)
{
    long delta;
    delta=(long)value-origin;
    delta=delta<0?-((-delta+zoom-1)/zoom):delta/zoom;
    if(delta < -2L*size) delta=-2L*size;
    if(delta > 2L*size) delta=2L*size;
    return (int)delta;
}
unsigned selection_mouse(Editor *e,int x,int y,unsigned buttons,unsigned pressed)
{
    Selection *s;
    int cx,cy,inside,old_x,old_y,old_w,old_h;
    s=&e->selection;
    cx=mapped(x,e->view.x,e->view.zoom,(int)e->canvas.width);
    cy=mapped(y,e->view.y,e->view.zoom,(int)e->canvas.height);
    inside=cx>=0 && cy>=0 && cx<(int)e->canvas.width && cy<(int)e->canvas.height;
    if(s->state==SEL_RECT) {
        old_x=s->x; old_y=s->y; old_w=s->w; old_h=s->h;
        begin_damage(e); cx=clamp(cx,0,(int)e->canvas.width-1); cy=clamp(cy,0,(int)e->canvas.height-1);
        s->x=cx<s->anchor_x?cx:s->anchor_x; s->y=cy<s->anchor_y?cy:s->anchor_y;
        s->w=(cx>s->anchor_x?cx-s->anchor_x:s->anchor_x-cx)+1;
        s->h=(cy>s->anchor_y?cy-s->anchor_y:s->anchor_y-cy)+1;
        if(!(buttons&EDIT_LEFT)) s->state=SEL_ACTIVE;
        if(old_x==s->x && old_y==s->y && old_w==s->w && old_h==s->h) return EDIT_STATUS;
        return end_damage(e);
    }
    if(s->state==SEL_DRAG) {
        old_x=s->dx; old_y=s->dy;
        begin_damage(e);
        s->dx=clamp(s->old_dx+cx-s->start_x,-s->x-s->w,(int)e->canvas.width-s->x);
        s->dy=clamp(s->old_dy+cy-s->start_y,-s->y-s->h,(int)e->canvas.height-s->y);
        if(!(buttons&EDIT_LEFT)) s->state=SEL_FLOAT;
        if(old_x==s->dx && old_y==s->dy) return EDIT_STATUS;
        return end_damage(e);
    }
    if(!(pressed&EDIT_LEFT) || !inside) return 0;
    if(s->state>=SEL_ACTIVE && cx>=s->x+s->dx && cx<s->x+s->dx+s->w && cy>=s->y+s->dy && cy<s->y+s->dy+s->h) {
        floating(e); s->state=SEL_DRAG;
        s->start_x=cx; s->start_y=cy; s->old_dx=s->dx; s->old_dy=s->dy;
        return EDIT_STATUS;
    }
    if(selection_floating(e)) return 0;
    begin_damage(e); selection_reset(e); s->state=SEL_RECT;
    s->x=s->anchor_x=cx; s->y=s->anchor_y=cy; s->w=s->h=1;
    return end_damage(e);
}
