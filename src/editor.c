#include "command.h"
#include <stddef.h>
#include <string.h>
#include "editor.h"
#include "tools.h"
#include "flyout.h"

static void follow(Editor *e);
static unsigned editor_frame_command(Editor *e,int key);

static void stroke_bounds(Editor *editor, int x0, int y0, int x1, int y1)
{
    editor->dirty_x0 = x0 < x1 ? x0 : x1;
    editor->dirty_x1 = x0 > x1 ? x0 : x1;
    editor->dirty_y0 = y0 < y1 ? y0 : y1;
    editor->dirty_y1 = y0 > y1 ? y0 : y1;
}

void editor_dispose(Editor *e)
{
    history_destroy(&e->history);
    if(e->document) anim_free(e->document);
    if(e->saved) anim_free(e->saved);
    e->document=e->saved=NULL;
}
int editor_new_shape(Editor *e,unsigned w,unsigned h)
{
    if(!anim_dimensions(w,h)) return 0;
    selection_reset(e);
    e->playing=0; e->elapsed=e->first_frame=0;
    e->dragging=e->segment_valid=0; e->previous_buttons=0;
    e->foreground=15; e->background=0;
    canvas_init(&e->canvas,w,h,e->pixels,sizeof(e->pixels));
    e->canvas.mask=e->mask;
    canvas_clear(&e->canvas,0);
    anim_set(e->document,0,sizeof(P86Image));
    e->document->version=3;
    e->document->width=w; e->document->height=h;
    e->document->frames=1; e->document->duration[0]=ANIM_DEFAULT_MS;
    e->history.active=0;
    history_init(&e->history,&e->canvas);
    view_fit(&e->view,&e->canvas,e->timeline,0);
    editor_mark_saved(e);
    return 1;
}
int editor_new_document(Editor *e,unsigned w,unsigned h,int transparent,unsigned char background)
{
    if(!editor_new_shape(e,w,h)) return 0;
    e->background=background;
    canvas_clear(&e->canvas,background);
    if(transparent) memset(e->mask,255,w*h/8);
    e->document->transparent=(unsigned char)(transparent!=0);
    e->transparent_eraser=(unsigned char)(transparent!=0);
    history_init(&e->history,&e->canvas);
    editor_mark_saved(e);
    return 1;
}
int editor_init(Editor *e,unsigned side,unsigned short *queue)
{
    if(!e || !queue || !anim_dimensions(side,side)) return 0;
    memset(e,0,sizeof(*e)); e->group_tool[1]=TOOL_SELECT; e->fill_queue=queue; e->preview_colour=7; e->checker=1;
    e->document=anim_alloc(sizeof(P86Image));
    e->saved=anim_alloc(sizeof(P86Image));
    if(!e->document || !e->saved || !history_create(&e->history,e->document)) {
        editor_dispose(e); return 0;
    }
    return editor_new_shape(e,side,side);
}

void editor_store(Editor *e)
{
    unsigned size; size=e->history.size;
    anim_copy(e->document->pixels+e->history.active*size,e->pixels,size);
    anim_copy(e->document->mask+e->history.active*size/8,e->mask,size/8);
}
void editor_restore(Editor *e)
{
    unsigned size; size=e->history.size;
    anim_copy(e->pixels,e->document->pixels+e->history.active*size,size);
    anim_copy(e->mask,e->document->mask+e->history.active*size/8,size/8);
}

void editor_finish(Editor *editor)
{
    if (editor->dragging)
        history_commit(&editor->history, &editor->canvas);
    editor->dragging = editor->segment_valid = 0;
    if(editor->selection.state==SEL_DRAG) editor->selection.state=SEL_FLOAT;
    if(editor->selection.state==SEL_RECT) editor->selection.state=SEL_ACTIVE;
}

unsigned editor_background(Editor *e,unsigned char colour)
{
    int changed;
    changed=e->background!=colour; e->background=colour;
    return EDIT_UI|EDIT_STATUS|(changed && e->preview_link?EDIT_CANVAS:0);
}

