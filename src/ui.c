#include "ui.h"
#include "command.h"
#include "workpal.h"
void ui_init(UiState *ui)
{ menu_init(&ui->menu); ui->flyout.group=ui->flyout.hover=-1; ui->blocked=0; workpal_init(); }
int ui_input(UiState *ui,const Editor *e,int key,const MouseState *mouse,int *command)
{
    int consumed,group,slot,n;
    unsigned pressed;
    pressed=mouse->buttons & ~ui->menu.buttons;
    consumed=menu_key(&ui->menu,key,command);
    group=toolbar_hit(mouse->x,mouse->y);
    if(key<0 && (pressed&2) && group>=0) {
        ui->menu.active=0;
        ui->flyout.group=ui->flyout.group==group?-1:group;
        ui->flyout.hover=-1; *command=0; consumed=1;
    } else if(consumed && key>=0) {
        ui->flyout.group=-1;
    } else if(ui->flyout.group>=0 && !ui->menu.active) {
        n=flyout_count(ui->flyout.group);
        slot=flyout_hit(ui->flyout.group,mouse->x,mouse->y);
        if(key==KEY_LEFT || key==KEY_RIGHT)
            ui->flyout.hover=ui->flyout.hover<0?(key==KEY_LEFT?n-1:0):
                (ui->flyout.hover+n+(key==KEY_LEFT?-1:1))%n;
        else if(key<0) ui->flyout.hover=slot;
        if(key==27 || pressed || key==13) {
            if((pressed&1) && slot>=0) *command=tool_command(flyout_tool(ui->flyout.group,slot));
            if(key==13 && ui->flyout.hover>=0) *command=tool_command(flyout_tool(ui->flyout.group,ui->flyout.hover));
            ui->flyout.group=-1;
        }
        consumed=1;
    } else {
        consumed=menu_mouse(&ui->menu,mouse->x,mouse->y,mouse->buttons,command);
        if(ui->menu.active) ui->flyout.group=-1;
        if(!consumed && pressed && group>=0) {
            *command=tool_command(e->group_tool[group]); consumed=1;
        }
        if(!consumed && pressed && workpal_more(mouse->x,mouse->y)) {
            *command=pressed&1?CMD_PALETTE:CMD_BG; consumed=1;
        }
        if(!consumed && (pressed&1)) {
            *command=nav_hit(e->history.active,e->document->frames,mouse->x,mouse->y);
            if(*command) consumed=1;
        }
    }
    ui->menu.buttons=mouse->buttons;
    if(consumed) ui->blocked=1;
    else *command=keyboard_command(key);
    return consumed;
}
void ui_resume(UiState *ui,const MouseState *mouse)
{ ui->menu.buttons=mouse->buttons; ui->flyout.group=-1; ui->blocked=1; }
int ui_pointer(UiState *ui,const MouseState *mouse,int consumed,int was_active)
{
    if(ui->blocked || ui->menu.active || ui->flyout.group>=0 || was_active) {
        if(!mouse->buttons && !ui->menu.active && ui->flyout.group<0) ui->blocked=0;
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
    if(!open || ui->menu.active || ui->flyout.group>=0 || ui->blocked || !(m->buttons&1) ||
       m->x<DRAW_X-1 || m->x>=UI_RIGHT_X || m->y<TIMELINE_Y || m->y>=UI_STATUS_Y) return 0;
    *cmd=0;
    if(m->y<TIMELINE_Y+12) {
        slot=(m->x-DRAW_X)/(DRAW_WIDTH/9);
        if(slot>=0 && slot<8) *cmd=commands[slot];
        else *cmd=CMD_TIMELINE;
    } else {
        slot=(m->x-DRAW_X)/TIMELINE_CELL;
        if(slot>=0 && (unsigned)slot<TIMELINE_SLOTS && first+(unsigned)slot<count)
            *cmd=CMD_SELECT_FRAME+(int)first+slot;
    }
    ui->blocked=1; return 1;
}
