#include "menu.h"
#include "keyboard.h"
#include <ctype.h>
#include <string.h>
static const char *titles[MENU_COUNT] = {"File", "Edit", "View", "Frame", "Options", "Help"};
static const int counts[MENU_COUNT] = {6, 3, 6, 9, 3, 3};
static const MenuItem items[MENU_COUNT][10] = {
    {{"New", "N", CMD_NEW}, {"Open", "F3", CMD_OPEN}, {"Save", "F2", CMD_SAVE},
     {"Save As", "Shift-F2", CMD_SAVE_AS}, {"Export", "", CMD_EXPORT}, {"Exit", "Esc", CMD_EXIT}},
    {{"Undo", "Ctrl-Z", CMD_UNDO}, {"Redo", "Ctrl-Y", CMD_REDO}, {"Clear", "C", CMD_CLEAR}},
    {{"Zoom in", "+", CMD_ZOOM_IN}, {"Zoom out", "-", CMD_ZOOM_OUT}, {"Grid", "G", CMD_GRID}, {"8x8 guides", "8", CMD_GUIDE8}, {"16x16 guides", "6", CMD_GUIDE16}, {"Timeline", "Tab", CMD_TIMELINE}},
    {{"Add blank", "Ins", CMD_FRAME_ADD}, {"Duplicate", "D", CMD_FRAME_DUP},
     {"Delete", "Del", CMD_FRAME_DELETE}, {"Previous", ",", CMD_FRAME_PREV},
     {"Next", ".", CMD_FRAME_NEXT}, {"Play/stop", "Space", CMD_PLAY},
     {"Faster", "(", CMD_FASTER}, {"Slower", ")", CMD_SLOWER}, {"Onion skin", "", 0}},
    {{"Colours", "A", CMD_PALETTE}, {"Background", "B", CMD_BG}, {"Grid", "G", CMD_GRID}},
    {{"Controls", "", CMD_HELP}, {"About", "", CMD_ABOUT}, {"Font sample", "", CMD_FONT}}
};
int menu_width(int heading) { return font_width(titles[heading])+10; }
int menu_heading_left(int heading)
{
    int x,i; x=0;
    for(i=0;i<heading;++i) x+=menu_width(i);
    return x;
}
int menu_drop_width(int heading)
{
    int i,names,shortcuts,w;
    names=shortcuts=0;
    for(i=0;i<counts[heading];++i) {
        w=font_width(items[heading][i].label); if(w>names) names=w;
        w=font_width(items[heading][i].shortcut); if(w>shortcuts) shortcuts=w;
    }
    w=names+shortcuts+12+(shortcuts?2*FONT_WIDTH:0);
    return w<menu_width(heading)?menu_width(heading):w;
}
int menu_left(int heading)
{
    int x,w; x=menu_heading_left(heading); w=menu_drop_width(heading);
    return x+w>UI_WIDTH?UI_WIDTH-w:x;
}
void menu_bounds(const Menu *menu,Rect *r)
{
    r->x=menu_left(menu->heading); r->y=MENU_BAR_HEIGHT;
    r->w=menu->active?menu_drop_width(menu->heading):0;
    r->h=menu->active?counts[menu->heading]*MENU_ROW_HEIGHT+2:0;
}
int menu_items(int heading) { return counts[heading]; }
const char *menu_title(int heading) { return titles[heading]; }
const MenuItem *menu_item(int heading, int item) { return &items[heading][item]; }
void menu_init(Menu *menu)
{
    menu->active = menu->heading = menu->item = 0;
    menu->buttons = 0;
    memset(menu->remember,0,sizeof(menu->remember));
}
static void open_menu(Menu *menu, int heading)
{
    if(menu->active) menu->remember[menu->heading]=(unsigned char)menu->item;
    menu->active = 1; menu->heading = heading; menu->item = menu->remember[heading];
}
static void activate(Menu *menu, int *command)
{
    *command = items[menu->heading][menu->item].command;
    if (*command) { menu->remember[menu->heading]=(unsigned char)menu->item; menu->active = 0; }
}
int menu_key(Menu *menu, int key, int *command)
{
    int heading;
    *command = 0;
    if (key == KEY_MENU) {
        if (menu->active) menu->active = 0;
        else open_menu(menu, 0);
        return 1;
    }
    heading = keyboard_menu(key);
    if (heading >= 0) { open_menu(menu, heading); return 1; }
    if (key == KEY_F10) {
        if(menu->active) menu->active=0;
        else open_menu(menu,menu->heading);
        return 1;
    }
    if (!menu->active) return 0;
    if (key == 27) menu->active = 0;
    if (key == KEY_LEFT || key == KEY_RIGHT)
        open_menu(menu, (menu->heading + (key == KEY_LEFT ? 5 : 1)) % MENU_COUNT);
    if (key == KEY_UP || key == KEY_DOWN)
        menu->item = (menu->item + counts[menu->heading] + (key == KEY_UP ? -1 : 1)) % counts[menu->heading];
    if(key>=32 && key<=126) {
        int i;
        for(i=1;i<=counts[menu->heading];++i) {
            int row; row=(menu->item+i)%counts[menu->heading];
            if(tolower((unsigned char)items[menu->heading][row].label[0])==tolower(key)) {
                menu->item=row; break;
            }
        }
    }
    menu->remember[menu->heading]=(unsigned char)menu->item;
    if (key == 13) activate(menu, command);
    return 1;
}
int menu_mouse(Menu *menu, int x, int y, unsigned buttons, int *command)
{
    int i, row, consumed;
    unsigned pressed;
    *command = 0;
    pressed = buttons & ~menu->buttons;
    menu->buttons = buttons;
    consumed = menu->active;
    if (!(pressed & 1)) return consumed;
    if (y >= 0 && y < MENU_BAR_HEIGHT) {
        for (i = 0; i < MENU_COUNT; ++i)
            if (x >= menu_heading_left(i) && x < menu_heading_left(i) + menu_width(i)) {
                open_menu(menu, i); return 1;
            }
    }
    if (!menu->active) return 0;
    row = (y - MENU_ROW_TOP) / MENU_ROW_HEIGHT;
    if (x >= menu_left(menu->heading) && x < menu_left(menu->heading) + menu_drop_width(menu->heading) &&
        y >= MENU_ROW_TOP && row < counts[menu->heading]) {
        menu->item = row; activate(menu, command);
    } else menu->active = 0;
    return 1;
}
