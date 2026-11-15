#include <stdio.h>
#include <string.h>
#include "render.h"
#include "video.h"
#include "theme.h"
#include "cursor.h"
#include "workpal.h"
#include "profile.h"

/* Original bitmap marks, independently drawn at 8 and 16 pixels. */
static const unsigned char icons[7][8]={
 {3,7,14,28,56,112,224,192},{0,28,62,127,254,124,56,0},
 {16,40,68,130,124,56,16,0},{3,6,12,24,48,96,192,128},
 {255,129,129,129,129,129,129,255},{16,56,84,146,84,56,16,0},{255,129,189,165,165,189,129,255}};
static const unsigned short large_icons[7][16]={
 {3,7,15,30,60,120,240,480,960,1920,3840,7680,15360,28672,57344,49152},
 {0,0,992,2032,4088,8188,16382,32767,65534,32764,16376,8176,4064,1984,896,0},
 {256,640,1088,2080,4112,8200,16388,32770,65534,32764,16376,8176,4064,1984,896,256},
 {15,31,62,124,248,496,992,1984,3968,7936,15872,31744,63488,61440,57344,32768},
 {65535,32769,40961,32769,32769,32769,32769,32769,32769,32769,32769,32769,32769,32769,32769,65535},
 {256,896,1856,3376,6424,12556,24838,65535,24838,12556,6424,3376,1856,896,256,0},
 {65535,32769,32769,40957,40965,40965,40965,40965,40965,40965,40965,40957,32769,32769,32769,65535}};
static int restoring;
static void protect(int x,int y,int w,int h)
{ if(!restoring) cursor_protect(x,y,w,h); }
static void icon(int tool,int x,int y,int width,int height,int selected,int hover)
{
    int xx,yy,size,ix;
    unsigned short bits;
    unsigned char ink,paper;
    size=UI_WIDTH==640?16:8;
    paper=hover?UI_SELECT:selected?UI_PAPER:UI_PANEL;
    ink=hover?UI_WHITE:selected?UI_INK:UI_WHITE;
    protect(x,y,width,height);
    video_rect(x,y,width,height,selected?UI_WHITE:UI_DIM);
    video_rect(x+1,y+1,width-2,height-2,paper);
    ix=x+(width-size)/2;
    for(yy=0;yy<size;++yy) {
        bits=size==16?large_icons[tool][yy]:icons[tool][yy];
        for(xx=0;xx<size;++xx) if(bits & (1U<<(size-1-xx))) video_pixel(ix+xx,y+2+yy,ink);
    }
}
void render_group(const Editor *e,int group)
{
    int active;
    PROFILE_COUNT(ui_groups);
    active=tool_group(e->tool);
    if(e->selection.state && e->group_tool[1]==TOOL_ALL) active=1;
    icon(e->group_tool[group],TOOL_X,TOOL_Y+group*TOOL_STEP,TOOL_WIDTH,TOOL_HEIGHT,active==group,0);
    video_pixel(TOOL_X+TOOL_WIDTH-3,TOOL_Y+group*TOOL_STEP+TOOL_HEIGHT-3,UI_WHITE);
}
void render_flyout(const Editor *e,const Flyout *f)
{
    Rect r;
    int i,tool;
    if(f->group<0) return;
    PROFILE_COUNT(ui_flyouts);
    flyout_bounds(f->group,&r); protect(r.x,r.y,r.w,r.h);
    video_rect(r.x,r.y,r.w,r.h,UI_INK);
    for(i=0;i<flyout_count(f->group);++i) {
        tool=flyout_tool(f->group,i);
        icon(tool,r.x+1+i*FLY_CELL,r.y+1,FLY_CELL,FLY_CELL,e->group_tool[f->group]==tool,f->hover==i);
    }
}
static void swatch(const Editor *e,int slot)
{
    int x,y;
    unsigned char colour,border;
    x=PALETTE_X+slot%PALETTE_COLS*PALETTE_CELL_W;
    y=PALETTE_Y+slot/PALETTE_COLS*PALETTE_CELL_H;
    colour=workpal_colour(slot);
    border=colour==e->foreground?UI_WHITE:colour==e->background?UI_FOCUS:UI_INK;
    protect(x,y,PALETTE_CELL_W,PALETTE_CELL_H);
    video_rect(x,y,PALETTE_CELL_W,PALETTE_CELL_H,border);
    video_rect(x+1,y+1,PALETTE_CELL_W-2,PALETTE_CELL_H-2,colour);
}
static void current_colour(unsigned char colour,int y,char name)
{
    char text[5];
    protect(PALETTE_LABEL_X,y-1,UI_LEFT_WIDTH-2,SWATCH_HEIGHT);
    sprintf(text,"%c%03u",name,(unsigned)colour);
    video_text_bg(PALETTE_LABEL_X,y,text,UI_WHITE,UI_PANEL);
    video_rect(SWATCH_X,y-1,SWATCH_WIDTH,SWATCH_HEIGHT,UI_WHITE);
    video_rect(SWATCH_X+1,y,SWATCH_WIDTH-2,FONT_HEIGHT,colour);
}
void render_ui(const Editor *editor)
{
    int i;
    PROFILE_COUNT(ui_full);
    video_rect(0,0,UI_LEFT_WIDTH,UI_STATUS_Y,UI_PANEL);
    video_rect(UI_RIGHT_X,0,UI_WIDTH-UI_RIGHT_X,UI_STATUS_Y,UI_PANEL);
    for(i=0;i<TOOL_GROUPS;++i) render_group(editor,i);
    video_text(PALETTE_LABEL_X,PALETTE_TITLE_Y,"Colour",UI_WHITE);
    current_colour(editor->foreground,FG_Y,'F');
    current_colour(editor->background,BG_Y,'B');
    for(i=0;i<16;++i) swatch(editor,i);
    video_text_bg(PALETTE_X,PALETTE_MORE_Y+1,"A All ",UI_INK,UI_BAR);
}
/* Palette-derived luminance selects black/white without changing the DAC.
   Fine mesh is dashed; tile boundaries are solid and two-tone. */
