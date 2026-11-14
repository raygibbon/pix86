#include <string.h>
#include "uipaint.h"
#include "video.h"
#include "cursor.h"
#include "font.h"
#include "theme.h"
void ui_border(const Rect *r,unsigned char colour)
{
    cursor_protect(r->x,r->y,r->w,r->h);
    video_rect(r->x,r->y,r->w,1,colour);
    video_rect(r->x,r->y+r->h-1,r->w,1,colour);
    video_rect(r->x,r->y,1,r->h,colour);
    video_rect(r->x+r->w-1,r->y,1,r->h,colour);
}
void ui_button(const Rect *r,const char *label,int selected)
{
    char text[7];
    unsigned n,start;
    unsigned char paper,ink;
    paper=selected?UI_SELECT:UI_PAPER; ink=selected?UI_WHITE:UI_INK;
    memset(text,' ',6); text[6]='\0'; n=(unsigned)strlen(label); start=(6-n)/2;
    memcpy(text+start,label,n);
    cursor_protect(r->x,r->y,r->w,r->h);
    video_rect(r->x,r->y,r->w,1,paper);
    video_rect(r->x,r->y+r->h-1,r->w,1,paper);
    video_rect(r->x,r->y+1,4,UI_FONT_HEIGHT,paper);
    video_rect(r->x+r->w-4,r->y+1,4,UI_FONT_HEIGHT,paper);
    video_ui_text_bg(r->x+4,r->y+1,text,ink,paper);
}