unsigned editor_mouse(Editor *editor, int x, int y, unsigned buttons)
{
    int cx, cy, inside, tool;
    unsigned pressed, changed, flags;
    unsigned char index;
    if(editor->playing) { editor->previous_buttons=buttons; return 0; }
    pressed = buttons & ~editor->previous_buttons;
    editor->previous_buttons = buttons;
    flags = EDIT_STATUS;
    if(selection_floating(editor) || editor->selection.state==SEL_RECT)
        return selection_mouse(editor,x,y,buttons,pressed);
    inside = view_map(&editor->view, x, y, &cx, &cy);
    if (editor->dragging) {
        if (inside) {
            if (!editor->segment_valid) {
                editor->last_x = cx;
                editor->last_y = cy;
            }
            changed=tools_stroke(&editor->canvas,editor->last_x,editor->last_y,cx,cy,
                                 editor->stroke_colour,editor->stroke_transparent);
            if (changed) {
                stroke_bounds(editor, editor->last_x, editor->last_y, cx, cy);
                flags |= EDIT_STROKE;
            }
            editor->last_x = cx;
            editor->last_y = cy;
        }
        editor->segment_valid = inside;
        if (!(buttons & EDIT_LEFT))
            editor_finish(editor);
        return flags;
    }
    if (pressed == 0)
        return flags;
    if (palette_hit(x, y, &index)) {
        if (pressed & EDIT_LEFT) editor->foreground = index;
        else if (pressed & EDIT_RIGHT) return flags | editor_background(editor,index);
        return flags | EDIT_UI;
    }
    tool = toolbar_hit(x, y);
    if ((pressed & EDIT_LEFT) && tool >= 0) {
        return flags|editor_key(editor,tool_command(editor->group_tool[tool]));
    }
    if((pressed&EDIT_LEFT) && ((editor->selection.state==SEL_ACTIVE && inside &&
        cx>=editor->selection.x && cy>=editor->selection.y &&
        cx<editor->selection.x+editor->selection.w && cy<editor->selection.y+editor->selection.h) ||
        editor->tool==TOOL_SELECT || editor->tool==TOOL_MOVE))
        return flags|selection_mouse(editor,x,y,buttons,pressed);
    if (!inside)
        return flags;
    if ((pressed & EDIT_RIGHT) ||
        ((pressed & EDIT_LEFT) && editor->tool == TOOL_PICK)) {
        tools_pick(&editor->canvas, (unsigned)cx, (unsigned)cy,
                   &editor->foreground);
        return flags | EDIT_UI;
    }
    if (!(pressed & EDIT_LEFT))
        return flags;
    if (editor->tool == TOOL_FILL) {
        if (tools_fill(&editor->canvas, (unsigned)cx, (unsigned)cy,
                       editor->foreground, editor->fill_queue)) {
            history_commit(&editor->history, &editor->canvas);
            flags |= EDIT_CANVAS;
        }
    } else {
        editor->dragging = editor->segment_valid = 1;
        editor->last_x = cx;
        editor->last_y = cy;
        editor->stroke_colour = editor->tool == TOOL_ERASER ?
                                editor->background : editor->foreground;
        editor->stroke_transparent=editor->tool==TOOL_ERASER && editor->transparent_eraser;
        if (tools_stroke(&editor->canvas, cx, cy, cx, cy, editor->stroke_colour,editor->stroke_transparent)) {
            stroke_bounds(editor, cx, cy, cx, cy);
            flags |= EDIT_STROKE;
        }
    }
    return flags;
}