static unsigned char grid_colour(unsigned char colour,int strong)
{
    unsigned n,level,grey;
    if(colour<16) level=colour;
    else { n=colour-16; level=(3*(n%6)+6*(n/6%6)+n/36%6)*3/10; }
    grey=level<8?level+(strong?5:3):level-(strong?5:3);
    if(grey<3) grey=3;
    if(grey>12) grey=12;
    return (unsigned char)grey;
}
static unsigned char preview_colour(const Editor *e,int x,int y)
{
    unsigned char colour;
    colour=e->preview_link?e->background:e->preview_colour;
    if(e->checker && ((x/8+y/8)&1)) {
        if(colour<16) colour=(unsigned char)(colour==15?14:colour+1);
        else colour=(unsigned char)((colour-16)%6==5?colour-1:colour+1);
    }
    return colour;
}
static void preview_cell(const Editor *e,int sx,int sy,int zoom)
{
    int x,y,w,h;
    for(y=sy;y<sy+zoom;y+=h) {
        h=8-y%8; if(h>sy+zoom-y) h=sy+zoom-y;
        for(x=sx;x<sx+zoom;x+=w) {
            w=8-x%8; if(w>sx+zoom-x) w=sx+zoom-x;
            video_rect(x,y,w,h,preview_colour(e,x,y));
        }
    }
}
static void canvas_cell(const Editor *editor,int x,int y)
{
    int sx,sy,zoom;
    unsigned char colour,grid;
    int transparent,xx,yy,left,top,right,bottom;
    const Selection *selection;
    zoom=editor->view.zoom;
    sx=editor->view.x+x*zoom; sy=editor->view.y+y*zoom;
    selection_sample(editor,x,y,&colour,&transparent);
    if(transparent) { preview_cell(editor,sx,sy,zoom); colour=preview_colour(editor,sx,sy); }
    else video_rect(sx,sy,zoom,zoom,colour);
    grid=grid_colour(colour,0);
    if(editor->grid && zoom>=3) {
        video_rect(sx+zoom-1,sy,1,zoom,grid);
        video_rect(sx,sy+zoom-1,zoom,1,grid);
    }
    if(editor->guide && zoom>=2) {
        grid=grid_colour(colour,1);
        if(((editor->guide&1) && (x+1)%8==0) || ((editor->guide&2) && (x+1)%16==0))
            video_rect(sx+zoom-1,sy,1,zoom,grid);
        if(((editor->guide&1) && (y+1)%8==0) || ((editor->guide&2) && (y+1)%16==0))
            video_rect(sx,sy+zoom-1,zoom,1,grid);
    }
    selection=&editor->selection;
    if(selection->state) {
        left=selection->x+selection->dx; top=selection->y+selection->dy;
        right=left+selection->w-1; bottom=top+selection->h-1;
        if(x>=left && x<=right && y>=top && y<=bottom &&
           (x==left || x==right || y==top || y==bottom)) {
            for(yy=0;yy<zoom;++yy) for(xx=0;xx<zoom;++xx)
                if((x==left && xx==0) || (x==right && xx==zoom-1) ||
                   (y==top && yy==0) || (y==bottom && yy==zoom-1))
                    video_pixel(sx+xx,sy+yy,((sx+xx+sy+yy)/2)&1?15:0);
        }
    }
}

