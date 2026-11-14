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
    if (visible && x == cursor_x && y == cursor_y) return;
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

void cursor_protect(int x,int y,int width,int height)
{
    if(visible && width>0 && height>0 && (long)x+width>cursor_x &&
       x<(long)cursor_x+8 && (long)y+height>cursor_y && y<(long)cursor_y+8)
        cursor_hide();
}
