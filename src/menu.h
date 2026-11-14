#ifndef PIX86_MENU_H
#define PIX86_MENU_H
#define MENU_COUNT 6
#define CMD_HELP 600
#define CMD_ABOUT 601
typedef struct Menu { int active, heading, item; unsigned buttons; } Menu;
typedef struct MenuItem { const char *label, *shortcut; int command; } MenuItem;
void menu_init(Menu *menu);
/* Nonzero consumes the event. command is zero unless an item activates. */
int menu_key(Menu *menu, int key, int *command);
int menu_mouse(Menu *menu, int x, int y, unsigned buttons, int *command);
int menu_left(int heading);
int menu_width(int heading);
int menu_items(int heading);
const char *menu_title(int heading);
const MenuItem *menu_item(int heading, int item);
void menu_draw(const Menu *menu);
#endif
