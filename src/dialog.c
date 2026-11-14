#include <string.h>
#include <dos.h>
#include "dialog.h"
#include "video.h"
#include "keyboard.h"
#include "mouse.h"
#include "cursor.h"
#include "font.h"
#include "theme.h"

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
    for (;;) {
        if (has_mouse) cursor_show(mouse->x, mouse->y);
        delay(10);
        key = keyboard_poll();
        mouse_poll(mouse);
        pressed = mouse->buttons & ~previous_buttons;
        previous_buttons = mouse->buttons;
        cursor_hide();
        if (key >= 0) return key;
        if (pressed & MOUSE_LEFT) return -2;
        /* Pointer motion needs cursor restoration, not a dialog repaint. */
    }
}
static void box(const char *title)
{
    video_rect(DIALOG_X, DIALOG_Y, DIALOG_WIDTH, DIALOG_HEIGHT, UI_INK);
    video_rect(DIALOG_X + 1, DIALOG_Y + 1, DIALOG_WIDTH - 2, DIALOG_HEIGHT - 2, UI_PAPER);
    video_text(DIALOG_TEXT_X, DIALOG_Y + 3, title, UI_INK);
    video_rect(DIALOG_X + 1, 61, DIALOG_WIDTH - 2, 1, UI_DIM);
}
static void line(int y, const char *text)
{
    char part[41];
    unsigned offset, length;
    length = (unsigned)strlen(text);
    for (offset = 0; offset < length && y < 134; offset += 40) {
        strncpy(part, text + offset, 40);
        part[40] = '\0';
        video_text(DIALOG_TEXT_X, y, part, UI_INK);
        y += FONT_HEIGHT;
    }
}
static void button(int x, const char *label, int selected)
{
    video_rect(x, DIALOG_BUTTON_Y, DIALOG_BUTTON_WIDTH, DIALOG_BUTTON_HEIGHT,
               selected ? UI_SELECT : UI_PAPER);
    video_text(x + 4, DIALOG_BUTTON_Y + 1, label, selected ? UI_WHITE : UI_INK);
}
static int hit_button(const MouseState *mouse)
{
    if (mouse->y < DIALOG_BUTTON_Y || mouse->y >= DIALOG_BUTTON_Y + DIALOG_BUTTON_HEIGHT) return -1;
    if (mouse->x >= DIALOG_ACCEPT_X && mouse->x < DIALOG_ACCEPT_X + DIALOG_BUTTON_WIDTH) return 0;
    if (mouse->x >= DIALOG_CANCEL_X && mouse->x < DIALOG_CANCEL_X + DIALOG_BUTTON_WIDTH) return 1;
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
    char visible[40];
    int result, key, focus, hit;
    dialog_input(&input, initial);
    focus = 0; offset = 0;
    begin_dialog(); mouse_poll(&mouse);
    for (;;) {
        if (input.cursor < offset) offset = input.cursor;
        if (input.cursor > offset + 38) offset = input.cursor - 38;
        box(title);
        video_text(DIALOG_TEXT_X, 66, "DOS 8.3 path; default .P86", UI_INK);
        video_rect(DIALOG_TEXT_X - 1, 82, 244, 12, focus == 0 ? UI_FIELD : UI_PAPER);
        if (input.replace && focus == 0) video_rect(DIALOG_TEXT_X, 83, 240, 10, UI_FOCUS);
        strncpy(visible, input.text + offset, 39); visible[39] = '\0';
        video_text(DIALOG_TEXT_X, 84, visible, input.replace && focus == 0 ? UI_WHITE : UI_INK);
        if (focus == 0) video_rect(DIALOG_TEXT_X + (int)(input.cursor-offset)*FONT_WIDTH, 92, 5, 1, UI_INK);
        video_text(DIALOG_TEXT_X, 106, "Tab: focus  Arrows: edit", UI_INK);
        video_text(DIALOG_TEXT_X, 118, "Enter: accept  Esc: cancel", UI_INK);
        button(DIALOG_ACCEPT_X, "OK", focus == 1); button(DIALOG_CANCEL_X, "Cancel", focus == 2);
        key = event(&mouse);
        if (key == -2) {
            hit = hit_button(&mouse);
            if (hit == 1) return 0;
            if (hit == 0) { key = 13; focus = 1; }
            else if (mouse.x >= DIALOG_TEXT_X && mouse.x < DIALOG_TEXT_X + 240 && mouse.y >= 82 && mouse.y < 94) {
                position = offset + (unsigned)(mouse.x-DIALOG_TEXT_X)/FONT_WIDTH;
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
        video_text(DIALOG_TEXT_X, 120, "Y: yes  N/Esc: cancel", UI_INK);
        button(DIALOG_ACCEPT_X, "Yes", focus == 0); button(DIALOG_CANCEL_X, "Cancel", focus == 1);
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
        video_text(DIALOG_TEXT_X, 66, "1:16x16     2:32x32     3:64x64", UI_INK);
        for (i = 0; i < 3; ++i) video_rect(DIALOG_TEXT_X + i*72, 78, 60, 1, i == choice ? UI_INK : UI_PAPER);
        video_text(DIALOG_TEXT_X, 100, "Arrows: size  Tab: focus", UI_INK);
        button(DIALOG_ACCEPT_X, "Create", focus == 0); button(DIALOG_CANCEL_X, "Cancel", focus == 1);
        key = event(&mouse);
        if (key == -2) {
            hit = hit_button(&mouse);
            if (hit == 1) return 0;
            if (hit == 0) return 16 << choice;
            if (mouse.y >= 62 && mouse.y < 82 && mouse.x >= DIALOG_TEXT_X && mouse.x < DIALOG_TEXT_X + 216) {
                choice = (mouse.x - DIALOG_TEXT_X)/72; focus = 0;
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
        button(DIALOG_ACCEPT_X, "OK", 1);
        key = event(&mouse);
        if (key >= 0 || (key == -2 && hit_button(&mouse) == 0)) return;
    }
}