static void canvas_frame(int x,int y,int w,int h)
{
    video_rect(x,y,w,1,UI_FRAME);
    video_rect(x,y+h-1,w,1,UI_FRAME);
    video_rect(x,y,1,h,UI_FRAME);
    video_rect(x+w-1,y,1,h,UI_FRAME);
}

void render_canvas(const Editor *editor)
{
    unsigned x, y;
    int zoom;
    const View *view;
    PROFILE_COUNT(canvas_full);
    PROFILE_ADD(canvas_cells,(unsigned long)editor->canvas.width*editor->canvas.height);
    view = &editor->view;
    zoom = view->zoom;
    {
        Rect area,art,parts[4];
        int n,i;
        area.x=DRAW_X-1; area.y=DRAW_Y-1; area.w=DRAW_WIDTH+3; area.h=UI_STATUS_Y-DRAW_Y+1;
        art.x=view->x-1; art.y=view->y-1;
        art.w=(int)view->width*zoom+2; art.h=(int)view->height*zoom+2;
        n=rect_subtract(&area,&art,parts);
        for(i=0;i<n;++i) video_rect(parts[i].x,parts[i].y,parts[i].w,parts[i].h,UI_DESK);
        canvas_frame(art.x,art.y,art.w,art.h);
    }
    for (y = 0; y < editor->canvas.height; ++y)
        for (x = 0; x < editor->canvas.width; ++x)
            canvas_cell(editor,(int)x,(int)y);
}

static void render_canvas_region(const Editor *editor,const Rect *r)
{
    int x0,y0,x1,y1,x,y,zoom,width,height;
    zoom=editor->view.zoom; width=(int)editor->canvas.width;
    height=(int)editor->canvas.height;
    canvas_frame(editor->view.x-1,editor->view.y-1,
                 width*zoom+2,height*zoom+2);
    x0=(r->x-editor->view.x)/zoom;
    x1=(r->x+r->w-1-editor->view.x)/zoom;
    y0=(r->y-editor->view.y)/zoom;
    y1=(r->y+r->h-1-editor->view.y)/zoom;
    if(x0<0) x0=0;
    if(y0<0) y0=0;
    if(x1>=width) x1=width-1;
    if(y1>=height) y1=height-1;
    PROFILE_COUNT(canvas_partial);
    PROFILE_ADD(canvas_cells,(unsigned long)(x1-x0+1)*(unsigned long)(y1-y0+1));
    for(y=y0;y<=y1;++y)
        for(x=x0;x<=x1;++x) canvas_cell(editor,x,y);
}

