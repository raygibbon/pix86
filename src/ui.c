#include "ui.h"
#include "command.h"
void ui_init(UiState *ui) { menu_init(&ui->menu); ui->blocked=0; }
int ui_input(UiState *ui,int key,const MouseState *mouse,int *command)
{
    int consumed;
    consumed=menu_key(&ui->menu,key,command);
    if(!consumed || key<0)
        consumed=menu_mouse(&ui->menu,mouse->x,mouse->y,mouse->buttons,command);
    else ui->menu.buttons=mouse->buttons;
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
