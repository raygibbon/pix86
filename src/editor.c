#include <ctype.h>
#include <stddef.h>
#include <string.h>
#include "editor.h"
#include "tools.h"

static void stroke_bounds(Editor *editor, int x0, int y0, int x1, int y1)
{
    editor->dirty_x0 = x0 < x1 ? x0 : x1;
    editor->dirty_x1 = x0 > x1 ? x0 : x1;
    editor->dirty_y0 = y0 < y1 ? y0 : y1;
    editor->dirty_y1 = y0 > y1 ? y0 : y1;
}

int editor_init(Editor *editor, unsigned side, unsigned short *fill_queue)
{
    if (editor == NULL || fill_queue == NULL ||
        (side != 16 && side != 32 && side != 64))
        return 0;
    canvas_init(&editor->canvas, side, side, editor->pixels, sizeof(editor->pixels));
    editor->background = 0;
    editor->foreground = 15;
    editor->tool = TOOL_PENCIL;
    editor->grid = 1;
    editor->dragging = editor->segment_valid = 0;
    editor->previous_buttons = 0;
    editor->fill_queue = fill_queue;
    canvas_clear(&editor->canvas, editor->background);
    history_init(&editor->history, &editor->canvas);
    view_set(&editor->view, &editor->canvas, view_max_zoom(&editor->canvas));
    editor_mark_saved(editor);
    return 1;
}

void editor_finish(Editor *editor)
{
    if (editor->dragging)
        history_commit(&editor->history, &editor->canvas);
    editor->dragging = editor->segment_valid = 0;
}

unsigned editor_mouse(Editor *editor, int x, int y, unsigned buttons)
{
    int cx, cy, inside, tool;
    unsigned pressed, changed, flags;
    unsigned char index;
    pressed = buttons & ~editor->previous_buttons;
    editor->previous_buttons = buttons;
    flags = EDIT_STATUS;
    inside = view_map(&editor->view, x, y, &cx, &cy);
    if (editor->dragging) {
        if (inside) {
            if (!editor->segment_valid) {
                editor->last_x = cx;
                editor->last_y = cy;
            }
            if (editor->tool == TOOL_ERASER)
                changed = tools_erase(&editor->canvas, editor->last_x,
                          editor->last_y, cx, cy, editor->stroke_colour);
            else
                changed = tools_pencil(&editor->canvas, editor->last_x,
                          editor->last_y, cx, cy, editor->stroke_colour);
            if (changed) {
                stroke_bounds(editor, editor->last_x, editor->last_y, cx, cy);
                flags |= EDIT_STROKE;
            }
            editor->last_x = cx;
            editor->last_y = cy;
        }
        editor->segment_valid = inside;
        if (!(buttons & EDIT_LEFT))
            editor_finish(editor);
        return flags;
    }
    if (pressed == 0)
        return flags;
    if (palette_hit(x, y, &index)) {
        if (pressed & EDIT_LEFT) editor->foreground = index;
        else if (pressed & EDIT_RIGHT) editor->background = index;
        return flags | EDIT_UI;
    }
    tool = toolbar_hit(x, y);
    if ((pressed & EDIT_LEFT) && tool >= 0) {
        editor->tool = tool;
        return flags | EDIT_UI;
    }
    if (!inside)
        return flags;
    if ((pressed & EDIT_RIGHT) ||
        ((pressed & EDIT_LEFT) && editor->tool == TOOL_PICK)) {
        tools_pick(&editor->canvas, (unsigned)cx, (unsigned)cy,
                   &editor->foreground);
        return flags | EDIT_UI;
    }
    if (!(pressed & EDIT_LEFT))
        return flags;
    if (editor->tool == TOOL_FILL) {
        if (tools_fill(&editor->canvas, (unsigned)cx, (unsigned)cy,
                       editor->foreground, editor->fill_queue)) {
            history_commit(&editor->history, &editor->canvas);
            flags |= EDIT_CANVAS;
        }
    } else {
        editor->dragging = editor->segment_valid = 1;
        editor->last_x = cx;
        editor->last_y = cy;
        editor->stroke_colour = editor->tool == TOOL_ERASER ?
                                editor->background : editor->foreground;
        if (tools_pencil(&editor->canvas, cx, cy, cx, cy, editor->stroke_colour)) {
            stroke_bounds(editor, cx, cy, cx, cy);
            flags |= EDIT_STROKE;
        }
    }
    return flags;
}

