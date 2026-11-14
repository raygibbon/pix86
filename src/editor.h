#ifndef PIX86_EDITOR_H
#define PIX86_EDITOR_H
#include "canvas.h"
#include "history.h"
#include "view.h"
#define TOOL_PENCIL 0
#define TOOL_ERASER 1
#define TOOL_FILL 2
#define TOOL_PICK 3
#define EDIT_UI 1
#define EDIT_CANVAS 2
#define EDIT_STATUS 4
#define EDIT_STROKE 8
#define EDIT_LEFT 1
#define EDIT_RIGHT 2

typedef struct Editor {
    Canvas canvas;
    unsigned char pixels[4096];
    History history;
    View view;
    unsigned short *fill_queue;
    unsigned char foreground, background, stroke_colour;
    int tool, grid, dragging, segment_valid, last_x, last_y;
    int dirty_x0, dirty_y0, dirty_x1, dirty_y1;
    unsigned previous_buttons;
} Editor;

/* Supported square sizes: 16,32,64; fill_queue has 4096 entries. */
int editor_init(Editor *editor, unsigned side, unsigned short *fill_queue);
unsigned editor_mouse(Editor *editor, int x, int y, unsigned buttons);
unsigned editor_key(Editor *editor, int key);
void editor_finish(Editor *editor);
#endif
