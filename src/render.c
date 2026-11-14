#include <stdio.h>
#include <string.h>
#include "render.h"
#include "video.h"
#include "theme.h"
#include "cursor.h"
#include "workpal.h"

static void tool_row(const Editor *editor,int tool)
{
    static const char *labels[4]={"P Pen ","E Ers ","F Fill","I Pick"};
    int y;
    unsigned char ink,paper;
    y=TOOL_Y+tool*TOOL_STEP;
    paper=tool==editor->tool?UI_PAPER:UI_PANEL;
    ink=tool==editor->tool?UI_INK:UI_WHITE;
    cursor_protect(TOOL_X,y,TOOL_WIDTH,TOOL_HEIGHT);
    video_rect(TOOL_X,y,TOOL_WIDTH,1,paper);
    video_rect(TOOL_X,y+TOOL_HEIGHT-1,TOOL_WIDTH,1,paper);
    video_rect(TOOL_X,y+1,1,FONT_HEIGHT,paper);
    video_rect(TOOL_X+TOOL_WIDTH-1,y+1,1,FONT_HEIGHT,paper);
    video_text_bg(TOOL_TEXT_X,y+TOOL_TEXT_Y,labels[tool],ink,paper);
}
static void swatch(const Editor *e,int slot)
{
    int x,y;
    unsigned char colour,border;
    x=PALETTE_X+slot%PALETTE_COLS*PALETTE_CELL_W;
    y=PALETTE_Y+slot/PALETTE_COLS*PALETTE_CELL_H;
    colour=workpal_colour(slot);
    border=colour==e->foreground?UI_WHITE:colour==e->background?UI_FOCUS:UI_INK;
    cursor_protect(x,y,PALETTE_CELL_W,PALETTE_CELL_H);
    video_rect(x,y,PALETTE_CELL_W,PALETTE_CELL_H,border);
    video_rect(x+1,y+1,PALETTE_CELL_W-2,PALETTE_CELL_H-2,colour);
}
static void current_colour(unsigned char colour,int y,char name)
{
    char text[5];
    cursor_protect(PALETTE_LABEL_X,y-1,TOOL_WIDTH,SWATCH_HEIGHT);
    sprintf(text,"%c%03u",name,(unsigned)colour);
    video_text_bg(PALETTE_LABEL_X,y,text,UI_WHITE,UI_PANEL);
    video_rect(SWATCH_X,y-1,SWATCH_WIDTH,SWATCH_HEIGHT,UI_WHITE);
    video_rect(SWATCH_X+1,y,SWATCH_WIDTH-2,FONT_HEIGHT,colour);
}
void render_ui(const Editor *editor)
{
    int i;
    video_rect(0,UI_MENU_HEIGHT,UI_LEFT_WIDTH,UI_STATUS_Y-UI_MENU_HEIGHT,UI_PANEL);
    video_text(PALETTE_LABEL_X,TOOL_TITLE_Y,"Tools",UI_WHITE);
    video_rect(1,TOOL_TITLE_Y+FONT_HEIGHT+1,TOOL_WIDTH+1,1,UI_DIM);
    for(i=0;i<4;++i) tool_row(editor,i);
    video_text(PALETTE_LABEL_X,PALETTE_TITLE_Y,"Colour",UI_WHITE);
    current_colour(editor->foreground,FG_Y,'F');
    current_colour(editor->background,BG_Y,'B');
    for(i=0;i<16;++i) swatch(editor,i);
    video_text_bg(PALETTE_X,PALETTE_MORE_Y+1,"A All ",UI_INK,UI_BAR);
}
/* Grid dots follow exact cell edges, using a nearby shade rather than a bright
   continuous mesh. This is view-only and never writes to canvas storage. */
static unsigned char grid_colour(unsigned char colour)
{
    unsigned n;
    if(colour<16) return (unsigned char)(colour+(colour<8?1:-1));
    n=colour-16;
    if(colour>=232) return (unsigned char)(colour-180);
    if(n%6) return (unsigned char)(colour-1);
    if(n/6%6) return (unsigned char)(colour-6);
    if(n/36) return (unsigned char)(colour-36);
    return 17;
}
static void canvas_cell(const Editor *editor,int x,int y)
{
    int sx,sy,zoom,i;
    unsigned char colour,grid;
    zoom=editor->view.zoom;
    sx=editor->view.x+x*zoom; sy=editor->view.y+y*zoom;
    colour=editor->canvas.pixels[(unsigned)y*editor->canvas.width+x];
    video_rect(sx,sy,zoom,zoom,colour);
    if(editor->grid && zoom>=2) {
        grid=grid_colour(colour);
        for(i=1;i<zoom;i+=2) {
            video_pixel(sx+zoom-1,sy+i,grid);
            video_pixel(sx+i,sy+zoom-1,grid);
        }
    }
}

