#include <string.h>
#include <dos.h>
#include "dialog.h"
#include "video.h"
#include "keyboard.h"
#include "mouse.h"
#include "cursor.h"

static int has_mouse;
static unsigned previous_buttons;
void dialog_mouse(int available) { has_mouse = available; }
static void begin_dialog(void)
{
    MouseState mouse;
    cursor_hide();
    mouse_poll(&mouse);
    previous_buttons = mouse.buttons;
}
/* Each dialog consumes fresh presses only; the opening click is swallowed. */
static int event(MouseState *mouse)
{
    int key;
    unsigned pressed;
    if (has_mouse) cursor_show(mouse->x, mouse->y);
    delay(10);
    key = keyboard_poll();
    mouse_poll(mouse);
    pressed = mouse->buttons & ~previous_buttons;
    previous_buttons = mouse->buttons;
    cursor_hide();
    if (key >= 0) return key;
    return (pressed & MOUSE_LEFT) ? -2 : -1;
}
static void box(const char *title)
{
    video_rect(6, 42, 308, 116, 15);
    video_rect(7, 43, 306, 114, 3);
    video_rect(7, 43, 306, 14, 5);
    video_text(12, 46, title, 15);
}
static void line(int y, const char *text)
{
    char part[37];
    unsigned offset, length;
    length = (unsigned)strlen(text);
    for (offset = 0; offset < length && y < 146; offset += 36) {
        strncpy(part, text + offset, 36);
        part[36] = '\0';
        video_text(12, y, part, 15);
        y += 8;
    }
}
static void button(int x, const char *label, int selected)
{
    video_rect(x, 136, 88, 16, selected ? 15 : 1);
    video_rect(x + 1, 137, 86, 14, selected ? 8 : 4);
    video_text(x + 5, 140, label, 15);
}
static int hit_button(const MouseState *mouse)
{
    if (mouse->y < 136 || mouse->y >= 152) return -1;
    if (mouse->x >= 44 && mouse->x < 132) return 0;
    if (mouse->x >= 188 && mouse->x < 276) return 1;
    return -1;
}
static int focus_key(int key)
{
    return key == 9 || key == KEY_SHIFT_TAB || key == KEY_LEFT || key == KEY_RIGHT;
}
int dialog_filename(const char *title, const char *initial, char *output)
{
    DialogInput input;
    MouseState mouse;
    unsigned offset, position;
    char visible[35];
    int result, key, focus, hit;
    dialog_input(&input, initial);
    focus = 0; offset = 0;
    begin_dialog(); mouse_poll(&mouse);
    for (;;) {
        if (input.cursor < offset) offset = input.cursor;
        if (input.cursor > offset + 33) offset = input.cursor - 33;
        box(title);
        video_text(12, 66, "DOS 8.3 path; default .P86", 15);
        video_rect(11, 82, 294, 14, input.replace && focus == 0 ? 8 : 1);
        strncpy(visible, input.text + offset, 34); visible[34] = '\0';
        video_text(12, 85, visible, 15);
        if (focus == 0) video_rect(12 + (int)(input.cursor-offset)*8, 94, 8, 1, 15);
        video_text(12, 110, "Tab: focus  Arrows: edit", 15);
        button(44, "OK", focus == 1); button(188, "Cancel", focus == 2);
        key = event(&mouse);
        if (key == -2) {
            hit = hit_button(&mouse);
            if (hit == 1) return 0;
            if (hit == 0) { key = 13; focus = 1; }
            else if (mouse.x >= 12 && mouse.x < 292 && mouse.y >= 82 && mouse.y < 97) {
                position = offset + (unsigned)(mouse.x-12)/8;
                input.cursor = position < input.length ? position : input.length;
                input.replace = 0; focus = 0;
            }
        }
        if (key == 27 || (key == 13 && focus == 2)) return 0;
        if (key == 9) { focus = (focus + 1) % 3; continue; }
        if (key == KEY_SHIFT_TAB) { focus = (focus + 2) % 3; continue; }
        result = (focus == 0 || key == 13) ? dialog_edit(&input, key) : 0;
        if (result > 0) { strcpy(output, input.text); return 1; }
    }
}
int dialog_confirm(const char *title, const char *question)
{
    int key, focus, hit;
    MouseState mouse;
    focus = 1; /* Destructive prompts default to Cancel. */
    begin_dialog(); mouse_poll(&mouse);
    for (;;) {
        box(title); line(66, question);
        video_text(12, 120, "Y: yes  N/Esc: cancel", 15);
        button(44, "Yes", focus == 0); button(188, "Cancel", focus == 1);
        key = event(&mouse);
        if (key == -2) { hit = hit_button(&mouse); if (hit >= 0) return hit == 0; }
        if (focus_key(key)) focus = !focus;
        if (key == 13) return focus == 0;
        if (key == 'y' || key == 'Y') return 1;
        if (key == 'n' || key == 'N' || key == 27) return 0;
    }
}
int dialog_new_size(unsigned current)
{
    int key, choice, focus, i, hit;
    MouseState mouse;
    choice = current == 16 ? 0 : current == 32 ? 1 : 2;
    focus = 0;
    begin_dialog(); mouse_poll(&mouse);
    for (;;) {
        box("New canvas");
        video_text(12, 66, "1:16x16   2:32x32   3:64x64", 15);
        for (i = 0; i < 3; ++i) video_rect(12 + i*80, 78, 64, 2, i == choice ? 15 : 1);
        video_text(12, 100, "Arrows: size  Tab: focus", 15);
        button(44, "Create", focus == 0); button(188, "Cancel", focus == 1);
        key = event(&mouse);
        if (key == -2) {
            hit = hit_button(&mouse);
            if (hit == 1) return 0;
            if (hit == 0) return 16 << choice;
            if (mouse.y >= 62 && mouse.y < 82 && mouse.x >= 12 && mouse.x < 252) {
                choice = (mouse.x - 12)/80; focus = 0;
            }
        }
        if (key == 27) return 0;
        if (key == 13) return focus ? 0 : 16 << choice;
        if (key == 9 || key == KEY_SHIFT_TAB) focus = !focus;
        if (key == KEY_LEFT || key == KEY_UP) choice = (choice+2)%3;
        if (key == KEY_RIGHT || key == KEY_DOWN) choice = (choice+1)%3;
        if (key >= '1' && key <= '3') return 16 << (key-'1');
    }
}
void dialog_message(const char *title, const char *message, const FileReport *report)
{
    MouseState mouse;
    int key;
    begin_dialog(); mouse_poll(&mouse);
    for (;;) {
        box(title); line(62, message);
        if (report != NULL && report->backup[0]) line(86, report->backup);
        if (report != NULL && report->temporary[0]) line(110, report->temporary);
        button(44, "OK", 1);
        key = event(&mouse);
        if (key >= 0 || (key == -2 && hit_button(&mouse) == 0)) return;
    }
}
