#ifndef PIX86_SCREEN_H
#define PIX86_SCREEN_H
#include "render.h"
#include "menu.h"
#include "mouse.h"
typedef struct Screen {
    Menu menu;
    int tool;
    unsigned char foreground,background;
    char status[UI_STATUS_CHARS+1];
    char filename[13];
} Screen;
void screen_filename(Screen *screen,const char *path);
void screen_init(Screen *screen, const Editor *editor, const MouseState *mouse, int has_mouse);
void screen_update(Screen *screen,const Editor *editor,const Menu *menu,unsigned flags,
                   const MouseState *mouse,int has_mouse);
#endif