void render_playback_canvas(const Editor *editor,unsigned previous_frame)
{
    unsigned x,y,offset,previous_offset,previous_mask_offset,mask_offset;
    unsigned char bit,current_transparent,previous_transparent;
    int sx,sy,zoom;
    if(previous_frame>=editor->document->frames) return;
    PROFILE_COUNT(canvas_partial);
    PROFILE_ADD(canvas_compares,(unsigned long)editor->canvas.width*editor->canvas.height);
    zoom=editor->view.zoom;
    previous_offset=previous_frame*editor->history.size;
    previous_mask_offset=previous_frame*editor->history.size/8;
    for(y=0;y<editor->canvas.height;++y) for(x=0;x<editor->canvas.width;++x) {
        offset=y*editor->canvas.width+x;
        mask_offset=previous_mask_offset+offset/8;
        bit=(unsigned char)(1U<<(offset%8));
        current_transparent=(unsigned char)((editor->mask[offset/8]&bit)!=0);
        previous_transparent=(unsigned char)((editor->document->mask[mask_offset]&bit)!=0);
        if(current_transparent!=previous_transparent ||
           (!current_transparent &&
            editor->pixels[offset]!=editor->document->pixels[previous_offset+offset])) {
            sx=editor->view.x+(int)x*zoom; sy=editor->view.y+(int)y*zoom;
            cursor_protect(sx,sy,zoom,zoom);
            canvas_cell(editor,(int)x,(int)y);
        }
    }
}

void render_stroke(const Editor *editor)
{
    int x,y;
    PROFILE_COUNT(canvas_partial);
    PROFILE_ADD(canvas_cells,(unsigned long)(editor->dirty_x1-editor->dirty_x0+1)*
                (unsigned long)(editor->dirty_y1-editor->dirty_y0+1));
    for(y=editor->dirty_y0;y<=editor->dirty_y1;++y)
        for(x=editor->dirty_x0;x<=editor->dirty_x1;++x) canvas_cell(editor,x,y);
}

void render_cells(const Editor *editor)
{
    unsigned i,index;
    PROFILE_COUNT(canvas_partial);
    PROFILE_ADD(canvas_cells,editor->dirty_count);
    for(i=0;i<editor->dirty_count;++i) {
        index=editor->fill_queue[i];
        canvas_cell(editor,(int)(index%editor->canvas.width),
                    (int)(index/editor->canvas.width));
    }
}

void render_status_text(const Editor *editor, int mouse_x, int mouse_y, int has_mouse, const char *filename, char *text)
{
    char prefix[54],colours[12];
    unsigned columns,n;
    const Selection *selection;
    (void)mouse_x; (void)mouse_y; (void)has_mouse;
    selection=&editor->selection;
    if(selection_floating(editor))
        sprintf(prefix,"MOVE %+d,%+d Enter/Esc",selection->dx,selection->dy);
    else if(selection->state)
        sprintf(prefix,"SEL %02d,%02d %02dx%02d",selection->x,selection->y,selection->w,selection->h);
    else sprintf(prefix,"%-12.12s%c %ux%u Z%d",filename && *filename?filename:"UNTITLED",
                 editor_is_dirty(editor)?'*':' ',editor->canvas.width,editor->canvas.height,editor->view.zoom);
    columns=(unsigned)(UI_WIDTH/FONT_WIDTH); memset(text,' ',columns); text[columns]='\0';
    n=(unsigned)strlen(prefix); if(n>25) n=25; memcpy(text,prefix,n);
    sprintf(colours,"F%03u B%03u",(unsigned)editor->foreground,(unsigned)editor->background);
    memcpy(text+NAV_RIGHT/FONT_WIDTH,colours,9);

}

void render_nav(const Editor *e)
{
    unsigned i,first,count;
    int x;
    char label[4];
    first=nav_first(e->history.active,e->document->frames); count=e->document->frames-first;
    if(count>(unsigned)nav_slots()) count=(unsigned)nav_slots();
    protect(NAV_X,UI_STATUS_Y,NAV_RIGHT-NAV_X,FONT_HEIGHT);
    video_rect(NAV_X,UI_STATUS_Y,NAV_RIGHT-NAV_X,FONT_HEIGHT,UI_BAR);
    video_text_bg(NAV_X,UI_STATUS_Y,"<",UI_INK,UI_BAR);
    for(i=0;i<count;++i) {
        x=NAV_X+NAV_ARROW+(int)i*NAV_CELL;
        sprintf(label,"%2u",first+i+1);
        video_text_bg(x,UI_STATUS_Y,label,first+i==e->history.active?UI_WHITE:UI_INK,
                      first+i==e->history.active?UI_SELECT:UI_BAR);
    }
    video_text_bg(NAV_X+NAV_ARROW+(int)count*NAV_CELL,UI_STATUS_Y,">",UI_INK,UI_BAR);
}

