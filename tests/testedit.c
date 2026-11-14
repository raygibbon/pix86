#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "canvas.h"
#include "tools.h"
#include "history.h"
#include "view.h"
#include "editor.h"
#include "workpal.h"

static Editor editor;
static unsigned short queue[4096];
static unsigned char storage[4098], before[4096];

static unsigned char pixel(const Canvas *canvas, unsigned x, unsigned y)
{
    unsigned char colour;
    assert(canvas_get(canvas, x, y, &colour));
    return colour;
}

static void test_pixels(void)
{
    Canvas canvas;
    unsigned x, y, colour;
    memset(storage, 77, sizeof(storage));
    assert(canvas_init(&canvas, 64, 64, storage + 1, 4096));
    for (colour = 0; colour < 256; ++colour) {
        assert(canvas_set(&canvas, 63, 63, (unsigned char)colour));
        assert(pixel(&canvas, 63, 63) == colour);
    }
    assert(!canvas_set(&canvas, 64, 0, 1));
    assert(!canvas_set(&canvas, 0, 64, 1));
    assert(!canvas_set(&canvas, 65535U, 65535U, 1));
    assert(!canvas_set(NULL, 0, 0, 1));
    canvas_clear(&canvas, 255);
    for (y = 0; y < 64; ++y)
        for (x = 0; x < 64; ++x)
            assert(pixel(&canvas, x, y) == 255);
    assert(storage[0] == 77 && storage[4097] == 77);
}

static void test_view(void)
{
    unsigned side, x, y, index;
    int zoom, cx, cy, dx, dy;
    unsigned char colour;
    for (side = 16; side <= 64; side *= 2) {
        assert(editor_init(&editor, side, queue));
        memset(before, 123, side * side);
        memcpy(editor.canvas.pixels, before, side * side);
        for (zoom = 1; zoom <= view_max_zoom(&editor.canvas); ++zoom) {
            assert(view_set(&editor.view, &editor.canvas, zoom));
            assert(editor.view.x == DRAW_X + (DRAW_WIDTH - (int)side * zoom) / 2);
            assert(editor.view.y == DRAW_Y + (DRAW_HEIGHT - (int)side * zoom) / 2);
            for (y = 0; y < side; ++y)
                for (x = 0; x < side; ++x)
                    for (dy = 0; dy < zoom; ++dy)
                        for (dx = 0; dx < zoom; ++dx) {
                            assert(view_map(&editor.view,
                                editor.view.x + (int)x * zoom + dx,
                                editor.view.y + (int)y * zoom + dy, &cx, &cy));
                            assert(cx == (int)x && cy == (int)y);
                        }
            assert(!view_map(&editor.view, editor.view.x - 1, editor.view.y, &cx, &cy));
            assert(!view_map(&editor.view, editor.view.x, editor.view.y - 1, &cx, &cy));
            assert(!view_map(&editor.view, editor.view.x + (int)side * zoom,
                             editor.view.y, &cx, &cy));
            assert(!view_map(&editor.view, editor.view.x,
                             editor.view.y + (int)side * zoom, &cx, &cy));
            assert(memcmp(before, editor.canvas.pixels, side * side) == 0);
        }
        assert(!view_set(&editor.view, &editor.canvas, 0));
        assert(!view_set(&editor.view, &editor.canvas, view_max_zoom(&editor.canvas) + 1));
    }
    for (index = 0; index < 16; ++index) {
        assert(palette_hit(PALETTE_X + (int)(index % PALETTE_COLS) * PALETTE_CELL_W,
                          PALETTE_Y + (int)(index / PALETTE_COLS) * PALETTE_CELL_H, &colour));
        assert(colour == workpal_colour((int)index));
    }
    for(index=0;index<256;++index) {
        assert(workpal_full_hit(FULLPAL_X+(int)(index%16)*FULLPAL_CELL_W,
                               FULLPAL_Y+(int)(index/16)*FULLPAL_CELL_H,&colour));
        assert(colour==index);
    }
    assert(!palette_hit(PALETTE_X - 1, PALETTE_Y, &colour));
    assert(!palette_hit(PALETTE_X, PALETTE_Y - 1, &colour));
    assert(!palette_hit(320, PALETTE_Y, &colour));
    assert(!palette_hit(PALETTE_X, PALETTE_Y + PALETTE_ROWS * PALETTE_CELL_H, &colour));
}

