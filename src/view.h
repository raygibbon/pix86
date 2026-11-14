#ifndef PIX86_VIEW_H
#define PIX86_VIEW_H
#include "canvas.h"
#define DRAW_X 24
#define DRAW_Y 14
#define DRAW_WIDTH 222
#define DRAW_HEIGHT 176
#define PALETTE_X 254
#define PALETTE_Y 24
#define PALETTE_CELL_W 4
#define PALETTE_CELL_H 6

#define TOOL_X 1
#define TOOL_Y 18
#define TOOL_WIDTH 20
#define TOOL_HEIGHT 12
#define TOOL_STEP 14

typedef struct View {
    int x, y, zoom;
    unsigned width, height;
} View;
int view_max_zoom(const Canvas *canvas);
int view_set(View *view, const Canvas *canvas, int zoom);
int view_map(const View *view, int screen_x, int screen_y, int *x, int *y);
int toolbar_hit(int x, int y);
int palette_hit(int screen_x, int screen_y, unsigned char *index);
#endif
