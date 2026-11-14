#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "keyboard.h"
#include "menu.h"
#include "dialog.h"
#include "mouse.h"
#include "video.h"
#include "cursor.h"
#include "editor.h"
#include "font.h"

typedef struct Event { int key, x, y; unsigned buttons; } Event;
static Event events[128];
static int count, current, visible;
static unsigned draws, poll_draws;
static Editor editor;
static unsigned short queue[4096];
static void start(void) { count = 0; current = -1; visible = 0; }
static void add(int key, int x, int y, unsigned buttons)
{
    assert(count < 128);
    events[count].key = key; events[count].x = x; events[count].y = y;
    events[count++].buttons = buttons;
}
int keyboard_poll(void)
{
    if (current >= 0 && events[current].key < 0 && !events[current].buttons)
        assert(draws == poll_draws); /* idle/release must not repaint a dialog */
    poll_draws = draws;
    assert(++current < count);
    return events[current].key;
}
void mouse_poll(MouseState *mouse)
{
    mouse->x = current < 0 ? 0 : events[current].x;
    mouse->y = current < 0 ? 0 : events[current].y;
    mouse->buttons = current < 0 ? 0 : events[current].buttons;
}
void delay(unsigned ms) { (void)ms; }
void cursor_hide(void) { visible = 0; }
void cursor_show(int x, int y) { (void)x; (void)y; visible = 1; }
void video_rect(int x, int y, int w, int h, unsigned char colour)
{
    assert(!visible && x >= 0 && y >= 0 && x+w <= 320 && y+h <= 200);
    (void)colour; ++draws;
}
void video_text(int x, int y, const char *text, unsigned char colour)
{
    assert(!visible && x >= 0 && y >= 0 && x+(int)strlen(text)*FONT_WIDTH <= 320 && y+8 <= 200);
    (void)colour; ++draws;
}
static void menus(void)
{
    Menu menu;
    int command, h, row;
    menu_init(&menu);
    assert(!menu_key(&menu, 'p', &command));
    assert(menu_key(&menu, KEY_MENU, &command) && menu.active && !menu.heading);
    menu_key(&menu, KEY_RIGHT, &command);
    assert(menu_key(&menu, KEY_MENU, &command) && !menu.active);
    menu.heading = 4;
    menu_key(&menu, KEY_MENU, &command); assert(menu.active && !menu.heading);
    menu_key(&menu, KEY_MENU, &command); assert(!menu.active);
    assert(menu_key(&menu, KEY_F10, &command) && menu.active);
    assert(menu_key(&menu, KEY_LEFT, &command) && menu.heading == 5);
    menu_key(&menu, KEY_RIGHT, &command); assert(menu.heading == 0);
    menu_key(&menu, KEY_UP, &command); assert(menu.item == 4);
    menu_key(&menu, KEY_DOWN, &command); assert(menu.item == 0);
    menu_key(&menu, KEY_DOWN, &command);
    menu_key(&menu, 13, &command); assert(command == KEY_F3 && !menu.active);
    for (h = 0; h < MENU_COUNT; ++h) {
        int keys[MENU_COUNT] = {KEY_ALT_F, KEY_ALT_E, KEY_ALT_V, KEY_ALT_R, KEY_ALT_O, KEY_ALT_H};
        assert(keyboard_menu(keys[h]) == h);
        menu_key(&menu, keys[h], &command); assert(menu.active && menu.heading == h);
        for (row = 0; row < menu_items(h); ++row) {
            menu.item = row; menu_draw(&menu);
            menu_key(&menu, 13, &command);
            assert(command == menu_item(h, row)->command);
            assert(menu.active == !command);
            menu_key(&menu, keys[h], &command);
        }
        menu_key(&menu, 27, &command); assert(!menu.active && !command);
    }
    assert(keyboard_menu('f') == -1);
    for (h = 0; h < 4; ++h) {
        assert(toolbar_hit(TOOL_X, TOOL_Y+h*TOOL_STEP) == h);
        assert(toolbar_hit(TOOL_X, TOOL_Y+h*TOOL_STEP+TOOL_HEIGHT) == -1);
    }
    assert(toolbar_hit(TOOL_X-1, TOOL_Y) == -1);
    assert(toolbar_hit(TOOL_X+TOOL_WIDTH, TOOL_Y) == -1);
    menu_key(&menu, KEY_F10, &command);
    menu_key(&menu, 'c', &command); assert(!command && menu.active);
    menu_key(&menu, KEY_F10, &command); assert(!menu.active);
    assert(menu_mouse(&menu, 5, 4, 1, &command) && menu.active);
    menu_mouse(&menu, 5, 4, 0, &command);
    assert(menu_mouse(&menu, 8, MENU_ROW_TOP+2*MENU_ROW_HEIGHT, 1, &command));
    assert(command == KEY_F2 && !menu.active);
    menu_mouse(&menu, 8, MENU_ROW_TOP+2*MENU_ROW_HEIGHT, 0, &command);
    menu_key(&menu, KEY_F10, &command);
    assert(menu_mouse(&menu, 100, 180, 1, &command) && !menu.active && !command);
    /* The dismissal is consumed; holding into the drawing area cannot edit. */
    assert(editor_init(&editor, 16, queue));
    editor.previous_buttons = 1;
    editor_mouse(&editor, editor.view.x, editor.view.y, 1);
    assert(!editor_is_dirty(&editor));
    editor_mouse(&editor, editor.view.x, editor.view.y, 0);
    editor_mouse(&editor, editor.view.x, editor.view.y, 1);
    assert(editor_is_dirty(&editor));
}
static void keys(void)
{
    assert(keyboard_decode(0x2b1c, 0) == KEY_MENU); /* Ctrl released while queued */
    assert(keyboard_decode(0x2b00, 4) == KEY_MENU); /* physical US fallback */
    assert(keyboard_decode(0x2b7c, 4) == KEY_MENU);
    assert(keyboard_decode(0x565c, 4) == KEY_MENU); /* logical ISO slash */
    assert(keyboard_decode(0x565c, 12) == '\\'); /* AltGr types a path */
    assert(keyboard_decode(0x2b5c, 0) == '\\');
    assert(keyboard_decode(0x2b7c, 0) == '|');
    assert(keyboard_decode(0x1c0d, 0) == 13);
    assert(keyboard_decode(0x011b, 0) == 27);
    assert(keyboard_decode(0x4b00, 0) == KEY_LEFT);
    assert(keyboard_decode(0x4be0, 0) == KEY_LEFT);
    assert(keyboard_decode(0x4400, 0) == KEY_F10);
    assert(keyboard_decode(0x2100, 8) == KEY_ALT_F);
    assert(keyboard_decode(0x5500, 1) == KEY_SAVE_AS);
    assert(keyboard_decode(0x2c1a, 4) == 26);
    assert(keyboard_decode(0x1519, 4) == 25);
    assert(keyboard_decode(0x1970, 0) == 'p');
}
static void inputs(void)
{
    DialogInput input;
    unsigned i;
    dialog_input(&input, "ABC.P86");
    dialog_edit(&input, KEY_HOME); dialog_edit(&input, KEY_RIGHT);
    dialog_edit(&input, 'X'); assert(!strcmp(input.text, "AXBC.P86"));
    dialog_edit(&input, KEY_DELETE); assert(!strcmp(input.text, "AXC.P86"));
    dialog_edit(&input, 8); assert(!strcmp(input.text, "AC.P86"));
    dialog_edit(&input, KEY_END); dialog_edit(&input, KEY_RIGHT);
    assert(input.cursor == input.length);
    dialog_edit(&input, KEY_HOME); dialog_edit(&input, 8); dialog_edit(&input, KEY_LEFT);
    assert(input.cursor == 0);
    dialog_input(&input, "");
    for (i = 0; i < 100; ++i) dialog_edit(&input, 'X');
    assert(input.length == FILE_PATH_MAX-1 && input.cursor == input.length);
}
static void dialogs(void)
{
    char path[FILE_PATH_MAX];
    FileReport report;
    unsigned i;
    dialog_mouse(1);
    start(); add('A',0,0,0); add(KEY_LEFT,0,0,0); add('B',0,0,0); add(13,0,0,0);
    assert(dialog_filename("Save", "OLD.P86", path) && !strcmp(path,"BA"));
    start(); add(-1,DIALOG_TEXT_X+FONT_WIDTH,86,1); add('X',0,0,0); add(13,0,0,0);
    assert(dialog_filename("Save", "ABC", path) && !strcmp(path,"AXBC"));
    start(); add(-1,0,0,0); add(-1,0,0,0); add(27,0,0,0);
    assert(!dialog_filename("Open", "", path));
    start(); add(27,0,0,0); strcpy(path,"UNCHANGED");
    assert(!dialog_filename("Open", "OLD.P86", path) && !strcmp(path,"UNCHANGED"));
    start(); for (i=0;i<79;++i) add('X',0,0,0); add(KEY_HOME,0,0,0); add(KEY_END,0,0,0); add(13,0,0,0);
    assert(dialog_filename("Save", "", path) && strlen(path)==79);
    start(); add(9,0,0,0); add(9,0,0,0); add(-1,DIALOG_ACCEPT_X+4,DIALOG_BUTTON_Y+2,1);
    assert(dialog_filename("Save", "FILE.P86", path)); /* OK from Cancel focus */
    start(); add(-1,DIALOG_CANCEL_X+4,DIALOG_BUTTON_Y+2,1); assert(!dialog_filename("Save", "FILE", path));
    start(); add(13,0,0,0); assert(!dialog_confirm("Unsaved", "Discard?"));
    start(); add(9,0,0,0); add(13,0,0,0); assert(dialog_confirm("Overwrite", "FILE.P86"));
    start(); add('y',0,0,0); assert(dialog_confirm("Unsaved", "Discard?"));
    start(); add(27,0,0,0); assert(!dialog_confirm("Unsaved", "Discard?"));
    start(); add(-1,DIALOG_ACCEPT_X+4,DIALOG_BUTTON_Y+2,1); assert(dialog_confirm("Overwrite", "FILE.P86"));
    start(); add(-1,DIALOG_CANCEL_X+4,DIALOG_BUTTON_Y+2,1); assert(!dialog_confirm("Unsaved", "Discard?"));
    start(); add(KEY_RIGHT,0,0,0); add(13,0,0,0); assert(dialog_new_size(16)==32);
    start(); add(-1,DIALOG_TEXT_X+150,70,1); add(-1,DIALOG_TEXT_X+150,70,0); add(-1,DIALOG_ACCEPT_X+4,DIALOG_BUTTON_Y+2,1); assert(dialog_new_size(16)==64);
    start(); add(27,0,0,0); assert(!dialog_new_size(32));
    memset(&report,0,sizeof(report));
    memset(report.backup,'B',FILE_PATH_MAX-1); memset(report.temporary,'T',FILE_PATH_MAX-1);
    start(); add(-1,DIALOG_ACCEPT_X+4,DIALOG_BUTTON_Y+2,1); dialog_message("Recovery", "Retain backup and temporary", &report);
    assert(!visible && draws);
}
int main(void)
{
    keys(); menus(); inputs(); dialogs();
    puts("PASS: menus, keyboard/mouse dialogs, cursor editing and drawing handoff.");
    return 0;
}