static void test_lines(void)
{
    int x1, y1, x, y, count, previous, current;
    unsigned char colour;
    assert(editor_init(&editor, 64, queue));
    /* Every endpoint from a central point exercises all octants and slopes. */
    for (y1 = 0; y1 < 64; ++y1)
        for (x1 = 0; x1 < 64; ++x1) {
            canvas_clear(&editor.canvas, 0);
            tools_pencil(&editor.canvas, 31, 31, x1, y1, 255);
            assert(pixel(&editor.canvas, 31, 31) == 255);
            assert(pixel(&editor.canvas, (unsigned)x1, (unsigned)y1) == 255);
            /* Every column/row on the major axis has exactly one connected pixel. */
            previous = -1;
            if (abs(x1 - 31) >= abs(y1 - 31)) {
                for (x = x1 < 31 ? x1 : 31; x <= (x1 > 31 ? x1 : 31); ++x) {
                    count = 0; current = 0;
                    for (y = 0; y < 64; ++y)
                        if (pixel(&editor.canvas, (unsigned)x, (unsigned)y) == 255) {
                            ++count; current = y;
                        }
                    assert(count == 1);
                    assert(previous < 0 || (current - previous <= 1 && previous - current <= 1));
                    previous = current;
                }
            } else {
                for (y = y1 < 31 ? y1 : 31; y <= (y1 > 31 ? y1 : 31); ++y) {
                    count = 0; current = 0;
                    for (x = 0; x < 64; ++x)
                        if (pixel(&editor.canvas, (unsigned)x, (unsigned)y) == 255) {
                            ++count; current = x;
                        }
                    assert(count == 1);
                    assert(previous < 0 || (current - previous <= 1 && previous - current <= 1));
                    previous = current;
                }
            }
        }
    canvas_clear(&editor.canvas, 0);
    assert(tools_pencil(&editor.canvas, 0, 0, 63, 63, 17) == 64);
    assert(tools_pencil(&editor.canvas, 0, 0, 63, 63, 17) == 0);
    assert(tools_erase(&editor.canvas, 63, 63, 0, 0, 231) == 64);
    assert(tools_pick(&editor.canvas, 31, 31, &colour) && colour == 231);
    assert(!tools_pick(&editor.canvas, 64, 0, &colour));
    assert(!tools_pencil(&editor.canvas, -1, 0, 2, 2, 1));
    assert(!tools_pencil(&editor.canvas, 0, 0, 64, 2, 1));
}

