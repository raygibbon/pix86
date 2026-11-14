#include <stdio.h>
#include <string.h>
#include "render.h"
#include "video.h"
#include "theme.h"
#include "cursor.h"

void render_ui(const Editor *editor)
{
    char text[12];
    int tool, x, y;
    unsigned index;
    static const char *labels[4] = {"Pen", "Ers", "Fil", "Pck"};
    video_rect(0, UI_MENU_HEIGHT, UI_LEFT_WIDTH, UI_STATUS_Y-UI_MENU_HEIGHT, UI_DESK);
    for (tool = 0; tool < 4; ++tool) {
        y = TOOL_Y + tool * TOOL_STEP;
        if (tool == editor->tool) video_rect(TOOL_X, y, TOOL_WIDTH, TOOL_HEIGHT, UI_BAR);
        video_text(TOOL_TEXT_X, y + TOOL_TEXT_Y, labels[tool], tool == editor->tool ? UI_INK : UI_WHITE);
    }
    video_rect(UI_RIGHT_X, UI_MENU_HEIGHT, UI_WIDTH-UI_RIGHT_X, UI_STATUS_Y-UI_MENU_HEIGHT, UI_DESK);
    video_text(PALETTE_LABEL_X, PALETTE_TITLE_Y, "Palette", UI_WHITE);
    for (index = 0; index < 256; ++index) {
        x = PALETTE_X + (int)(index % 16) * PALETTE_CELL_W;
        y = PALETTE_Y + (int)(index / 16) * PALETTE_CELL_H;
        video_rect(x, y, PALETTE_CELL_W, PALETTE_CELL_H,
                   index == editor->foreground ? UI_WHITE : UI_INK);
        video_rect(x + 1, y + 1, PALETTE_CELL_W - 2,
                   PALETTE_CELL_H - 2, (unsigned char)index);
    }
    sprintf(text, "FG %03u", (unsigned)editor->foreground);
    video_text(PALETTE_LABEL_X, FG_Y, text, UI_WHITE);
    video_rect(SWATCH_X, FG_Y-1, SWATCH_WIDTH, SWATCH_HEIGHT, UI_WHITE);
    video_rect(SWATCH_X+1, FG_Y, SWATCH_WIDTH-2, FONT_HEIGHT, editor->foreground);
    sprintf(text, "BG %03u", (unsigned)editor->background);
    video_text(PALETTE_LABEL_X, BG_Y, text, UI_WHITE);
    video_rect(SWATCH_X, BG_Y-1, SWATCH_WIDTH, SWATCH_HEIGHT, UI_WHITE);
    video_rect(SWATCH_X+1, BG_Y, SWATCH_WIDTH-2, FONT_HEIGHT, editor->background);
    video_text(PALETTE_LABEL_X, PALETTE_HINT_Y, "L:FG R:BG", UI_WHITE);
    video_text(PALETTE_LABEL_X, GRID_HINT_Y, "G: grid", UI_WHITE);
}

void render_canvas(const Editor *editor)
{
    unsigned x, y;
    int zoom, cell;
    const View *view;
    view = &editor->view;
    zoom = view->zoom;
    cell = editor->grid && zoom >= 2 ? zoom - 1 : zoom;
    {
        Rect area,art,parts[4];
        int n,i;
        area.x=DRAW_X-1; area.y=DRAW_Y-1; area.w=DRAW_WIDTH+2; area.h=DRAW_HEIGHT+2;
        art.x=view->x-1; art.y=view->y-1;
        art.w=(int)view->width*zoom+2; art.h=(int)view->height*zoom+2;
        n=rect_subtract(&area,&art,parts);
        for(i=0;i<n;++i) video_rect(parts[i].x,parts[i].y,parts[i].w,parts[i].h,UI_DESK);
        video_rect(art.x,art.y,art.w,1,8);
        video_rect(art.x,art.y+art.h-1,art.w,1,8);
        video_rect(art.x,art.y,1,art.h,8);
        video_rect(art.x+art.w-1,art.y,1,art.h,8);
    }
    for (y = 0; y < editor->canvas.height; ++y)
        for (x = 0; x < editor->canvas.width; ++x) {
            video_rect(view->x + (int)x * zoom, view->y + (int)y * zoom,
                       cell, cell, editor->canvas.pixels[y * editor->canvas.width + x]);
            if(cell<zoom) {
                video_rect(view->x+(int)x*zoom+cell,view->y+(int)y*zoom,1,zoom,8);
                video_rect(view->x+(int)x*zoom,view->y+(int)y*zoom+cell,cell,1,8);
            }
        }
}

