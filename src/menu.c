#include <ctype.h>
#include <string.h>
#include "menu.h"
#include "keyboard.h"

static const char *titles[MENU_COUNT] = {
    "File", "Edit", "View", "Frame", "Options", "Help"
};
static const int counts[MENU_COUNT] = {7, 4, 4, 3, 5, 2};
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
     {"Grid", "G", 'G', CMD_GRID}, {"", "", 0, 0},
     {"Display Mode", "", 'D', CMD_DISPLAY_MODE}},
    {{"Controls", "", 'C', CMD_HELP}, {"About", "", 'A', CMD_ABOUT}}
};

int menu_separator(int heading, int item)
{
    return items[heading][item].label[0] == '\0';
}

int menu_row_height(int heading, int item)
{
    return menu_separator(heading, item) ? MENU_SEPARATOR_HEIGHT : MENU_ROW_HEIGHT;
}

int menu_row_y(int heading, int item)
{
    int y, i;
    y = MENU_ROW_TOP;
    for (i = 0; i < item; ++i)
        y += menu_row_height(heading, i);
    return y;
}

int menu_letter(int heading, int item)
{
    int letter;
    if (menu_separator(heading, item))
        return 0;
    letter = items[heading][item].accelerator;
    return tolower((unsigned char)letter);
}

static int first_item(int heading)
{
    int i;
    for (i = 0; i < counts[heading]; ++i)
        if (!menu_separator(heading, i))
            return i;
    return 0;
}

static void remember(Menu *menu)
{
    if (!menu_separator(menu->heading, menu->item))
        menu->item_memory[menu->heading] = menu->item;
}

static int next_item(int heading, int item, int direction)
{
    int i;
    for (i = 0; i < counts[heading]; ++i) {
        item = (item + counts[heading] + direction) % counts[heading];
        if (!menu_separator(heading, item))
            return item;
    }
    return first_item(heading);
}

int menu_width(int heading)
{
    return font_ui_width(titles[heading]) + 8;
}

int menu_heading_left(int heading)
{
    int x, i;
    x = 0;
    for (i = 0; i < heading; ++i)
        x += menu_width(i);
    return x;
}

int menu_drop_width(int heading)
{
    int i, names, shortcuts, width;
    names = shortcuts = 0;
    for (i = 0; i < counts[heading]; ++i) {
        if (menu_separator(heading, i))
            continue;
        width = font_ui_width(items[heading][i].label);
        if (width > names)
            names = width;
        width = font_ui_width(items[heading][i].shortcut);
        if (width > shortcuts)
            shortcuts = width;
    }
    width = 3 * UI_FONT_WIDTH + names + shortcuts + 8 +
            (shortcuts ? UI_FONT_WIDTH : 0);
    return width < menu_width(heading) ? menu_width(heading) : width;
}

int menu_left(int heading)
{
    int x, width;
    x = menu_heading_left(heading);
    width = menu_drop_width(heading);
    if (x + width > UI_WIDTH)
        return width < UI_WIDTH ? UI_WIDTH - width : 0;
    return x;
}

void menu_bounds(const Menu *menu, Rect *bounds)
{
    bounds->x = menu_left(menu->heading);
    bounds->y = MENU_BAR_HEIGHT;
    bounds->w = menu->active ? menu_drop_width(menu->heading) : 0;
    bounds->h = menu->active ?
        menu_row_y(menu->heading, counts[menu->heading]) - MENU_BAR_HEIGHT + 1 : 0;
}

int menu_items(int heading) { return counts[heading]; }
const char *menu_title(int heading) { return titles[heading]; }
const MenuItem *menu_item(int heading, int item) { return &items[heading][item]; }

void menu_init(Menu *menu)
{
    int i;
    menu->active = menu->heading = 0;
    for (i = 0; i < MENU_COUNT; ++i)
        menu->item_memory[i] = first_item(i);
    menu->item = menu->item_memory[0];
    menu->buttons = 0;
}

static void choose(Menu *menu, int item)
{
    menu->item = item;
    menu->item_memory[menu->heading] = item;
}

static void open_menu(Menu *menu, int heading)
{
    if (menu->active)
        remember(menu);
    menu->heading = heading;
    menu->item = menu->item_memory[heading];
    menu->active = 1;
}

static void activate(Menu *menu, int *command)
{
    if (menu_separator(menu->heading, menu->item))
        return;
    *command = items[menu->heading][menu->item].command;
    choose(menu, menu->item);
    if (*command)
        menu->active = 0;
}

int menu_key(Menu *menu, int key, int *command)
{
    int heading, item, letter;
    *command = 0;
    if (key == KEY_MENU) {
        if (menu->active) {
            remember(menu);
            menu->active = 0;
        } else {
            open_menu(menu, 0);
        }
        return 1;
    }
    heading = keyboard_menu(key);
    if (heading >= 0) {
        open_menu(menu, heading);
        return 1;
    }
    if (key == KEY_F10) {
        if (menu->active) {
            remember(menu);
            menu->active = 0;
        } else {
            open_menu(menu, menu->heading);
        }
        return 1;
    }
    if (!menu->active)
        return 0;
    if (key == 27) {
        remember(menu);
        menu->active = 0;
        return 1;
    }
    if (key == KEY_LEFT || key == KEY_RIGHT) {
        open_menu(menu, (menu->heading +
                   (key == KEY_LEFT ? MENU_COUNT - 1 : 1)) % MENU_COUNT);
        return 1;
    }
    if (key == KEY_UP || key == KEY_DOWN) {
        choose(menu, next_item(menu->heading, menu->item,
                              key == KEY_UP ? -1 : 1));
        return 1;
    }
    if (key == 13) {
        activate(menu, command);
        return 1;
    }
    if (key > 0 && key < 256) {
        letter = tolower((unsigned char)key);
        for (item = 0; item < counts[menu->heading]; ++item) {
            if (menu_letter(menu->heading, item) != letter)
                continue;
            if (!menu_separator(menu->heading, item) &&
                items[menu->heading][item].command) {
                choose(menu, item);
                activate(menu, command);
            }
            return 1;
        }
    }
    remember(menu);
    return 1;
}

int menu_mouse(Menu *menu, int x, int y, unsigned buttons, int *command)
{
    int i, top;
    unsigned pressed;
    *command = 0;
    pressed = buttons & ~menu->buttons;
    menu->buttons = buttons;
    if (!(pressed & 1))
        return menu->active;
    if (y >= 0 && y < MENU_BAR_HEIGHT) {
        for (i = 0; i < MENU_COUNT; ++i) {
            if (x >= menu_heading_left(i) &&
                x < menu_heading_left(i) + menu_width(i)) {
                open_menu(menu, i);
                return 1;
            }
        }
    }
    if (!menu->active)
        return 0;
    if (x >= menu_left(menu->heading) + 1 &&
        x < menu_left(menu->heading) + menu_drop_width(menu->heading) - 1) {
        for (i = 0; i < counts[menu->heading]; ++i) {
            top = menu_row_y(menu->heading, i);
            if (y >= top && y < top + menu_row_height(menu->heading, i)) {
                if (!menu_separator(menu->heading, i)) {
                    choose(menu, i);
                    activate(menu, command);
                }
                return 1;
            }
        }
    }
    remember(menu);
    menu->active = 0;
    return 1;
}