#ifdef PIX86_TEST
void render_status(const Editor *editor,int x,int y,int has_mouse)
{
    char text[UI_STATUS_CHARS+1];
    render_status_text(editor,x,y,has_mouse,"",text);
    video_rect(0,UI_STATUS_Y,UI_WIDTH,FONT_HEIGHT,UI_BAR); video_text(0,UI_STATUS_Y,text,UI_INK); render_nav(editor);
}
#endif
void render_restore(const Editor *editor,const Rect *r)
{
    Rect screen,art,parts[4];
    int i,n;
    if(r->w<=0 || r->h<=0) return;
    PROFILE_COUNT(overlay_restores);
    cursor_protect(r->x,r->y,r->w,r->h); restoring=1;
    video_clip(r->x,r->y,r->w,r->h);
    screen.x=0; screen.y=0; screen.w=UI_WIDTH; screen.h=UI_STATUS_Y;
    art.x=editor->view.x-1; art.y=editor->view.y-1;
    art.w=(int)editor->view.width*editor->view.zoom+2;
    art.h=(int)editor->view.height*editor->view.zoom+2;
    n=rect_subtract(&screen,&art,parts);
    for(i=0;i<n;++i) video_rect(parts[i].x,parts[i].y,parts[i].w,parts[i].h,UI_DESK);
    if(r->x<UI_LEFT_WIDTH || r->x+r->w>UI_RIGHT_X) render_ui(editor);
    if(rect_overlap(r,&art)) render_canvas_region(editor,r);
    if(editor->timeline && r->y+r->h>TIMELINE_Y && r->x+r->w>DRAW_X-1) render_timeline(editor);
    video_unclip(); restoring=0;
}
void render_ui_change(const Editor *editor,int tool,unsigned char fg,unsigned char bg,
                      const unsigned char *old,int alpha)
{
    int i;
    unsigned char colour;
    PROFILE_COUNT(ui_controls);
    (void)tool; (void)alpha;
    for(i=0;i<16;++i) {
        colour=workpal_colour(i);
        if(old[i]!=colour || (fg!=editor->foreground && (colour==fg || colour==editor->foreground)) ||
           (bg!=editor->background && (colour==bg || colour==editor->background))) swatch(editor,i);
    }
    if(fg!=editor->foreground) current_colour(editor->foreground,FG_Y,'F');
    if(bg!=editor->background) current_colour(editor->background,BG_Y,'B');
}

static void timeline_thumbnail(const Editor *e,unsigned frame,int x,int y)
{
    int tx,ty;
    unsigned offset;
    unsigned char colour;
    for(ty=0;ty<8;++ty) for(tx=0;tx<8;++tx) {
        offset=(unsigned)(ty*e->canvas.height/8)*e->canvas.width+
               (unsigned)tx*e->canvas.width/8;
        colour=frame==e->history.active?e->pixels[offset]:
               e->document->pixels[frame*e->history.size+offset];
        if(frame==e->history.active?((e->mask[offset/8]>>(offset%8))&1):
           ((e->document->mask[(frame*e->history.size+offset)/8]>>(offset%8))&1))
            colour=preview_colour(e,x+12+tx,y+11+ty);
        video_pixel(x+12+tx,y+11+ty,colour);
    }
}

void render_timeline_thumbnail(const Editor *e)
{
    unsigned slot;
    int x,y;
    if(!e->timeline || e->history.active<e->first_frame) return;
    slot=e->history.active-e->first_frame;
    if(slot>=TIMELINE_SLOTS) return;
    PROFILE_COUNT(timeline_partial);
    x=DRAW_X+(int)slot*TIMELINE_CELL; y=TIMELINE_Y+13;
    protect(x+12,y+11,8,8);
    timeline_thumbnail(e,e->history.active,x,y);
}