unsigned editor_key(Editor *editor, int key)
{
    int zoom, tool;
    unsigned selection_flags;
    key = keyboard_command(key);
    if(selection_key(editor,key,&selection_flags)) return selection_flags;
    if(key>=CMD_SELECT_FRAME && key<CMD_SELECT_FRAME+ANIM_FRAMES)
        return editor_select(editor,(unsigned)(key-CMD_SELECT_FRAME));
    if(key>=CMD_FRAME_ADD && key<=CMD_SCROLL_RIGHT) {
        selection_reset(editor);
        if(key==CMD_PLAY) return editor_frame_command(editor,key);
        return editor_frame_command(editor,key)|EDIT_CANVAS;
    }
    editor->playing=0;

    tool = -1;
    if (key == 'p') tool = TOOL_PENCIL;
    if (key == 'e') tool = TOOL_ERASER;
    if (key == 'f') tool = TOOL_FILL;
    if (key == 'i') tool = TOOL_PICK;
    if(key==CMD_SELECT) tool=TOOL_SELECT;
    if(key==CMD_MOVE) tool=TOOL_MOVE;
    if (tool >= 0) {
        editor_finish(editor);
        editor->tool = tool; editor->group_tool[tool_group(tool)]=(unsigned char)tool;
        return EDIT_UI | EDIT_STATUS;
    }
    if(key==CMD_TIMELINE) {
        editor_finish(editor); editor->timeline=!editor->timeline;
        view_fit(&editor->view,&editor->canvas,editor->timeline,0);
        return EDIT_CANVAS | EDIT_STATUS | EDIT_TIMELINE;
    }
    if(key==CMD_GUIDE8 || key==CMD_GUIDE16) {
        editor_finish(editor);
        zoom=key==CMD_GUIDE8?1:2;
        editor->guide^=zoom;
        return EDIT_CANVAS | EDIT_STATUS;
    }
    if (key == 'g') {
        editor_finish(editor);
        editor->grid = !editor->grid;
        return EDIT_CANVAS | EDIT_STATUS;
    }
    if(key==CMD_TRANS_BLANK || key==CMD_TRANS_ERASE) {
        editor_finish(editor);
        if(key==CMD_TRANS_BLANK) {
            editor->document->transparent=!editor->document->transparent;
            editor->transparent_eraser=editor->document->transparent;
            history_commit(&editor->history,&editor->canvas);
        } else editor->transparent_eraser=!editor->transparent_eraser;
        return EDIT_UI|EDIT_STATUS|EDIT_TIMELINE;
    }
    if(key==CMD_PREVIEW_LINK || key==CMD_CHECKER) {
        editor_finish(editor);
    if(key==CMD_PREVIEW_LINK) editor->preview_link=!editor->preview_link;
        else editor->checker=!editor->checker;
        return EDIT_CANVAS|EDIT_STATUS;
    }
    if (key == 'c' || key==CMD_FILL_BG) {
        editor_finish(editor);
        if(key==CMD_CLEAR && editor->document->transparent) memset(editor->mask,255,editor->history.size/8);
        else canvas_clear(&editor->canvas, editor->background);
        history_commit(&editor->history, &editor->canvas);
        return EDIT_CANVAS | EDIT_STATUS;
    }
    if (key == CMD_UNDO || key == CMD_REDO) {
        editor_finish(editor);
        selection_reset(editor);
        if (key == CMD_UNDO)
            history_undo(&editor->history, &editor->canvas);
        else
            history_redo(&editor->history, &editor->canvas);
        follow(editor);
        return EDIT_CANVAS | EDIT_STATUS | EDIT_TIMELINE;
    }
    if (key == CMD_ZOOM_IN || key == CMD_ZOOM_OUT) {
        editor_finish(editor);
        zoom = editor->view.zoom + (key == '-' ? -1 : 1);
        if (view_fit(&editor->view, &editor->canvas, editor->timeline, zoom))
            return EDIT_CANVAS | EDIT_STATUS;
        return EDIT_STATUS;
    }
    return 0;
}

