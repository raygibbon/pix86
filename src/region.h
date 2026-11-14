#ifndef PIX86_REGION_H
#define PIX86_REGION_H
/* Screen-bounded, half-open rectangles; no allocation or backing framebuffer. */
typedef struct Rect { int x, y, w, h; } Rect;
int rect_overlap(const Rect *a, const Rect *b);
/* Disjoint pieces of a outside b, at most four; inputs are screen-bounded. */
int rect_subtract(const Rect *a, const Rect *b, Rect *out);
#endif
