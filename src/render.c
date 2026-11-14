#include <stdio.h>
#include <string.h>
#include "render.h"
#include "video.h"
#include "theme.h"

void render_ui(const Editor *editor)
{
    char text[12];
    int tool, x, y;
    unsigned index;
    static const char *labels[4] = {"Pen", "Ers", "Fil", "Pck"};
    video_rect(0, 10, 22, 182, UI_DESK);
    for (tool = 0; tool < 4; ++tool) {
        y = TOOL_Y + tool * TOOL_STEP;
        if (tool == editor->tool) video_rect(TOOL_X, y, TOOL_WIDTH, TOOL_HEIGHT, UI_BAR);
        video_text(2, y + 2, labels[tool], tool == editor->tool ? UI_INK : UI_WHITE);
    }
    video_rect(250, 10, 70, 182, UI_DESK);
    video_text(254, 13, "Palette", UI_WHITE);
    for (index = 0; index < 256; ++index) {
        x = PALETTE_X + (int)(index % 16) * PALETTE_CELL_W;
        y = PALETTE_Y + (int)(index / 16) * PALETTE_CELL_H;
        video_rect(x, y, PALETTE_CELL_W, PALETTE_CELL_H,
                   index == editor->foreground ? UI_WHITE : UI_INK);
        video_rect(x + 1, y + 1, PALETTE_CELL_W - 2,
                   PALETTE_CELL_H - 2, (unsigned char)index);
    }
    sprintf(text, "FG %03u", (unsigned)editor->foreground);
    video_text(254, 126, text, UI_WHITE);
    video_rect(300, 125, 16, 10, UI_WHITE);
    video_rect(301, 126, 14, 8, editor->foreground);
    sprintf(text, "BG %03u", (unsigned)editor->background);
    video_text(254, 140, text, UI_WHITE);
    video_rect(300, 139, 16, 10, UI_WHITE);
    video_rect(301, 140, 14, 8, editor->background);
    video_text(254, 165, "L:FG R:BG", UI_WHITE);
    video_text(254, 178, "G: grid", UI_WHITE);
}

void render_canvas(const Editor *editor)
{
    unsigned x, y;
    int zoom, cell;
    const View *view;
    view = &editor->view;
    zoom = view->zoom;
    cell = editor->grid && zoom >= 2 ? zoom - 1 : zoom;
    video_rect(DRAW_X - 1, DRAW_Y - 1, DRAW_WIDTH + 2, DRAW_HEIGHT + 2, UI_DESK);
    video_rect(view->x - 1, view->y - 1, (int)view->width * zoom + 2,
               (int)view->height * zoom + 2, 8);
    for (y = 0; y < editor->canvas.height; ++y)
        for (x = 0; x < editor->canvas.width; ++x)
            video_rect(view->x + (int)x * zoom, view->y + (int)y * zoom,
                       cell, cell, editor->canvas.pixels[y * editor->canvas.width + x]);
}

void render_stroke(const Editor *editor)
{
    int x, y, sx, sy, cell, zoom;
    zoom = editor->view.zoom;
    cell = editor->grid && zoom >= 2 ? zoom - 1 : zoom;
    for (y = editor->dirty_y0; y <= editor->dirty_y1; ++y)
        for (x = editor->dirty_x0; x <= editor->dirty_x1; ++x) {
            sx = editor->view.x + x * zoom;
            sy = editor->view.y + y * zoom;
            video_rect(sx, sy, zoom, zoom, 8);
            video_rect(sx, sy, cell, cell,
                       editor->canvas.pixels[(unsigned)y * editor->canvas.width + x]);
        }
}

void render_status(const Editor *editor, int mouse_x, int mouse_y, int has_mouse)
{
    char text[54], coords[12];
    int x, y;
    static const char *names[4] = {"Pen", "Ers", "Fil", "Pck"};
    video_rect(0, 192, 320, 8, UI_BAR);
    if (has_mouse && view_map(&editor->view, mouse_x, mouse_y, &x, &y))
        sprintf(coords, "X%02dY%02d", x, y);
    else strcpy(coords, "X--Y--");
    sprintf(text, "%c%ux%u %s %s FG%03u BG%03u Z%d G%d U%u R%u%s",
            editor_is_dirty(editor) ? '*' : ' ', editor->canvas.width,
            editor->canvas.height, coords, names[editor->tool],
            (unsigned)editor->foreground, (unsigned)editor->background,
            editor->view.zoom, editor->grid, editor->history.position,
            editor->history.count - editor->history.position - 1,
            has_mouse ? "" : " No mouse");
    video_text(0, 192, text, UI_INK);
}