void editor_mark_saved(Editor *e)
{
    editor_finish(e);
    editor_store(e);
    anim_copy(e->saved,e->document,sizeof(P86Image));
    e->saved_foreground=e->foreground; e->saved_background=e->background;
    e->saved_width=e->canvas.width; e->saved_height=e->canvas.height;
}
int editor_is_dirty(const Editor *e)
{
    unsigned i,size;
    size=e->canvas.width*e->canvas.height;
    if(e->saved_width!=e->canvas.width || e->saved_height!=e->canvas.height ||
       e->saved_foreground!=e->foreground || e->saved_background!=e->background ||
       e->saved->frames!=e->document->frames || e->saved->transparent!=e->document->transparent) return 1;
    for(i=0;i<e->document->frames;++i) {
        if(e->saved->duration[i]!=e->document->duration[i]) return 1;
        if(anim_compare(e->saved->mask+i*size/8,i==e->history.active?e->mask:e->document->mask+i*size/8,size/8)) return 1;
        if(anim_compare(e->saved->pixels+i*size,
            i==e->history.active?e->pixels:e->document->pixels+i*size,size)) return 1;
    }
    return 0;
}
void editor_image(const Editor *e,P86Image *image)
{
    anim_copy(image,e->document,sizeof(P86Image));
    memcpy(image->pixels+e->history.active*e->history.size,e->pixels,e->history.size);
    memcpy(image->mask+e->history.active*e->history.size/8,e->mask,e->history.size/8);
    image->foreground=e->foreground; image->background=e->background;
}
int editor_load(Editor *e,const P86Image *im)
{
    unsigned frames,i;
    if(!im || im->transparent>1 || !anim_dimensions(im->width,im->height)) return 0;
    frames=im->frames?im->frames:1;
    if(frames>ANIM_FRAMES || frames>ANIM_PIXELS/(im->width*im->height)) return 0;
    for(i=0;i<frames;++i) if(im->frames && (im->duration[i]<20 || im->duration[i]>2000)) return 0;
    selection_reset(e);
    editor_finish(e); e->playing=0; e->elapsed=e->first_frame=0;
    anim_copy(e->document,im,sizeof(P86Image));
    e->document->frames=frames; e->document->version=3;
    if(!im->frames) e->document->duration[0]=ANIM_DEFAULT_MS;
    canvas_init(&e->canvas,im->width,im->height,e->pixels,sizeof(e->pixels));
    e->canvas.mask=e->mask;
    memcpy(e->pixels,im->pixels,im->width*im->height);
    if(im->frames) memcpy(e->mask,im->mask,im->width*im->height/8);
    else { memset(e->mask,0,im->width*im->height/8); anim_set(e->document->mask,0,sizeof(e->document->mask)); e->document->transparent=0; }
    e->transparent_eraser=e->document->transparent;
    e->foreground=im->foreground; e->background=im->background;
    e->previous_buttons=0; e->history.active=0;
    history_init(&e->history,&e->canvas);
    view_fit(&e->view,&e->canvas,e->timeline,0);
    editor_mark_saved(e); return 1;
}

