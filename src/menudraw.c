#include "menu.h"
#include "video.h"
#include "cursor.h"
#include "theme.h"
#include "uipaint.h"
static void heading(const Menu *menu,int i)
{
    int x,w,selected;
    unsigned char ink,paper;
    static const int mnemonic[MENU_COUNT]={0,0,0,1,0,0};
    x=menu_heading_left(i); w=menu_width(i);
    selected=menu->active && menu->heading==i;
    ink=selected?UI_WHITE:UI_INK; paper=selected?UI_FOCUS:UI_BAR;
    cursor_protect(x,0,w,MENU_BAR_HEIGHT);
    video_rect(x,0,4,FONT_HEIGHT,paper);
    video_text_bg(x+4,0,menu_title(i),ink,paper);
    video_rect(x+w-4,0,4,FONT_HEIGHT,paper);
    video_rect(x,FONT_HEIGHT,w,1,paper);
    video_rect(x+4+mnemonic[i]*FONT_WIDTH,FONT_HEIGHT,5,1,ink);
}
static void row(const Menu *menu,int i)
{
    int x,y,w,right,end;
    unsigned char paper,ink;
    const MenuItem *item;
    x=menu_left(menu->heading); y=MENU_ROW_TOP+i*MENU_ROW_HEIGHT;
    w=menu_drop_width(menu->heading); item=menu_item(menu->heading,i);
    paper=i==menu->item?UI_SELECT:UI_PAPER;
    ink=!item->command?UI_DIM:i==menu->item?UI_WHITE:UI_INK;
    cursor_protect(x+1,y,w-2,MENU_ROW_HEIGHT);
    if(!item->label[0]) {
        video_rect(x+1,y,w-2,MENU_ROW_HEIGHT,paper);
        video_rect(x+4,y+FONT_HEIGHT/2,w-8,1,UI_DIM);
        return;
    }
    end=x+4+font_width(item->label); right=x+w-4-font_width(item->shortcut);
    video_rect(x+1,y,3,FONT_HEIGHT,paper);
    video_text_bg(x+4,y,item->label,ink,paper);
    if(item->accelerator) {
        const char *p;
        int index;
        for(p=item->label,index=0;*p;++p,++index) {
            int letter=*p;
            if(letter>='a' && letter<='z') letter-=('a'-'A');
            if(letter==item->accelerator) break;
        }
        if(*p) video_rect(x+4+index*FONT_WIDTH,y+FONT_HEIGHT-1,FONT_WIDTH-1,1,ink);
    }
    video_rect(end,y,right-end,FONT_HEIGHT,paper);
    video_text_bg(right,y,item->shortcut,ink,paper);
    video_rect(x+w-4,y,3,FONT_HEIGHT,paper);
    video_rect(x+1,y+FONT_HEIGHT,w-2,1,paper);
}
void menu_draw_bar(const Menu *menu)
{
    int i,x;
    cursor_protect(0,0,UI_WIDTH,MENU_BAR_HEIGHT);
    video_rect(0,0,UI_WIDTH,MENU_BAR_HEIGHT,UI_BAR);
    for(i=0;i<MENU_COUNT;++i) heading(menu,i);
    x=UI_WIDTH-font_width("Ctrl+\\")-4;
    video_text_bg(x,0,"Ctrl+\\",UI_INK,UI_BAR);
}
void menu_draw(const Menu *menu)
{
    Rect r;
    int i;
    menu_draw_bar(menu);
    if(!menu->active) return;
    menu_bounds(menu,&r); cursor_protect(r.x,r.y,r.w,r.h);
    /* Border strips never black-fill the interior before its final contents. */
    ui_border(&r,UI_INK);
    for(i=0;i<menu_items(menu->heading);++i) row(menu,i);
}
void menu_draw_change(const Menu *menu,const Menu *previous)
{
    Rect r;
    int i;
    if(menu->active!=previous->active || menu->heading!=previous->heading) {
        heading(menu,previous->heading);
        if(menu->heading!=previous->heading) heading(menu,menu->heading);
        if(menu->active) {
            menu_bounds(menu,&r); cursor_protect(r.x,r.y,r.w,r.h);
            ui_border(&r,UI_INK);
            for(i=0;i<menu_items(menu->heading);++i) row(menu,i);
        }
    } else if(menu->active && menu->item!=previous->item) {
        row(menu,previous->item); row(menu,menu->item);
    }
}
