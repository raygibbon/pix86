#include <stddef.h>
#include "canvas.h"

int canvas_init(Canvas *canvas, unsigned width, unsigned height,
                unsigned char *storage, unsigned capacity)
{
    unsigned long size;
    if (canvas == NULL)
        return 0;
    canvas->width = canvas->height = 0;
    canvas->pixels = NULL;
    size = (unsigned long)width * height;
    if (storage == NULL || width == 0 || height == 0 ||
        size > 32767UL || size > capacity)
        return 0;
    canvas->width = width;
    canvas->height = height;
    canvas->pixels = storage;
    return 1;
}

void canvas_clear(Canvas *canvas, unsigned char colour)
{
    unsigned i, count;
    if (canvas == NULL || canvas->pixels == NULL)
        return;
    count = canvas->width * canvas->height;
    for (i = 0; i < count; ++i)
        canvas->pixels[i] = colour;
}

int canvas_get(const Canvas *canvas, unsigned x, unsigned y,
               unsigned char *colour)
{
    if (canvas == NULL || canvas->pixels == NULL || colour == NULL ||
        x >= canvas->width || y >= canvas->height)
        return 0;
    *colour = canvas->pixels[y * canvas->width + x];
    return 1;
}

int canvas_set(Canvas *canvas, unsigned x, unsigned y, unsigned char colour)
{
    if (canvas == NULL || canvas->pixels == NULL ||
        x >= canvas->width || y >= canvas->height)
        return 0;
    canvas->pixels[y * canvas->width + x] = colour;
    return 1;
}
