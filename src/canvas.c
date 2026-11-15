#include <stddef.h>
#include "canvas.h"

int canvas_init(Canvas *canvas, unsigned width, unsigned height,
                unsigned char *storage, unsigned capacity)
{
    unsigned long size;
    if (canvas == NULL)
        return 0;
    canvas->width = canvas->height = 0;
    canvas->pixels = NULL; canvas->mask=NULL;
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
    for (i = 0; i < count; ++i) canvas->pixels[i] = colour;
    if(canvas->mask) for(i=0;i<(count+7)/8;++i) canvas->mask[i]=0;
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
    canvas_mask_set(canvas,x,y,0);
    return 1;
}

int canvas_transparent(const Canvas *c,unsigned x,unsigned y)
{
    unsigned pos;
    if(!c || !c->mask || x>=c->width || y>=c->height) return 0;
    pos=y*c->width+x; return (c->mask[pos/8]>>(pos%8))&1;
}
void canvas_mask_set(Canvas *c,unsigned x,unsigned y,int transparent)
{
    unsigned pos;
    unsigned char bit;
    if(!c || !c->mask || x>=c->width || y>=c->height) return;
    pos=y*c->width+x; bit=(unsigned char)(1U<<(pos%8));
    if(transparent) c->mask[pos/8]|=bit;
    else c->mask[pos/8]&=(unsigned char)~bit;
}
