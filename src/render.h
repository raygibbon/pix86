#ifndef PIX86_RENDER_H
#define PIX86_RENDER_H
#include "editor.h"
void render_ui(const Editor *editor);
void render_canvas(const Editor *editor);
void render_stroke(const Editor *editor);
void render_status(const Editor *editor, int mouse_x, int mouse_y, int has_mouse);
#endif
