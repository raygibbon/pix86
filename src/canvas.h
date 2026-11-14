#ifndef PIX86_CANVAS_H
#define PIX86_CANVAS_H

/* Caller owns storage; one byte per pixel, palette indices only. */
typedef struct Canvas {
    unsigned width;
    unsigned height;
    unsigned char *pixels;
} Canvas;

/* M1 caps a canvas at 32767 bytes for safe 16-bit near objects. */
int canvas_init(Canvas *canvas, unsigned width, unsigned height,
                unsigned char *storage, unsigned capacity);
void canvas_clear(Canvas *canvas, unsigned char colour);
int canvas_get(const Canvas *canvas, unsigned x, unsigned y,
               unsigned char *colour);
#endif
