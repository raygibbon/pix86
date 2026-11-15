#ifndef PIX86_SCREEN_H
#define PIX86_SCREEN_H
#include "render.h"
#include "menu.h"
#include "mouse.h"
#include "ui.h"
typedef struct Screen {
    Menu menu;
    Flyout flyout;
    unsigned nav_active,nav_count,first_frame;
    unsigned playback_render_ms,playback_fps;
    unsigned long playback_dropped,playback_window,playback_frames;
    int playback_active,timeline;
    unsigned char groups[2];
    int tool,alpha,active_group,playing;
    unsigned char foreground,background;
    char status[UI_STATUS_CHARS+1];
    char filename[13];
    unsigned char working[16];
} Screen;
void screen_filename(Screen *screen,const char *path);
void screen_init(Screen *screen, const Editor *editor, const MouseState *mouse, int has_mouse);
void screen_present(Screen *screen,const Editor *editor,const UiState *ui,unsigned flags,
                    const MouseState *mouse,int has_mouse);
void screen_update(Screen *screen,const Editor *editor,const Menu *menu,unsigned flags,
                   const MouseState *mouse,int has_mouse);
#endif