void render_canvas(const Editor *editor)
{
    unsigned x, y;
    int zoom;
    const View *view;
    view = &editor->view;
    zoom = view->zoom;
    {
        Rect area,art,parts[4];
        int n,i;
        area.x=DRAW_X-1; area.y=DRAW_Y-1; area.w=DRAW_WIDTH+2; area.h=DRAW_HEIGHT+2;
        art.x=view->x-1; art.y=view->y-1;
        art.w=(int)view->width*zoom+2; art.h=(int)view->height*zoom+2;
        n=rect_subtract(&area,&art,parts);
        for(i=0;i<n;++i) video_rect(parts[i].x,parts[i].y,parts[i].w,parts[i].h,UI_DESK);
        video_rect(art.x,art.y,art.w,1,UI_FRAME);
        video_rect(art.x,art.y+art.h-1,art.w,1,UI_FRAME);
        video_rect(art.x,art.y,1,art.h,UI_FRAME);
        video_rect(art.x+art.w-1,art.y,1,art.h,UI_FRAME);
    }
    for (y = 0; y < editor->canvas.height; ++y)
        for (x = 0; x < editor->canvas.width; ++x)
            canvas_cell(editor,(int)x,(int)y);
}

void render_stroke(const Editor *editor)
{
    int x,y;
    for(y=editor->dirty_y0;y<=editor->dirty_y1;++y)
        for(x=editor->dirty_x0;x<=editor->dirty_x1;++x) canvas_cell(editor,x,y);
}

void render_status_text(const Editor *editor, int mouse_x, int mouse_y, int has_mouse, const char *filename, char *text)
{
    char coords[12];
    int x, y;
    static const char *names[4] = {"Pen", "Ers", "Fil", "Pck"};
    if (has_mouse && view_map(&editor->view, mouse_x, mouse_y, &x, &y))
        sprintf(coords, "X%02dY%02d", x, y);
    else strcpy(coords, "X--Y--");
    /* 12 basename cells, then all required document/view information fit
       in the 53 complete 6-pixel cells on a 320-pixel scanline. */
    sprintf(text, "%-12.12s%c%ux%u %s %s F%03u B%03u Z%d",
            filename && *filename ? filename : "UNTITLED",
            editor_is_dirty(editor) ? '*' : ' ', editor->canvas.width,
            editor->canvas.height, coords, names[editor->tool],
            (unsigned)editor->foreground, (unsigned)editor->background,
            editor->view.zoom);

}

void render_status(const Editor *editor,int x,int y,int has_mouse)
{
    char text[UI_STATUS_MAX_CHARS+1];
    render_status_text(editor,x,y,has_mouse,"",text);
    video_rect(0,UI_STATUS_Y,UI_WIDTH,FONT_HEIGHT,UI_BAR); video_text(0,UI_STATUS_Y,text,UI_INK);
}
void render_restore(const Editor *editor,const Rect *r)
{
    Rect screen,art,parts[4];
    int i,n;
    if(r->w<=0 || r->h<=0) return;
    cursor_protect(r->x,r->y,r->w,r->h);
    video_clip(r->x,r->y,r->w,r->h);
    screen.x=0; screen.y=UI_MENU_HEIGHT; screen.w=UI_WIDTH; screen.h=UI_STATUS_Y-UI_MENU_HEIGHT;
    art.x=editor->view.x-1; art.y=editor->view.y-1;
    art.w=(int)editor->view.width*editor->view.zoom+2;
    art.h=(int)editor->view.height*editor->view.zoom+2;
    n=rect_subtract(&screen,&art,parts);
    for(i=0;i<n;++i) video_rect(parts[i].x,parts[i].y,parts[i].w,parts[i].h,UI_DESK);
    if(r->x<UI_LEFT_WIDTH || r->x+r->w>UI_RIGHT_X) render_ui(editor);
    if(rect_overlap(r,&art)) render_canvas(editor);
    video_unclip();
}
void render_ui_change(const Editor *editor,int tool,unsigned char fg,unsigned char bg,
                      const unsigned char *old)
{
    int i;
    unsigned char colour;
    if(tool!=editor->tool) {
        tool_row(editor,tool); tool_row(editor,editor->tool);
    }
    for(i=0;i<16;++i) {
        colour=workpal_colour(i);
        if(old[i]!=colour || (fg!=editor->foreground && (colour==fg || colour==editor->foreground)) ||
           (bg!=editor->background && (colour==bg || colour==editor->background))) swatch(editor,i);
    }
    if(fg!=editor->foreground) current_colour(editor->foreground,FG_Y,'F');
    if(bg!=editor->background) current_colour(editor->background,BG_Y,'B');
}
