#include <stdio.h>
#include <string.h>
#include <dos.h>
#include "editor.h"
#include "render.h"
#include "video.h"
#include "mouse.h"
#include "keyboard.h"
#include "cursor.h"

/* No large automatic objects or full-screen buffers in the small model. */
static Editor editor;
static unsigned short fill_queue[4096];

int main(int argc, char **argv)
{
    MouseState mouse, previous;
    unsigned side, flags;
    int has_mouse, disable_mouse, key, i, size_set;
    side = 32;
    size_set = disable_mouse = 0;
    for (i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "/NOMOUSE") == 0 && !disable_mouse)
            disable_mouse = 1;
        else if (!size_set && (strcmp(argv[i], "16") == 0 ||
                 strcmp(argv[i], "32") == 0 || strcmp(argv[i], "64") == 0)) {
            side = (unsigned)(argv[i][0] - '0') * 10 + (unsigned)(argv[i][1] - '0');
            size_set = 1;
        } else {
            puts("Usage: PIX86 [16|32|64] [/NOMOUSE]");
            return 1;
        }
    }
    if (!editor_init(&editor, side, fill_queue)) {
        puts("PIX86: could not initialize editor.");
        return 1;
    }
    if (!video_init()) {
        puts("PIX86: VGA Mode 13h required; launch from DOS text mode.");
        return 1;
    }
    has_mouse = !disable_mouse && mouse_init();
    video_clear(3);
    render_ui(&editor);
    render_canvas(&editor);
    mouse_poll(&mouse);
    render_status(&editor, mouse.x, mouse.y, has_mouse);
    if (has_mouse) cursor_show(mouse.x, mouse.y);
    previous = mouse;
    for (;;) {
        key = keyboard_poll();
        if (key == 27) break;
        flags = editor_key(&editor, key);
        mouse_poll(&mouse);
        if (mouse.x != previous.x || mouse.y != previous.y ||
            mouse.buttons != previous.buttons) {
            if (has_mouse)
                flags |= editor_mouse(&editor, mouse.x, mouse.y, mouse.buttons);
        }
        if (flags) {
            cursor_hide();
            if (flags & EDIT_UI) render_ui(&editor);
            if (flags & EDIT_CANVAS) render_canvas(&editor);
            else if (flags & EDIT_STROKE) render_stroke(&editor);
            render_status(&editor, mouse.x, mouse.y, has_mouse);
            if (has_mouse) cursor_show(mouse.x, mouse.y);
        }
        previous = mouse;
        delay(10);
    }
    editor_finish(&editor);
    cursor_hide();
    mouse_shutdown();
    video_shutdown();
    puts("PIX86: returned to DOS text mode.");
    return 0;
}
