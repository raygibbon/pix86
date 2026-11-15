#include "ui.h"
#include "command.h"
#include "workpal.h"
void ui_init(UiState *ui) { menu_init(&ui->menu); ui->blocked=0; workpal_init(); }
int ui_input(UiState *ui,int key,const MouseState *mouse,int *command)
{
    int consumed;
    unsigned pressed;
    pressed=mouse->buttons & ~ui->menu.buttons;
    consumed=menu_key(&ui->menu,key,command);
    if(!consumed || key<0)
        consumed=menu_mouse(&ui->menu,mouse->x,mouse->y,mouse->buttons,command);
    else ui->menu.buttons=mouse->buttons;
    if(!consumed && pressed && workpal_more(mouse->x,mouse->y)) {
        *command=pressed&1?CMD_PALETTE:CMD_BG; consumed=1;
    }
    if(consumed) ui->blocked=1;
    else *command=keyboard_command(key);
    return consumed;
}
void ui_resume(UiState *ui,const MouseState *mouse)
{ ui->menu.buttons=mouse->buttons; ui->blocked=1; }
int ui_pointer(UiState *ui,const MouseState *mouse,int consumed,int was_active)
{
    if(ui->blocked || ui->menu.active || was_active) {
        if(!mouse->buttons && !ui->menu.active) ui->blocked=0;
        return 0;
    }
    return !consumed;
}

/* Call after overlays and before editor pointer routing. A timeline press and
   its held release remain owned by the UI, even if the pointer leaves it. */
int ui_timeline(UiState *ui,int open,unsigned first,unsigned count,const MouseState *m,int *cmd)
{
    int slot;
    static const int commands[8]={CMD_FRAME_PREV,CMD_FRAME_NEXT,CMD_FRAME_ADD,
        CMD_FRAME_DUP,CMD_FRAME_DELETE,CMD_PLAY,CMD_SCROLL_LEFT,CMD_SCROLL_RIGHT};
    if(!open || ui->menu.active || ui->blocked || !(m->buttons&1) ||
       m->x<DRAW_X-1 || m->y<TIMELINE_Y || m->y>=UI_STATUS_Y) return 0;
    *cmd=0;
    if(m->y<TIMELINE_Y+12) {
        slot=(m->x-DRAW_X)/30;
        if(slot>=0 && slot<8) *cmd=commands[slot];
        else *cmd=CMD_TIMELINE;
    } else {
        slot=(m->x-DRAW_X)/TIMELINE_CELL;
        if(slot>=0 && slot<TIMELINE_SLOTS && first+(unsigned)slot<count)
            *cmd=CMD_SELECT_FRAME+(int)first+slot;
    }
    ui->blocked=1; return 1;
}