static void test_fill(void)
{
    unsigned x, y;
    assert(editor_init(&editor, 64, queue));
    assert(tools_fill(&editor.canvas, 0, 0, 255, queue) == 4096);
    for (y = 0; y < 64; ++y)
        for (x = 0; x < 64; ++x)
            assert(pixel(&editor.canvas, x, y) == 255);
    assert(tools_fill(&editor.canvas, 63, 63, 0, queue) == 4096);
    assert(tools_fill(&editor.canvas, 63, 63, 0, queue) == 0);
    assert(tools_fill(&editor.canvas, 64, 0, 1, queue) == 0);
    assert(tools_fill(&editor.canvas, 0, 64, 1, queue) == 0);
    assert(tools_fill(&editor.canvas, 0, 0, 1, NULL) == 0);
    tools_pencil(&editor.canvas, 10, 10, 20, 10, 2);
    tools_pencil(&editor.canvas, 20, 10, 20, 20, 2);
    tools_pencil(&editor.canvas, 20, 20, 10, 20, 2);
    tools_pencil(&editor.canvas, 10, 20, 10, 10, 2);
    assert(tools_fill(&editor.canvas, 11, 11, 7, queue) == 81);
    assert(pixel(&editor.canvas, 10, 10) == 2);
    assert(pixel(&editor.canvas, 21, 21) == 0);
    /* Matching diagonal pixels do not connect across other colours. */
    canvas_clear(&editor.canvas, 1);
    canvas_set(&editor.canvas, 0, 0, 0);
    canvas_set(&editor.canvas, 1, 1, 0);
    assert(tools_fill(&editor.canvas, 0, 0, 255, queue) == 1);
    assert(pixel(&editor.canvas, 1, 1) == 0);
    /* One-cell-wide winding corridor exercises queue reuse and boundaries. */
    canvas_clear(&editor.canvas, 1);
    for (y = 0; y < 64; y += 2) {
        for (x = 0; x < 64; ++x) canvas_set(&editor.canvas, x, y, 0);
        if (y < 62) canvas_set(&editor.canvas, (y % 4) ? 0 : 63, y + 1, 0);
    }
    assert(tools_fill(&editor.canvas, 0, 0, 123, queue) == 2079);
}

static void test_history(void)
{
    unsigned i;
    assert(editor_init(&editor, 64, queue));
    assert(!history_undo(&editor.history, &editor.canvas));
    assert(!history_redo(&editor.history, &editor.canvas));
    for (i = 1; i <= 10; ++i) {
        canvas_clear(&editor.canvas, (unsigned char)i);
        assert(history_commit(&editor.history, &editor.canvas));
    }
    for (i = 9; i >= 6; --i) {
        assert(history_undo(&editor.history, &editor.canvas));
        assert(pixel(&editor.canvas, 0, 0) == i);
        assert(pixel(&editor.canvas, 63, 63) == i);
    }
    assert(!history_undo(&editor.history, &editor.canvas));
    for (i = 7; i <= 10; ++i) {
        assert(history_redo(&editor.history, &editor.canvas));
        assert(pixel(&editor.canvas, 63, 63) == i);
    }
    assert(!history_redo(&editor.history, &editor.canvas));
    assert(history_undo(&editor.history, &editor.canvas));
    assert(!history_commit(&editor.history, &editor.canvas));
    assert(history_redo(&editor.history, &editor.canvas));
    assert(history_undo(&editor.history, &editor.canvas));
    canvas_clear(&editor.canvas, 255);
    assert(history_commit(&editor.history, &editor.canvas));
    assert(!history_redo(&editor.history, &editor.canvas));
    assert(history_undo(&editor.history, &editor.canvas));
    assert(pixel(&editor.canvas, 63, 63) == 9);
}

static void mouse_cell(int x, int y, unsigned buttons)
{
    editor_mouse(&editor, editor.view.x + x * editor.view.zoom,
                 editor.view.y + y * editor.view.zoom, buttons);
}

