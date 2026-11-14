#ifndef PIX86_MENU_H
#define PIX86_MENU_H
#include "layout.h"
#include "command.h"
#include "region.h"
#define MENU_COUNT 6
#define MENU_BAR_HEIGHT UI_MENU_HEIGHT
#define MENU_ROW_HEIGHT (UI_FONT_HEIGHT + 1)
#define MENU_SEPARATOR_HEIGHT 3
#define MENU_ROW_TOP (MENU_BAR_HEIGHT + 1)

typedef struct Menu {
    int active, heading, item;
    unsigned char item_memory[MENU_COUNT];
    unsigned buttons;
} Menu;
typedef struct MenuItem {
    const char *label, *shortcut;
    int accelerator, command;
} MenuItem;
void menu_init(Menu *menu);
/* Nonzero consumes the event. command is zero unless an item activates. */
int menu_key(Menu *menu, int key, int *command);
int menu_mouse(Menu *menu, int x, int y, unsigned buttons, int *command);
int menu_heading_left(int heading);
int menu_left(int heading);
int menu_width(int heading);
int menu_items(int heading);
int menu_separator(int heading,int item);
int menu_letter(int heading,int item);
int menu_row_y(int heading,int item);
int menu_row_height(int heading,int item);
const char *menu_title(int heading);
const MenuItem *menu_item(int heading, int item);
int menu_drop_width(int heading);
void menu_bounds(const Menu *menu, Rect *bounds);
void menu_draw_bar(const Menu *menu);
void menu_draw_change(const Menu *menu, const Menu *previous);
void menu_draw(const Menu *menu);
#endif
