#include <string.h>
#include "menu.h"
#include "video.h"
#include "cursor.h"
#include "theme.h"
#include "uipaint.h"

static void heading(const Menu *menu, int index)
{
    static const int mnemonic[MENU_COUNT] = {0, 0, 0, 1, 0, 0};
    int x, width, selected;
    unsigned char ink;
    x = menu_heading_left(index);
    width = menu_width(index);
    selected = menu->active && menu->heading == index;
    ink = selected ? UI_MENU_ACTIVE : UI_INK;
    cursor_protect(x, 0, width, MENU_BAR_HEIGHT);
    video_rect(x, 0, width, UI_FONT_HEIGHT, UI_MENU_BAR);
    video_ui_text_bg(x + 3, 0, menu_title(index), ink, UI_MENU_BAR);
    video_rect(x, UI_FONT_HEIGHT, width, 1, UI_MENU_BAR);
    video_rect(x + 3 + mnemonic[index] * UI_FONT_WIDTH, UI_FONT_HEIGHT,
               UI_FONT_WIDTH - 1, 1, ink);
}

static void row(const Menu *menu, int index)
{
    int x, y, width, right, end;
    unsigned char paper, ink;
    const MenuItem *item;
    char label[32];
    x = menu_left(menu->heading);
    y = menu_row_y(menu->heading, index);
    width = menu_drop_width(menu->heading);
    item = menu_item(menu->heading, index);
    if (menu_separator(menu->heading, index)) {
        cursor_protect(x + 1, y, width - 2, MENU_SEPARATOR_HEIGHT);
        video_rect(x + 1, y, width - 2, MENU_SEPARATOR_HEIGHT, UI_MENU_PAPER);
        video_rect(x + 3, y + 1, width - 6, 1, UI_DIM);
        return;
    }
    label[0] = (char)menu_letter(menu->heading, index);
    label[1] = ')';
    label[2] = ' ';
    strcpy(label + 3, item->label);
    paper = index == menu->item ? UI_MENU_SELECT : UI_MENU_PAPER;
    ink = !item->command ? UI_DIM : index == menu->item ? UI_WHITE : UI_INK;
    cursor_protect(x + 1, y, width - 2, MENU_ROW_HEIGHT);
    end = x + 4 + font_ui_width(label);
    right = x + width - 4 - font_ui_width(item->shortcut);
    video_rect(x + 1, y, 3, UI_FONT_HEIGHT, paper);
    video_ui_text_bg(x + 4, y, label, ink, paper);
    video_rect(end, y, right - end, UI_FONT_HEIGHT, paper);
    video_ui_text_bg(right, y, item->shortcut, ink, paper);
    video_rect(x + width - 4, y, 3, UI_FONT_HEIGHT, paper);
    video_rect(x + 1, y + UI_FONT_HEIGHT, width - 2, 1, paper);
}

void menu_draw_bar(const Menu *menu)
{
    int i, x;
    cursor_protect(0, 0, UI_WIDTH, MENU_BAR_HEIGHT);
    video_rect(0, 0, UI_WIDTH, MENU_BAR_HEIGHT, UI_MENU_BAR);
    for (i = 0; i < MENU_COUNT; ++i)
        heading(menu, i);
    x = UI_WIDTH - font_ui_width("Ctrl+\\") - 4;
    video_ui_text_bg(x, 0, "Ctrl+\\", UI_INK, UI_MENU_BAR);
}

void menu_draw(const Menu *menu)
{
    Rect bounds;
    int i;
    menu_draw_bar(menu);
    if (!menu->active)
        return;
    menu_bounds(menu, &bounds);
    cursor_protect(bounds.x, bounds.y, bounds.w, bounds.h);
    ui_border(&bounds, UI_INK);
    for (i = 0; i < menu_items(menu->heading); ++i)
        row(menu, i);
}

void menu_draw_change(const Menu *menu, const Menu *previous)
{
    Rect bounds;
    int i;
    if (menu->active != previous->active || menu->heading != previous->heading) {
        heading(menu, previous->heading);
        if (menu->heading != previous->heading)
            heading(menu, menu->heading);
        if (menu->active) {
            menu_bounds(menu, &bounds);
            cursor_protect(bounds.x, bounds.y, bounds.w, bounds.h);
            ui_border(&bounds, UI_INK);
            for (i = 0; i < menu_items(menu->heading); ++i)
                row(menu, i);
        }
    } else if (menu->active && menu->item != previous->item) {
        row(menu, previous->item);
        row(menu, menu->item);
    }
}
