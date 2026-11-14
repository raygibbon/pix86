#include <string.h>
#include "menu.h"
#include "video.h"
#include "font.h"
#include "theme.h"
void menu_draw(const Menu *menu)
{
    int i, x, y, selected;
    const MenuItem *item;
    static const int mnemonic[MENU_COUNT] = {0, 0, 0, 1, 0, 0};
    video_rect(0, 0, 320, MENU_BAR_HEIGHT, UI_BAR);
    for (i = 0; i < MENU_COUNT; ++i) {
        x = menu_heading_left(i);
        selected = menu->active && menu->heading == i;
        if (selected) video_rect(x, 0, menu_width(i), MENU_BAR_HEIGHT, UI_FOCUS);
        video_text(x + 4, 1, menu_title(i), selected ? UI_WHITE : UI_INK);
        video_rect(x + 4 + mnemonic[i]*FONT_WIDTH, 8, 5, 1,
                   selected ? UI_WHITE : UI_INK);
    }
    video_text(272, 1, "Ctrl+\\", UI_INK);
    if (!menu->active) return;
    x = menu_left(menu->heading);
    video_rect(x, MENU_BAR_HEIGHT, MENU_DROP_WIDTH,
               menu_items(menu->heading) * MENU_ROW_HEIGHT + 4, UI_INK);
    video_rect(x + 1, MENU_BAR_HEIGHT + 1, MENU_DROP_WIDTH - 2,
               menu_items(menu->heading) * MENU_ROW_HEIGHT + 2, UI_PAPER);
    for (i = 0; i < menu_items(menu->heading); ++i) {
        item = menu_item(menu->heading, i);
        y = MENU_ROW_TOP + i * MENU_ROW_HEIGHT;
        selected = i == menu->item;
        if (selected) video_rect(x + 2, y, MENU_DROP_WIDTH - 4, MENU_ROW_HEIGHT, UI_SELECT);
        video_text(x + 5, y + 1, item->label,
                   !item->command ? UI_DIM : selected ? UI_WHITE : UI_INK);
        video_text(x + MENU_DROP_WIDTH - 5 - (int)strlen(item->shortcut)*FONT_WIDTH,
                   y + 1, item->shortcut, selected ? UI_WHITE : UI_INK);
    }
    video_rect(0, 192, 320, 8, UI_BAR);
    video_text(2, 192, "Arrows: move  Enter: choose  Esc/Ctrl+\\: close", UI_INK);
}
