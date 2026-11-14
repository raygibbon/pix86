#ifndef PIX86_CANVAS_H
#define PIX86_CANVAS_H

/* Caller owns storage; one byte per pixel, palette indices only. */
typedef struct Canvas {
    unsigned width;
    unsigned height;
    unsigned char *pixels;
} Canvas;

/* Canvases are capped at 32767 bytes for safe 16-bit near objects. */
int canvas_init(Canvas *canvas, unsigned width, unsigned height,
                unsigned char *storage, unsigned capacity);
int canvas_set(Canvas *canvas, unsigned x, unsigned y, unsigned char colour);
void canvas_clear(Canvas *canvas, unsigned char colour);
int canvas_get(const Canvas *canvas, unsigned x, unsigned y,
               unsigned char *colour);
#endif
