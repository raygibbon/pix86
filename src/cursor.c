#include "video.h"
#include "cursor.h"

static unsigned char saved[64];
static int cursor_x, cursor_y, visible;

void cursor_hide(void)
{
    int x, y;
    if (!visible)
        return;
    for (y = 0; y < 8; ++y)
        for (x = 0; x < 8; ++x)
            video_pixel(cursor_x + x, cursor_y + y, saved[y * 8 + x]);
    visible = 0;
}

void cursor_show(int x, int y)
{
    int xx, yy;
    cursor_hide();
    cursor_x = x;
    cursor_y = y;
    for (yy = 0; yy < 8; ++yy)
        for (xx = 0; xx < 8; ++xx)
            saved[yy * 8 + xx] = video_read(x + xx, y + yy);
    /* Original small corner pointer, with a dark border. */
    for (yy = 0; yy < 7; ++yy)
        for (xx = 0; xx <= yy / 2 + 1; ++xx)
            video_pixel(x + xx, y + yy,
                        (xx == 0 || xx == yy / 2 + 1 || yy == 6) ? 0 : 15);
    visible = 1;
}
