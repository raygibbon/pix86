#include <stdio.h>
#include "render.h"
#include "video.h"

void render_ui(const Editor *editor)
{
    char text[41];
    int tool, x, y;
    unsigned index;
    static const char *labels[4] = {"PEN", "ERS", "FIL", "PCK"};
    video_rect(0, 0, 320, 16, 5);
    sprintf(text, "PIX86 %ux%u  +/- ZOOM", editor->canvas.width,
            editor->canvas.height);
    video_text(4, 4, text, 15);
    video_text(288, 4, "ESC", 15);
    video_rect(0, 16, 36, 176, 5);
    for (tool = 0; tool < 4; ++tool) {
        y = 24 + tool * 30;
        video_rect(2, y, 32, 24, tool == editor->tool ? 15 : 1);
        video_rect(3, y + 1, 30, 22, tool == editor->tool ? 8 : 4);
        video_text(6, y + 8, labels[tool], 15);
    }
    video_text(4, 148, "P E", 15);
    video_text(4, 160, "F I", 15);
    video_text(4, 180, "U R", 15);
    video_rect(224, 16, 96, 176, 3);
    sprintf(text, "FG:%03u", (unsigned)editor->foreground);
    video_text(224, 20, text, 15);
    video_rect(280, 19, 12, 10, 15);
    video_rect(281, 20, 10, 8, editor->foreground);
    for (index = 0; index < 256; ++index) {
        x = PALETTE_X + (int)(index % 16) * PALETTE_CELL_W;
        y = PALETTE_Y + (int)(index / 16) * PALETTE_CELL_H;
        video_rect(x, y, PALETTE_CELL_W, PALETTE_CELL_H,
                   index == editor->foreground ? 15 : 1);
        video_rect(x + 1, y + 1, PALETTE_CELL_W - 2,
                   PALETTE_CELL_H - 2, (unsigned char)index);
    }
    sprintf(text, "BG:%03u", (unsigned)editor->background);
    video_text(224, 180, text, 15);
    video_rect(280, 179, 12, 10, 15);
    video_rect(281, 180, 10, 8, editor->background);
}

void render_canvas(const Editor *editor)
{
    unsigned x, y;
    int zoom, cell;
    const View *view;
    view = &editor->view;
    zoom = view->zoom;
    cell = editor->grid && zoom >= 2 ? zoom - 1 : zoom;
    video_rect(DRAW_X - 1, DRAW_Y - 1, DRAW_WIDTH + 2, DRAW_HEIGHT + 2, 3);
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
    char text[41], coords[12];
    int x, y;
    static const char *names[4] = {"PEN", "ERS", "FIL", "PCK"};
    video_rect(0, 192, 320, 8, 5);
    if (!has_mouse) {
        video_text(0, 192, "NO MOUSE DRIVER - ESC TO EXIT", 15);
        return;
    }
    if (view_map(&editor->view, mouse_x, mouse_y, &x, &y))
        sprintf(coords, "%02d,%02d", x, y);
    else
        sprintf(coords, "--,--");
    sprintf(text, "%s %s Z:%d G:%d FG:%03u U:%u R:%u", names[editor->tool], coords,
            editor->view.zoom, editor->grid, (unsigned)editor->foreground,
            editor->history.position,
            editor->history.count - editor->history.position - 1);
    video_text(0, 192, text, 15);
}
