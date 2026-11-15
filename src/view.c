#include <stddef.h>
#include "view.h"
#include "workpal.h"

int view_max_zoom(const Canvas *canvas)
{
    int horizontal, vertical;
    if (canvas == NULL || canvas->width == 0 || canvas->height == 0 ||
        canvas->width > DRAW_WIDTH || canvas->height > DRAW_HEIGHT)
        return 0;
    horizontal = DRAW_WIDTH / canvas->width;
    vertical = DRAW_HEIGHT / canvas->height;
    return horizontal < vertical ? horizontal : vertical;
}

int view_fit(View *view,const Canvas *canvas,int timeline,int zoom)
{
    int height,max,h;
    if(view==NULL || canvas==NULL || !canvas->width || !canvas->height) return 0;
    height=timeline?TIMELINE_Y-DRAW_Y-2:DRAW_HEIGHT;
    max=DRAW_WIDTH/canvas->width; h=height/canvas->height;
    if(h<max) max=h;
    if(zoom==0) zoom=max;
    if(zoom<1 || zoom>max) return 0;
    view->width=canvas->width; view->height=canvas->height; view->zoom=zoom;
    view->x=DRAW_X+(DRAW_WIDTH-(int)canvas->width*zoom)/2;
    view->y=DRAW_Y+(height-(int)canvas->height*zoom)/2;
    return 1;
}
int view_set(View *view,const Canvas *canvas,int zoom)
{ return zoom>0 && view_fit(view,canvas,0,zoom); }

int view_map(const View *view, int screen_x, int screen_y, int *x, int *y)
{
    long dx, dy;
    if (view == NULL || x == NULL || y == NULL || view->zoom < 1)
        return 0;
    *x = *y = -1;
    dx = (long)screen_x - view->x;
    dy = (long)screen_y - view->y;
    if (dx < 0 || dy < 0 || dx >= (long)view->width * view->zoom ||
        dy >= (long)view->height * view->zoom)
        return 0;
    *x = (int)(dx / view->zoom);
    *y = (int)(dy / view->zoom);
    return 1;
}

int palette_hit(int screen_x, int screen_y, unsigned char *index)
{
    int x, y;
    if (index == NULL || screen_x < PALETTE_X || screen_y < PALETTE_Y ||
        screen_x >= PALETTE_X + PALETTE_COLS * PALETTE_CELL_W ||
        screen_y >= PALETTE_Y + PALETTE_ROWS * PALETTE_CELL_H)
        return 0;
    x = (screen_x - PALETTE_X) / PALETTE_CELL_W;
    y = (screen_y - PALETTE_Y) / PALETTE_CELL_H;
    *index = workpal_colour(y * PALETTE_COLS + x);
    return 1;
}

int toolbar_hit(int x, int y)
{
    int row;
    if (x < TOOL_X || x >= TOOL_X + TOOL_WIDTH || y < TOOL_Y) return -1;
    row = (y - TOOL_Y) / TOOL_STEP;
    if (row >= 4 || (y - TOOL_Y) % TOOL_STEP >= TOOL_HEIGHT) return -1;
    return row;
}
