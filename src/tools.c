#include <stddef.h>
#include <stdlib.h>
#include "tools.h"

unsigned tools_stroke_collect(Canvas *canvas,int x0,int y0,int x1,int y1,
                              unsigned char colour,int transparent,
                              unsigned short *changed_cells)
{
    int dx, dy, sx, sy, error, twice;
    unsigned changed;
    unsigned char old;
    if (x0 < 0 || y0 < 0 || x1 < 0 || y1 < 0 ||
        !canvas_get(canvas, (unsigned)x0, (unsigned)y0, &old) ||
        !canvas_get(canvas, (unsigned)x1, (unsigned)y1, &old) ||
        canvas->width > 64 || canvas->height > 64)
        return 0;
    dx = abs(x1 - x0);
    dy = abs(y1 - y0);
    sx = x0 < x1 ? 1 : -1;
    sy = y0 < y1 ? 1 : -1;
    error = dx - dy;
    changed = 0;
    for (;;) {
        if (canvas_get(canvas, (unsigned)x0, (unsigned)y0, &old) &&
            (transparent?!canvas_transparent(canvas,(unsigned)x0,(unsigned)y0):
             old!=colour || canvas_transparent(canvas,(unsigned)x0,(unsigned)y0))) {
            if(transparent) canvas_mask_set(canvas,(unsigned)x0,(unsigned)y0,1);
            else canvas_set(canvas, (unsigned)x0, (unsigned)y0, colour);
            if(changed_cells)
                changed_cells[changed]=(unsigned short)((unsigned)y0*canvas->width+(unsigned)x0);
            ++changed;
        }
        if (x0 == x1 && y0 == y1)
            break;
        twice = error * 2;
        if (twice > -dy) { error -= dy; x0 += sx; }
        if (twice < dx) { error += dx; y0 += sy; }
    }
    return changed;
}

unsigned tools_stroke(Canvas *canvas,int x0,int y0,int x1,int y1,
                      unsigned char colour,int transparent)
{
    return tools_stroke_collect(canvas,x0,y0,x1,y1,colour,transparent,NULL);
}

unsigned tools_pencil(Canvas *c,int x0,int y0,int x1,int y1,unsigned char colour)
{ return tools_stroke(c,x0,y0,x1,y1,colour,0); }

unsigned tools_erase(Canvas *canvas, int x0, int y0, int x1, int y1,
                     unsigned char background)
{
    return tools_pencil(canvas, x0, y0, x1, y1, background);
}

unsigned tools_fill(Canvas *canvas, unsigned x, unsigned y,
                    unsigned char colour, unsigned short *queue)
{
    unsigned char old;
    unsigned head, tail, pos, nx, ny, neighbor, direction;
    unsigned count;
    int transparent;
    transparent=canvas_transparent(canvas,x,y);
    if (queue == NULL || !canvas_get(canvas, x, y, &old) || (!transparent && old == colour) ||
        (unsigned long)canvas->width * canvas->height > TOOLS_MAX_PIXELS)
        return 0;
    count = canvas->width * canvas->height;
    head = 0;
    tail = 1;
    queue[0] = (unsigned short)(y * canvas->width + x);
    canvas_set(canvas, x, y, colour);
    /* Mark on enqueue: every pixel enters the bounded queue at most once. */
    while (head < tail) {
        pos = queue[head++];
        nx = pos % canvas->width;
        ny = pos / canvas->width;
        for (direction = 0; direction < 4; ++direction) {
            if (direction == 0) {
                if (nx == 0) continue;
                neighbor = pos - 1;
            } else if (direction == 1) {
                if (nx + 1 == canvas->width) continue;
                neighbor = pos + 1;
            } else if (direction == 2) {
                if (ny == 0) continue;
                neighbor = pos - canvas->width;
            } else {
                if (ny + 1 == canvas->height) continue;
                neighbor = pos + canvas->width;
            }
            if (neighbor < count && canvas_transparent(canvas,neighbor%canvas->width,neighbor/canvas->width)==transparent &&
                (transparent || canvas->pixels[neighbor] == old)) {
                canvas_set(canvas,neighbor%canvas->width,neighbor/canvas->width,colour);
                queue[tail++] = (unsigned short)neighbor;
            }
        }
    }
    return tail;
}

int tools_pick(const Canvas *canvas, unsigned x, unsigned y,
               unsigned char *colour)
{
    return canvas_get(canvas, x, y, colour);
}
