#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "canvas.h"
#include "cursor.h"
#include "video.h"

static unsigned char screen[VIDEO_WIDTH * VIDEO_HEIGHT];
static unsigned char baseline[VIDEO_WIDTH * VIDEO_HEIGHT];

void video_pixel(int x, int y, unsigned char colour)
{
    if (x >= 0 && x < VIDEO_WIDTH && y >= 0 && y < VIDEO_HEIGHT)
        screen[y * VIDEO_WIDTH + x] = colour;
}

unsigned char video_read(int x, int y)
{
    if (x < 0 || x >= VIDEO_WIDTH || y < 0 || y >= VIDEO_HEIGHT)
        return 0;
    return screen[y * VIDEO_WIDTH + x];
}

static void test_canvas(void)
{
    Canvas canvas;
    unsigned char storage[18], colour;
    memset(storage, 99, sizeof(storage));
    assert(canvas_init(&canvas, 4, 4, storage + 1, 16));
    canvas_clear(&canvas, 231);
    assert(storage[0] == 99 && storage[17] == 99);
    assert(canvas_get(&canvas, 0, 0, &colour) && colour == 231);
    assert(canvas_get(&canvas, 3, 3, &colour) && colour == 231);
    assert(!canvas_get(&canvas, 4, 0, &colour));
    assert(!canvas_get(&canvas, 0, 4, &colour));
    assert(!canvas_get(&canvas, 0, 0, NULL));
    assert(!canvas_init(&canvas, 4, 4, storage, 15));
    assert(canvas.pixels == NULL && canvas.width == 0);
    canvas_clear(&canvas, 1);
    assert(!canvas_get(&canvas, 0, 0, &colour));
    assert(!canvas_init(&canvas, 0, 1, storage, 18));
    assert(!canvas_init(&canvas, 1, 0, storage, 18));
    assert(!canvas_init(&canvas, 1, 1, NULL, 18));
    assert(!canvas_init(NULL, 1, 1, storage, 18));
    assert(!canvas_init(&canvas, 65535U, 65535U, storage, 18));
    assert(!canvas_init(&canvas, 256, 128, storage, 18));
}

static void test_cursor(void)
{
    int x, y;
    unsigned i;
    for (i = 0; i < sizeof(screen); ++i)
        screen[i] = (unsigned char)(i % 251);
    memcpy(baseline, screen, sizeof(screen));
    cursor_hide();
    for (y = 0; y < VIDEO_HEIGHT; ++y)
        for (x = 0; x < VIDEO_WIDTH; ++x) {
            cursor_show(x, y);
            cursor_hide();
            assert(memcmp(screen, baseline, sizeof(screen)) == 0);
        }
    cursor_show(100, 100);
    cursor_show(102, 102); /* overlapping old and new saved areas */
    cursor_hide();
    cursor_hide();
    assert(memcmp(screen, baseline, sizeof(screen)) == 0);
    cursor_show(0, 192);
    cursor_hide(); /* status update must happen after hide */
    video_pixel(1, 193, 123);
    cursor_show(0, 192);
    cursor_hide();
    assert(video_read(1, 193) == 123);
}

int main(void)
{
    test_canvas();
    test_cursor();
    puts("PASS: canvas bounds/capacity; cursor restoration at all screen positions.");
    return 0;
}
