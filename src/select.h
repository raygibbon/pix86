#ifndef PIX86_SELECT_H
#define PIX86_SELECT_H
#define SEL_NONE 0
#define SEL_RECT 1
#define SEL_ACTIVE 2
#define SEL_FLOAT 3
#define SEL_DRAG 4
typedef struct Selection {
    int state,x,y,w,h,dx,dy,anchor_x,anchor_y;
    int start_x,start_y,old_dx,old_dy;
} Selection;
struct Editor;
void selection_reset(struct Editor *e);
int selection_floating(const struct Editor *e);
int selection_key(struct Editor *e,int key,unsigned *flags);
unsigned selection_mouse(struct Editor *e,int x,int y,unsigned buttons,unsigned pressed);
void selection_sample(const struct Editor *e,int x,int y,unsigned char *colour,int *transparent);
#endif
