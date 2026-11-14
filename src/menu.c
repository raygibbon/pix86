#include "menu.h"
#include "keyboard.h"
static const char *titles[MENU_COUNT] = {"File", "Edit", "View", "Frame", "Options", "Help"};
static const int starts[MENU_COUNT] = {0, 36, 72, 108, 150, 204};
static const int counts[MENU_COUNT] = {5, 3, 3, 3, 1, 2};
static const MenuItem items[MENU_COUNT][5] = {
    {{"New", "N", 'n'}, {"Open", "F3", KEY_F3}, {"Save", "F2", KEY_F2},
     {"Save As", "Shift-F2", KEY_SAVE_AS}, {"Exit", "Esc", 27}},
    {{"Undo", "Ctrl-Z", 26}, {"Redo", "Ctrl-Y", 25}, {"Clear", "C", 'c'}},
    {{"Zoom in", "+", '+'}, {"Zoom out", "-", '-'}, {"Grid", "G", 'g'}},
    {{"Add frame", "", 0}, {"Duplicate", "", 0}, {"Playback", "", 0}},
    {{"Grid", "G", 'g'}},
    {{"Controls", "", CMD_HELP}, {"About", "", CMD_ABOUT}}
};
int menu_heading_left(int heading) { return starts[heading]; }
int menu_left(int heading) { return starts[heading] > 172 ? 172 : starts[heading]; }
int menu_width(int heading) { return (heading + 1 < MENU_COUNT ? starts[heading+1] : 240) - starts[heading]; }
int menu_items(int heading) { return counts[heading]; }
const char *menu_title(int heading) { return titles[heading]; }
const MenuItem *menu_item(int heading, int item) { return &items[heading][item]; }
void menu_init(Menu *menu)
{
    menu->active = menu->heading = menu->item = 0;
    menu->buttons = 0;
}
static void open_menu(Menu *menu, int heading)
{
    menu->active = 1; menu->heading = heading; menu->item = 0;
}
static void activate(Menu *menu, int *command)
{
    *command = items[menu->heading][menu->item].command;
    if (*command) menu->active = 0;
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
    if (key == KEY_F10) { menu->active = !menu->active; menu->item = 0; return 1; }
    if (!menu->active) return 0;
    if (key == 27) menu->active = 0;
    if (key == KEY_LEFT || key == KEY_RIGHT)
        open_menu(menu, (menu->heading + (key == KEY_LEFT ? 5 : 1)) % MENU_COUNT);
    if (key == KEY_UP || key == KEY_DOWN)
        menu->item = (menu->item + counts[menu->heading] + (key == KEY_UP ? -1 : 1)) % counts[menu->heading];
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
            if (x >= starts[i] && x < starts[i] + menu_width(i)) {
                open_menu(menu, i); return 1;
            }
    }
    if (!menu->active) return 0;
    row = (y - MENU_ROW_TOP) / MENU_ROW_HEIGHT;
    if (x >= menu_left(menu->heading) && x < menu_left(menu->heading) + MENU_DROP_WIDTH &&
        y >= MENU_ROW_TOP && row < counts[menu->heading]) {
        menu->item = row; activate(menu, command);
    } else menu->active = 0;
    return 1;
}
