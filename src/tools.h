#ifndef PIX86_TOOLS_H
#define PIX86_TOOLS_H
#include "canvas.h"
#define TOOLS_MAX_PIXELS 4096

/* Endpoints must be inside the canvas. Returns changed-pixel count. */
unsigned tools_stroke(Canvas *canvas,int x0,int y0,int x1,int y1,unsigned char colour,int transparent);
unsigned tools_pencil(Canvas *canvas, int x0, int y0, int x1, int y1,
                      unsigned char colour);
unsigned tools_erase(Canvas *canvas, int x0, int y0, int x1, int y1,
                     unsigned char background);
/* Four-connected fill; caller supplies 4096 unsigned-short queue entries. */
unsigned tools_fill(Canvas *canvas, unsigned x, unsigned y,
                    unsigned char colour, unsigned short *queue);
int tools_pick(const Canvas *canvas, unsigned x, unsigned y,
               unsigned char *colour);
#endif
