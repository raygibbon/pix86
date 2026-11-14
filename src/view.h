#ifndef PIX86_VIEW_H
#define PIX86_VIEW_H
#include "canvas.h"
#define DRAW_X 40
#define DRAW_Y 20
#define DRAW_WIDTH 176
#define DRAW_HEIGHT 168
#define PALETTE_X 224
#define PALETTE_Y 32
#define PALETTE_CELL_W 6
#define PALETTE_CELL_H 9

typedef struct View {
    int x, y, zoom;
    unsigned width, height;
} View;
int view_max_zoom(const Canvas *canvas);
int view_set(View *view, const Canvas *canvas, int zoom);
int view_map(const View *view, int screen_x, int screen_y, int *x, int *y);
int palette_hit(int screen_x, int screen_y, unsigned char *index);
#endif
