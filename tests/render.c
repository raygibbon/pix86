#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "editor.h"
#include "render.h"
#include "video.h"

static Editor editor;
static unsigned short queue[4096];
static unsigned char screen[64000];

void video_rect(int x, int y, int w, int h, unsigned char colour)
{
    int xx, yy;
    for (yy = y; yy < y + h; ++yy)
        for (xx = x; xx < x + w; ++xx)
            if (xx >= 0 && yy >= 0 && xx < 320 && yy < 200)
                screen[yy * 320 + xx] = colour;
}

void video_text(int x, int y, const char *text, unsigned char colour)
{
    (void)x; (void)y; (void)colour;
    assert(strlen(text) <= 40);
}

static void test_render(unsigned side)
{
    unsigned x, y, index;
    int zoom, sx, sy;
    assert(editor_init(&editor, side, queue));
    for (y = 0; y < side; ++y)
        for (x = 0; x < side; ++x)
            canvas_set(&editor.canvas, x, y, (unsigned char)(y * side + x));
    for (zoom = view_max_zoom(&editor.canvas); zoom >= 1; --zoom) {
        view_set(&editor.view, &editor.canvas, zoom);
        render_canvas(&editor);
        for (y = 0; y < side; ++y)
            for (x = 0; x < side; ++x) {
                sx = editor.view.x + (int)x * zoom;
                sy = editor.view.y + (int)y * zoom;
                assert(screen[sy * 320 + sx] == (unsigned char)(y * side + x));
                if (zoom > 1)
                    assert(screen[(sy + zoom - 1) * 320 + sx + zoom - 1] == 8);
            }
        assert(screen[DRAW_Y * 320 + DRAW_X] == 3); /* old zoom cleared */
    }
    editor.grid = 0;
    view_set(&editor.view, &editor.canvas, view_max_zoom(&editor.canvas));
    render_canvas(&editor);
    sx = editor.view.x; sy = editor.view.y;
    zoom = editor.view.zoom;
    assert(screen[(sy + zoom - 1) * 320 + sx + zoom - 1] == 0);
    render_ui(&editor);
    for (index = 0; index < 256; ++index) {
        sx = PALETTE_X + (int)(index % 16) * PALETTE_CELL_W + 1;
        sy = PALETTE_Y + (int)(index / 16) * PALETTE_CELL_H + 1;
        assert(screen[sy * 320 + sx] == index);
    }
    render_status(&editor, 319, 199, 1);
    render_status(&editor, editor.view.x, editor.view.y, 1);
    render_status(&editor, 0, 0, 0);
    sx = editor.view.x + zoom;
    sy = editor.view.y + zoom;
    assert(editor_mouse(&editor, sx, sy, EDIT_LEFT) & EDIT_STROKE);
    assert(editor.dirty_x0 == 1 && editor.dirty_x1 == 1);
    assert(editor.dirty_y0 == 1 && editor.dirty_y1 == 1);
    render_stroke(&editor);
    assert(screen[sy * 320 + sx] == editor.foreground);
    assert(screen[editor.view.y * 320 + editor.view.x] == 0);
    editor.grid = 1;
    render_canvas(&editor);
    editor.foreground = 255;
    editor_mouse(&editor, sx, sy, 0);
    editor_mouse(&editor, sx, sy, EDIT_LEFT);
    render_stroke(&editor);
    assert(screen[sy * 320 + sx] == 255);
    assert(screen[(sy + zoom - 1) * 320 + sx + zoom - 1] == 8);
}

int main(void)
{
    test_render(16);
    test_render(32);
    test_render(64);
    puts("PASS: canvas rendering at every zoom, grids and all palette indices.");
    return 0;
}