static void follow(Editor *e)
{
    unsigned a;
    a=e->history.active;
    if(a<e->first_frame) e->first_frame=a;
    if(a>=e->first_frame+TIMELINE_SLOTS) e->first_frame=a-TIMELINE_SLOTS+1;
    if(e->first_frame>=e->document->frames) e->first_frame=e->document->frames-1;
}
unsigned editor_select(Editor *e,unsigned frame)
{
    if(frame>=e->document->frames) return 0;
    if(selection_floating(e)) return EDIT_STATUS;
    selection_reset(e);
    editor_finish(e); e->playing=0; e->elapsed=0;
    editor_store(e);
    e->history.active=frame;
    editor_restore(e);
    follow(e); return EDIT_CANVAS|EDIT_STATUS|EDIT_TIMELINE;
}
static unsigned editor_frame_command(Editor *e,int key)
{
    unsigned n,a,size,d;
    editor_finish(e);
    n=e->document->frames; a=e->history.active; size=e->history.size;
    if(key==CMD_PLAY) {
        e->playing=!e->playing; e->elapsed=0;
        if(e->playing) e->playback_dropped=0;
        return EDIT_TIMELINE|EDIT_STATUS;
    }
    if(key==CMD_FRAME_PREV || key==CMD_FRAME_NEXT)
        return editor_select(e,(a+n+(key==CMD_FRAME_PREV?-1:1))%n);
    e->playing=0;
    if(key==CMD_SCROLL_LEFT || key==CMD_SCROLL_RIGHT) {
        if(key==CMD_SCROLL_LEFT && e->first_frame) --e->first_frame;
        if(key==CMD_SCROLL_RIGHT && e->first_frame+TIMELINE_SLOTS<n) ++e->first_frame;
        return EDIT_TIMELINE;
    }
    editor_store(e);
    if(key==CMD_FRAME_ADD || key==CMD_FRAME_DUP) {
        if(n>=ANIM_FRAMES || n>=ANIM_PIXELS/size) return EDIT_STATUS;
        anim_move(e->document->pixels+(a+2)*size,e->document->pixels+(a+1)*size,(n-a-1)*size);
        anim_move(e->document->mask+(a+2)*size/8,e->document->mask+(a+1)*size/8,(n-a-1)*size/8);
        if(key==CMD_FRAME_DUP) anim_copy(e->document->mask+(a+1)*size/8,e->mask,size/8);
        else anim_set(e->document->mask+(a+1)*size/8,e->document->transparent?255:0,size/8);
        for(d=n;d>a+1;--d) e->document->duration[d]=e->document->duration[d-1];
        if(key==CMD_FRAME_DUP) anim_copy(e->document->pixels+(a+1)*size,e->pixels,size);
        else anim_set(e->document->pixels+(a+1)*size,e->background,size);
        e->document->duration[a+1]=key==CMD_FRAME_DUP?e->document->duration[a]:ANIM_DEFAULT_MS;
        e->document->frames=n+1; e->history.active=a+1;
    } else if(key==CMD_FRAME_DELETE) {
        if(n==1) return EDIT_STATUS;
        anim_move(e->document->pixels+a*size,e->document->pixels+(a+1)*size,(n-a-1)*size);
        anim_move(e->document->mask+a*size/8,e->document->mask+(a+1)*size/8,(n-a-1)*size/8);
        for(d=a;d+1<n;++d) e->document->duration[d]=e->document->duration[d+1];
        e->document->frames=n-1; if(a==n-1) e->history.active=a-1;
    } else if(key==CMD_FASTER || key==CMD_SLOWER) {
        d=e->document->duration[a];
        if(key==CMD_FASTER && d>20) d-=10;
        if(key==CMD_SLOWER && d<2000) d+=10;
        e->document->duration[a]=d;
    }
    editor_restore(e);
    history_commit(&e->history,&e->canvas); follow(e);
    return EDIT_CANVAS|EDIT_STATUS|EDIT_TIMELINE;
}
unsigned editor_tick(Editor *e,unsigned ms)
{
    unsigned long elapsed,remaining,total,cycles;
    unsigned old,next,i;
    unsigned long due;
    if(!e->playing || !ms) return 0;
    elapsed=(unsigned long)e->elapsed+ms;
    old=e->history.active;
    if(elapsed<e->document->duration[old]) {
        e->elapsed=(unsigned)elapsed;
        return 0;
    }
    if(e->document->frames==1) {
        e->elapsed=(unsigned)(elapsed%e->document->duration[old]);
        return 0;
    }

    /* Count frame deadlines the old catch-up path would have crossed, but
       present only the next frame and restart its duration from this tick. */
    due=1; remaining=elapsed-e->document->duration[old];
    next=(old+1)%e->document->frames;
    total=0;
    for(i=0;i<e->document->frames;++i) total+=e->document->duration[i];
    if(remaining>=total) {
        cycles=remaining/total;
        due+=cycles*e->document->frames;
        remaining%=total;
    }
    while(remaining>=e->document->duration[next]) {
        remaining-=e->document->duration[next];
        next=(next+1)%e->document->frames;
        ++due;
    }
    if(due>1) {
        unsigned long missed;
        missed=due-1;
        if(missed>=9999UL || e->playback_dropped>9999UL-missed) e->playback_dropped=9999UL;
        else e->playback_dropped+=missed;
    }
    e->playback_previous=old;
    editor_store(e);
    e->history.active=(old+1)%e->document->frames;
    e->elapsed=0;
    editor_restore(e);
    follow(e); return EDIT_PLAYBACK|EDIT_STATUS|EDIT_TIMELINE;
}
