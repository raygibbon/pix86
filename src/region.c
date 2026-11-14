#include "region.h"
int rect_overlap(const Rect *a, const Rect *b)
{
    return a->w > 0 && a->h > 0 && b->w > 0 && b->h > 0 &&
           a->x < b->x+b->w && b->x < a->x+a->w &&
           a->y < b->y+b->h && b->y < a->y+a->h;
}
static void piece(Rect *out, int *n, int x, int y, int w, int h)
{
    if(w <= 0 || h <= 0) return;
    out[*n].x=x; out[*n].y=y; out[*n].w=w; out[*n].h=h; ++*n;
}
int rect_subtract(const Rect *a, const Rect *b, Rect *out)
{
    int x0,y0,x1,y1,n;
    n=0;
    if(!rect_overlap(a,b)) { out[0]=*a; return 1; }
    x0=a->x>b->x?a->x:b->x; y0=a->y>b->y?a->y:b->y;
    x1=a->x+a->w<b->x+b->w?a->x+a->w:b->x+b->w;
    y1=a->y+a->h<b->y+b->h?a->y+a->h:b->y+b->h;
    piece(out,&n,a->x,a->y,a->w,y0-a->y);
    piece(out,&n,a->x,y1,a->w,a->y+a->h-y1);
    piece(out,&n,a->x,y0,x0-a->x,y1-y0);
    piece(out,&n,x1,y0,a->x+a->w-x1,y1-y0);
    return n;
}
