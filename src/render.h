#ifndef PIX86_RENDER_H
#define PIX86_RENDER_H
#include "editor.h"
#include "region.h"
void render_restore(const Editor *editor, const Rect *rect);
void render_ui_change(const Editor *editor, int tool, unsigned char foreground, unsigned char background, const unsigned char *working);
void render_status_text(const Editor *editor, int mouse_x, int mouse_y, int has_mouse, const char *filename, char *text);
void render_ui(const Editor *editor);
void render_canvas(const Editor *editor);
void render_stroke(const Editor *editor);
void render_status(const Editor *editor, int mouse_x, int mouse_y, int has_mouse);
#endif
