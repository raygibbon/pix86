#include "menu.h"
#include "keyboard.h"
static const char *titles[MENU_COUNT] = {"File", "Edit", "View", "Frame", "Options", "Help"};
static const int counts[MENU_COUNT] = {7, 4, 4, 3, 4, 4};
static const MenuItem items[MENU_COUNT][7] = {
    {{"New", "N", 'N', CMD_NEW}, {"Open", "F3", 'O', CMD_OPEN}, {"", "", 0, 0},
     {"Save", "F2", 'S', CMD_SAVE}, {"Save As", "Shift-F2", 'A', CMD_SAVE_AS},
     {"", "", 0, 0}, {"Exit", "Esc", 'X', CMD_EXIT}},
    {{"Undo", "Ctrl-Z", 'U', CMD_UNDO}, {"Redo", "Ctrl-Y", 'R', CMD_REDO},
     {"", "", 0, 0}, {"Clear", "C", 'C', CMD_CLEAR}},
    {{"Zoom in", "+", 'I', CMD_ZOOM_IN}, {"Zoom out", "-", 'O', CMD_ZOOM_OUT},
     {"", "", 0, 0}, {"Grid", "G", 'G', CMD_GRID}},
    {{"Add frame", "", 'A', 0}, {"Duplicate", "", 'D', 0}, {"Playback", "", 'P', 0}},
    {{"Colours", "A", 'C', CMD_PALETTE}, {"Background", "B", 'B', CMD_BG},
     {"", "", 0, 0}, {"Grid", "G", 'G', CMD_GRID}},
    {{"Controls", "", 'C', CMD_HELP}, {"About", "", 'A', CMD_ABOUT},
     {"", "", 0, 0}, {"Font sample", "", 'F', CMD_FONT}}
};
static int selectable(int heading, int item)
{ return items[heading][item].label[0] != '\0'; }
static int first_item(int heading)
{
    int i;
    for (i=0;i<counts[heading];++i) if(selectable(heading,i)) return i;
    return 0;
}
static void remember(Menu *menu)
{
    if(selectable(menu->heading,menu->item)) menu->item_memory[menu->heading]=menu->item;
}
static int next_item(int heading,int item,int direction)
{
    int i;
    for(i=0;i<counts[heading];++i) {
        item=(item+counts[heading]+direction)%counts[heading];
        if(selectable(heading,item)) return item;
    }
    return first_item(heading);
}
int menu_width(int heading) { return font_width(titles[heading])+8; }
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
    w=names+shortcuts+8+(shortcuts?FONT_WIDTH:0);
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
    int i;
    menu->active = menu->heading = 0;
    for(i=0;i<MENU_COUNT;++i) menu->item_memory[i]=first_item(i);
    menu->item=menu->item_memory[0];
    menu->buttons = 0;
}
static void open_menu(Menu *menu, int heading)
{
    if(menu->active) remember(menu);
    menu->heading = heading;
    menu->item = selectable(heading,menu->item_memory[heading]) ?
                 menu->item_memory[heading] : first_item(heading);
    menu->active = 1;
}
static void activate(Menu *menu, int *command)
{
    remember(menu);
    *command = items[menu->heading][menu->item].command;
    if (*command) menu->active = 0;
}
int menu_key(Menu *menu, int key, int *command)
{
    int heading;
    *command = 0;
    if (key == KEY_MENU) {
        if (menu->active) { remember(menu); menu->active = 0; }
        else open_menu(menu, 0);
        return 1;
    }
    heading = keyboard_menu(key);
    if (heading >= 0) { open_menu(menu, heading); return 1; }
    if (key == KEY_F10) {
        if(menu->active) { remember(menu); menu->active=0; }
        else open_menu(menu,menu->heading);
        return 1;
    }
    if (!menu->active) return 0;
    if (key == 27) { remember(menu); menu->active = 0; }
    if (key == KEY_LEFT || key == KEY_RIGHT)
        open_menu(menu, (menu->heading + (key == KEY_LEFT ? 5 : 1)) % MENU_COUNT);
    if (key == KEY_UP || key == KEY_DOWN)
        menu->item = next_item(menu->heading,menu->item,key==KEY_UP?-1:1);
    if(key>='a' && key<='z') key-=('a'-'A');
    if(key>='A' && key<='Z') {
        int i;
        for(i=0;i<counts[menu->heading];++i)
            if(selectable(menu->heading,i) && items[menu->heading][i].accelerator==key) {
                menu->item=i;
                activate(menu,command);
                return 1;
            }
    }
    if (key == 13) activate(menu, command);
    if(menu->active) remember(menu);
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
        if(selectable(menu->heading,row)) {
            menu->item = row;
            activate(menu, command);
            if(menu->active) remember(menu);
        }
    } else { remember(menu); menu->active = 0; }
    return 1;
}