void render_stroke(const Editor *editor)
{
    int x, y, sx, sy, cell, zoom;
    zoom = editor->view.zoom;
    cell = editor->grid && zoom >= 2 ? zoom - 1 : zoom;
    for (y = editor->dirty_y0; y <= editor->dirty_y1; ++y)
        for (x = editor->dirty_x0; x <= editor->dirty_x1; ++x) {
            sx = editor->view.x + x * zoom;
            sy = editor->view.y + y * zoom;
            video_rect(sx, sy, cell, cell,
                       editor->canvas.pixels[(unsigned)y * editor->canvas.width + x]);
            if(cell<zoom) {
                video_rect(sx+cell,sy,1,zoom,8); video_rect(sx,sy+cell,cell,1,8);
            }
        }
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
    char text[54];
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
void render_ui_change(const Editor *editor,int tool,unsigned char fg,unsigned char bg)
{
    int i,x,y;
    unsigned index;
    char text[12];
    static const char *labels[4]={"Pen","Ers","Fil","Pck"};
    if(tool!=editor->tool) for(i=0;i<4;++i) if(i==tool || i==editor->tool) {
        y=TOOL_Y+i*TOOL_STEP; cursor_protect(TOOL_X,y,TOOL_WIDTH,TOOL_HEIGHT);
        video_rect(TOOL_X,y,TOOL_WIDTH,TOOL_HEIGHT,i==editor->tool?UI_BAR:UI_DESK);
        video_text(TOOL_TEXT_X,y+TOOL_TEXT_Y,labels[i],i==editor->tool?UI_INK:UI_WHITE);
    }
    if(fg!=editor->foreground) for(i=0;i<2;++i) {
        index=i?editor->foreground:fg;
        x=PALETTE_X+(int)(index%16)*PALETTE_CELL_W;
        y=PALETTE_Y+(int)(index/16)*PALETTE_CELL_H;
        cursor_protect(x,y,PALETTE_CELL_W,PALETTE_CELL_H);
        video_rect(x,y,PALETTE_CELL_W,PALETTE_CELL_H,i?UI_WHITE:UI_INK);
        video_rect(x+1,y+1,PALETTE_CELL_W-2,PALETTE_CELL_H-2,(unsigned char)index);
    }
    if(fg!=editor->foreground) {
        cursor_protect(PALETTE_LABEL_X,FG_Y-1,UI_WIDTH-PALETTE_LABEL_X,SWATCH_HEIGHT);
        sprintf(text,"FG %03u",(unsigned)editor->foreground); video_text_bg(PALETTE_LABEL_X,FG_Y,text,UI_WHITE,UI_DESK);
        video_rect(SWATCH_X+1,FG_Y,SWATCH_WIDTH-2,FONT_HEIGHT,editor->foreground);
    }
    if(bg!=editor->background) {
        cursor_protect(PALETTE_LABEL_X,BG_Y-1,UI_WIDTH-PALETTE_LABEL_X,SWATCH_HEIGHT);
        sprintf(text,"BG %03u",(unsigned)editor->background); video_text_bg(PALETTE_LABEL_X,BG_Y,text,UI_WHITE,UI_DESK);
        video_rect(SWATCH_X+1,BG_Y,SWATCH_WIDTH-2,FONT_HEIGHT,editor->background);
    }
}
