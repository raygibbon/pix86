#include "menu.h"
#include "video.h"
void menu_draw(const Menu *menu)
{
    int i, x, y;
    const MenuItem *item;
    static const int mnemonic[MENU_COUNT] = {0, 0, 0, 1, 0, 0};
    video_rect(0, 0, 320, 16, 5);
    for (i = 0; i < MENU_COUNT; ++i) {
        x = 0;
        if (i > 0) { int j; for (j = 0; j < i; ++j) x += menu_width(j); }
        if (menu->active && menu->heading == i)
            video_rect(x, 0, menu_width(i), 16, 8);
        video_text(x + 2, 4, menu_title(i), 15);
        video_rect(x + 2 + mnemonic[i]*8, 13, 8, 1, 15);
    }
    if (!menu->active) return;
    x = menu_left(menu->heading);
    video_rect(x, 16, 202, menu_items(menu->heading) * 14 + 4, 15);
    video_rect(x + 1, 17, 200, menu_items(menu->heading) * 14 + 2, 3);
    for (i = 0; i < menu_items(menu->heading); ++i) {
        item = menu_item(menu->heading, i);
        y = 18 + i * 14;
        if (i == menu->item) video_rect(x + 2, y, 198, 14, 8);
        video_text(x + 5, y + 3, item->label, item->command ? 15 : 7);
        video_text(x + 131, y + 3, item->shortcut, 15);
    }
    video_rect(0, 192, 320, 8, 5);
    video_text(0, 192, "Arrows: move  Enter: select  Esc: close", 15);
}