void render_timeline(const Editor *e)
{
    unsigned f,slot;
    int x,y;
    unsigned char paper,ink;
    char label[20];
    static const char *controls[9]={"<",">","+","Dup","Del","Play","<<",">>","X"};
    if(!e->timeline) return;
    PROFILE_COUNT(timeline_full);
    protect(DRAW_X-1,TIMELINE_Y,DRAW_WIDTH+3,TIMELINE_HEIGHT);
    video_rect(DRAW_X-1,TIMELINE_Y,DRAW_WIDTH+3,TIMELINE_HEIGHT,UI_PANEL);
    video_rect(DRAW_X-1,TIMELINE_Y,DRAW_WIDTH+3,1,UI_DIM);
    for(slot=0;slot<9;++slot) {
        x=DRAW_X+(int)slot*((DRAW_WIDTH)/9);
        video_text_bg(x,TIMELINE_Y+2,slot==5 && e->playing?"Stop":controls[slot],UI_WHITE,UI_PANEL);
    }
    for(slot=0;slot<TIMELINE_SLOTS;++slot) {
        f=e->first_frame+slot; if(f>=e->document->frames) break;
        x=DRAW_X+(int)slot*TIMELINE_CELL; y=TIMELINE_Y+13;
        paper=f==e->history.active?UI_SELECT:UI_BAR;
        ink=f==e->history.active?UI_WHITE:UI_INK;
        video_rect(x,y,TIMELINE_CELL-2,21,paper);
        sprintf(label,"%u",f+1); video_text(x+2,y+1,label,ink);
        timeline_thumbnail(e,f,x,y);
    }
    sprintf(label,"%u/%u",e->history.active+1,e->document->frames);
    video_text(DRAW_X+TIMELINE_SLOTS*TIMELINE_CELL,TIMELINE_Y+14,label,UI_WHITE);
    sprintf(label,"%ums",e->document->duration[e->history.active]);
    video_text(DRAW_X+TIMELINE_SLOTS*TIMELINE_CELL,TIMELINE_Y+24,label,UI_WHITE);
}

static void render_timeline_playback_cell(const Editor *e,unsigned frame)
{
    int x,y;
    unsigned slot,number;
    unsigned char paper,ink;
    char label[3];
    if(frame<e->first_frame || frame>=e->document->frames) return;
    slot=frame-e->first_frame;
    if(slot>=TIMELINE_SLOTS) return;
    x=DRAW_X+(int)slot*TIMELINE_CELL; y=TIMELINE_Y+13;
    paper=frame==e->history.active?UI_SELECT:UI_BAR;
    ink=frame==e->history.active?UI_WHITE:UI_INK;
    cursor_protect(x,y,TIMELINE_CELL-2,21);
    video_rect(x,y,TIMELINE_CELL-2,11,paper);
    video_rect(x,y+19,TIMELINE_CELL-2,2,paper);
    video_rect(x,y+11,12,8,paper);
    video_rect(x+20,y+11,2,8,paper);
    number=frame+1;
    if(number>=10) {
        label[0]=(char)('0'+number/10); label[1]=(char)('0'+number%10); label[2]='\0';
    } else {
        label[0]=(char)('0'+number); label[1]='\0';
    }
    video_text(x+2,y+1,label,ink);
}

void render_timeline_playback(const Editor *e,unsigned previous_frame)
{
    int x,y;
    char label[20];
    if(!e->timeline) return;
    PROFILE_COUNT(timeline_partial);
    render_timeline_playback_cell(e,previous_frame);
    if(e->history.active!=previous_frame)
        render_timeline_playback_cell(e,e->history.active);
    x=DRAW_X+TIMELINE_SLOTS*TIMELINE_CELL;
    y=TIMELINE_Y+14;
    cursor_protect(x,y,6*FONT_WIDTH,2*FONT_HEIGHT+2);
    video_rect(x,y,6*FONT_WIDTH,FONT_HEIGHT,UI_PANEL);
    sprintf(label,"%u/%u",e->history.active+1,e->document->frames);
    video_text(x,y,label,UI_WHITE);
    video_rect(x,y+FONT_HEIGHT+2,6*FONT_WIDTH,FONT_HEIGHT,UI_PANEL);
    sprintf(label,"%ums",e->document->duration[e->history.active]);
    video_text(x,y+FONT_HEIGHT+2,label,UI_WHITE);
}