static void test_interactions(void)
{
    unsigned i;
    assert(editor_init(&editor, 16, queue));
    mouse_cell(0, 0, EDIT_LEFT);
    mouse_cell(7, 7, EDIT_LEFT);
    mouse_cell(15, 15, 0); /* fast final movement and release is included */
    assert(editor.history.position == 1);
    for (i = 0; i < 16; ++i) assert(pixel(&editor.canvas, i, i) == 15);
    editor_key(&editor, 26);
    for (i = 0; i < 16; ++i) assert(pixel(&editor.canvas, i, i) == 0);
    editor_key(&editor, 25);
    for (i = 0; i < 16; ++i) assert(pixel(&editor.canvas, i, i) == 15);
    editor_key(&editor, 'e');
    editor.background = 231;
    mouse_cell(0, 0, EDIT_LEFT);
    mouse_cell(15, 15, 0);
    assert(pixel(&editor.canvas, 7, 7) == 231);
    editor_key(&editor, 'u');
    assert(pixel(&editor.canvas, 7, 7) == 15);
    editor_key(&editor, 'i');
    mouse_cell(7, 7, EDIT_LEFT);
    mouse_cell(7, 7, 0);
    assert(editor.foreground == 15);
    assert(editor.history.count - editor.history.position - 1 == 1);
    editor_key(&editor, 'f');
    editor.foreground = 255;
    mouse_cell(0, 15, EDIT_LEFT);
    mouse_cell(1, 15, EDIT_LEFT); /* holding fill must not create another action */
    mouse_cell(1, 15, 0);
    assert(editor.history.position == 2);
    assert(!history_redo(&editor.history, &editor.canvas));
    assert(pixel(&editor.canvas, 0, 15) == 255);
    assert(pixel(&editor.canvas, 15, 0) == 0); /* diagonal wall separates sides */
    editor_key(&editor, 'u');
    assert(pixel(&editor.canvas, 0, 15) == 0);
    editor_key(&editor, 'p');
    mouse_cell(0, 15, EDIT_LEFT);
    editor_mouse(&editor, PALETTE_X, PALETTE_Y, EDIT_LEFT);
    mouse_cell(15, 0, EDIT_LEFT);
    mouse_cell(15, 0, 0);
    assert(pixel(&editor.canvas, 7, 8) == 0); /* do not bridge an excursion into UI */
    editor_key(&editor, 'u');
    mouse_cell(2, 8, EDIT_LEFT);
    mouse_cell(4, 8, EDIT_LEFT);
    editor_key(&editor, 'u'); /* closes and undoes the active drag */
    mouse_cell(8, 8, EDIT_LEFT);
    mouse_cell(8, 8, 0);
    assert(pixel(&editor.canvas, 2, 8) == 0);
    assert(pixel(&editor.canvas, 4, 8) == 0);
    assert(pixel(&editor.canvas, 8, 8) == 15);
    memcpy(before, editor.canvas.pixels, 256);
    for (i = 0; i < 256; ++i) {
        int slot;
        workpal_remember((unsigned char)i);
        for(slot=0;slot<16 && workpal_colour(slot)!=i;++slot) { }
        assert(slot<16);
        editor_mouse(&editor, PALETTE_X + slot%PALETTE_COLS * PALETTE_CELL_W,
                     PALETTE_Y + slot/PALETTE_COLS * PALETTE_CELL_H, EDIT_LEFT);
        editor_mouse(&editor, PALETTE_X, PALETTE_Y, 0);
        assert(editor.foreground == i);
    }
    editor_mouse(&editor, PALETTE_X + 15%PALETTE_COLS * PALETTE_CELL_W,
                 PALETTE_Y + 15/PALETTE_COLS * PALETTE_CELL_H, EDIT_RIGHT);
    editor_mouse(&editor, PALETTE_X, PALETTE_Y, 0);
    assert(editor.background == 255);
    editor_key(&editor, '-');
    editor_key(&editor, '+');
    assert(!editor.grid);
    editor_key(&editor, 'g');
    assert(editor.grid);
    editor_key(&editor, 'g');
    assert(!editor.grid);
    assert(memcmp(before, editor.canvas.pixels, 256) == 0);
    editor_key(&editor, 'c');
    for (i = 0; i < 256; ++i) assert(editor.canvas.pixels[i] == 255);
    editor_key(&editor, 'u');
    assert(memcmp(before, editor.canvas.pixels, 256) == 0);
    /* A press starting outside the canvas cannot turn into a paint drag. */
    editor_mouse(&editor, 6, 32, EDIT_LEFT);
    mouse_cell(5, 8, EDIT_LEFT);
    mouse_cell(5, 8, 0);
    assert(memcmp(before, editor.canvas.pixels, 256) == 0);
    assert(!editor_init(&editor, 8, queue));
}

int main(void)
{
    test_pixels();
    test_view();
    test_lines();
    test_fill();
    test_history();
    test_interactions();
    puts("PASS: M2 pixels, zoom, lines, fill, history, palette and editor gestures.");
    return 0;
}
