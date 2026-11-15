#ifndef PIX86_VIEW_H
#define PIX86_VIEW_H
#include "canvas.h"
#include "layout.h"

typedef struct View {
    int x, y, zoom;
    unsigned width, height;
} View;
int view_fit(View *view,const Canvas *canvas,int timeline,int zoom);
int view_max_zoom(const Canvas *canvas);
int view_set(View *view, const Canvas *canvas, int zoom);
int view_map(const View *view, int screen_x, int screen_y, int *x, int *y);
int toolbar_hit(int x, int y);
int palette_hit(int screen_x, int screen_y, unsigned char *index);
#endif