unsigned editor_key(Editor *editor, int key)
{
    int zoom, tool;
    if (key < 0 || key > 255)
        return 0;
    key = tolower((unsigned char)key);
    tool = -1;
    if (key == 'p') tool = TOOL_PENCIL;
    if (key == 'e') tool = TOOL_ERASER;
    if (key == 'f') tool = TOOL_FILL;
    if (key == 'i') tool = TOOL_PICK;
    if (tool >= 0) {
        editor_finish(editor);
        editor->tool = tool;
        return EDIT_UI | EDIT_STATUS;
    }
    if (key == 'g') {
        editor_finish(editor);
        editor->grid = !editor->grid;
        return EDIT_CANVAS | EDIT_STATUS;
    }
    if (key == 'c') {
        editor_finish(editor);
        canvas_clear(&editor->canvas, editor->background);
        history_commit(&editor->history, &editor->canvas);
        return EDIT_CANVAS | EDIT_STATUS;
    }
    if (key == 'u' || key == 26 || key == 'r' || key == 25) {
        editor_finish(editor);
        if (key == 'u' || key == 26)
            history_undo(&editor->history, &editor->canvas);
        else
            history_redo(&editor->history, &editor->canvas);
        return EDIT_CANVAS | EDIT_STATUS;
    }
    if (key == '+' || key == '=' || key == '-') {
        editor_finish(editor);
        zoom = editor->view.zoom + (key == '-' ? -1 : 1);
        if (view_set(&editor->view, &editor->canvas, zoom))
            return EDIT_CANVAS | EDIT_STATUS;
        return EDIT_STATUS;
    }
    return 0;
}

void editor_mark_saved(Editor *editor)
{
    editor_finish(editor);
    editor->saved_width = editor->canvas.width;
    editor->saved_height = editor->canvas.height;
    editor->saved_foreground = editor->foreground;
    editor->saved_background = editor->background;
    memcpy(editor->saved_pixels, editor->canvas.pixels,
           editor->canvas.width * editor->canvas.height);
}

int editor_is_dirty(const Editor *editor)
{
    return editor->saved_width != editor->canvas.width ||
           editor->saved_height != editor->canvas.height ||
           editor->saved_foreground != editor->foreground ||
           editor->saved_background != editor->background ||
           memcmp(editor->saved_pixels, editor->canvas.pixels,
                  editor->canvas.width * editor->canvas.height) != 0;
}

void editor_image(const Editor *editor, P86Image *image)
{
    image->width = editor->canvas.width;
    image->height = editor->canvas.height;
    image->foreground = editor->foreground;
    image->background = editor->background;
    memcpy(image->pixels, editor->pixels, image->width * image->height);
}

int editor_load(Editor *editor, const P86Image *image)
{
    if (image == NULL || image->width != image->height ||
        (image->width != 16 && image->width != 32 && image->width != 64))
        return 0;
    editor_finish(editor);
    canvas_init(&editor->canvas, image->width, image->height,
                editor->pixels, sizeof(editor->pixels));
    memcpy(editor->pixels, image->pixels, image->width * image->height);
    editor->foreground = image->foreground;
    editor->background = image->background;
    editor->previous_buttons = 0;
    history_init(&editor->history, &editor->canvas);
    view_set(&editor->view, &editor->canvas, view_max_zoom(&editor->canvas));
    editor_mark_saved(editor);
    return 1;
}
