#ifndef PIX86_EDITOR_H
#define PIX86_EDITOR_H
#include "canvas.h"
#include "select.h"
#include "history.h"
#include "view.h"
#include "fileio.h"
#define TOOL_PENCIL 0
#define TOOL_ERASER 1
#define TOOL_FILL 2
#define TOOL_PICK 3
#define TOOL_SELECT 4
#define TOOL_MOVE 5
#define TOOL_COUNT 6
#define EDIT_SELECTION 32
#define EDIT_UI 1
#define EDIT_CANVAS 2
#define EDIT_STATUS 4
#define EDIT_STROKE 8
#define EDIT_TIMELINE 16
#define EDIT_PLAYBACK 64
#define EDIT_LEFT 1
#define EDIT_RIGHT 2

typedef struct Editor {
    Canvas canvas;
    Selection selection;
    unsigned char group_tool[2];
    unsigned char pixels[4096];
    unsigned char mask[512];
    History history;
    Animation ANIM_FAR *document, ANIM_FAR *saved;
    unsigned saved_width, saved_height;
    unsigned char saved_foreground, saved_background;
    unsigned first_frame, elapsed;
    unsigned playback_previous;
    unsigned long playback_dropped;
    int playing;
    View view;
    unsigned short *fill_queue;
    unsigned char foreground, background, stroke_colour, preview_colour;
    int preview_link, checker, transparent_eraser, stroke_transparent;
    int timeline, guide, tool, grid, dragging, segment_valid, last_x, last_y;
    int dirty_x0, dirty_y0, dirty_x1, dirty_y1;
    unsigned previous_buttons;
} Editor;

/* Init a fresh object; dispose before reinitializing. New/Open reuse its storage.
   fill_queue has 4096 entries. Dimensions are checked by anim_dimensions. */
int editor_init(Editor *editor, unsigned side, unsigned short *fill_queue);
void editor_dispose(Editor *editor);
int editor_new_document(Editor *editor,unsigned width,unsigned height,int transparent,unsigned char background);
int editor_new_shape(Editor *editor,unsigned width,unsigned height);
unsigned editor_tick(Editor *editor,unsigned milliseconds);
unsigned editor_select(Editor *editor,unsigned frame);
void editor_store(Editor *editor);
void editor_restore(Editor *editor);
unsigned editor_background(Editor *editor,unsigned char colour);
unsigned editor_mouse(Editor *editor, int x, int y, unsigned buttons);
unsigned editor_key(Editor *editor, int key);
void editor_finish(Editor *editor);
void editor_mark_saved(Editor *editor);
int editor_is_dirty(const Editor *editor);
void editor_image(const Editor *editor, P86Image *image);
int editor_load(Editor *editor, const P86Image *image);
#endif
