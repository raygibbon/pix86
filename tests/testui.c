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

typedef struct Event { int key, x, y; unsigned buttons; } Event;
static Event events[128];
static int count, current, visible;
static unsigned draws;
static Editor editor;
static unsigned short queue[4096];
static void start(void) { count = 0; current = -1; visible = 0; }
static void add(int key, int x, int y, unsigned buttons)
{
    assert(count < 128);
    events[count].key = key; events[count].x = x; events[count].y = y;
    events[count++].buttons = buttons;
}
int keyboard_poll(void) { assert(++current < count); return events[current].key; }
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
    assert(!visible && x >= 0 && y >= 0 && x+(int)strlen(text)*8 <= 320 && y+8 <= 200);
    (void)colour; ++draws;
}
static void menus(void)
{
    Menu menu;
    int command, h, row;
    menu_init(&menu);
    assert(!menu_key(&menu, 'p', &command));
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
    menu_key(&menu, KEY_F10, &command);
    menu_key(&menu, 'c', &command); assert(!command && menu.active);
    menu_key(&menu, KEY_F10, &command); assert(!menu.active);
    assert(menu_mouse(&menu, 5, 4, 1, &command) && menu.active);
    menu_mouse(&menu, 5, 4, 0, &command);
    assert(menu_mouse(&menu, 8, 18+2*14, 1, &command));
    assert(command == KEY_F2 && !menu.active);
    menu_mouse(&menu, 8, 18+2*14, 0, &command);
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
    start(); add(27,0,0,0); strcpy(path,"UNCHANGED");
    assert(!dialog_filename("Open", "OLD.P86", path) && !strcmp(path,"UNCHANGED"));
    start(); for (i=0;i<79;++i) add('X',0,0,0); add(KEY_HOME,0,0,0); add(KEY_END,0,0,0); add(13,0,0,0);
    assert(dialog_filename("Save", "", path) && strlen(path)==79);
    start(); add(9,0,0,0); add(9,0,0,0); add(-1,50,140,1);
    assert(dialog_filename("Save", "FILE.P86", path)); /* OK from Cancel focus */
    start(); add(-1,200,140,1); assert(!dialog_filename("Save", "FILE", path));
    start(); add(13,0,0,0); assert(!dialog_confirm("Unsaved", "Discard?"));
    start(); add(9,0,0,0); add(13,0,0,0); assert(dialog_confirm("Overwrite", "FILE.P86"));
    start(); add('y',0,0,0); assert(dialog_confirm("Unsaved", "Discard?"));
    start(); add(27,0,0,0); assert(!dialog_confirm("Unsaved", "Discard?"));
    start(); add(-1,50,140,1); assert(dialog_confirm("Overwrite", "FILE.P86"));
    start(); add(-1,200,140,1); assert(!dialog_confirm("Unsaved", "Discard?"));
    start(); add(KEY_RIGHT,0,0,0); add(13,0,0,0); assert(dialog_new_size(16)==32);
    start(); add(-1,180,70,1); add(-1,180,70,0); add(-1,50,140,1); assert(dialog_new_size(16)==64);
    start(); add(27,0,0,0); assert(!dialog_new_size(32));
    memset(&report,0,sizeof(report));
    memset(report.backup,'B',FILE_PATH_MAX-1); memset(report.temporary,'T',FILE_PATH_MAX-1);
    start(); add(-1,50,140,1); dialog_message("Recovery", "Retain backup and temporary", &report);
    assert(!visible && draws);
}
int main(void)
{
    menus(); inputs(); dialogs();
    puts("PASS: menus, keyboard/mouse dialogs, cursor editing and drawing handoff.");
    return 0;
}
