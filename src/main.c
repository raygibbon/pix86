#include <stdio.h>
#include <string.h>
#include <dos.h>
#include "editor.h"
#include "render.h"
#include "video.h"
#include "mouse.h"
#include "keyboard.h"
#include "cursor.h"
#include "project.h"
#include "dialog.h"
#include "menu.h"

/* No large automatic objects or full-screen buffers in the small model. */
static Editor editor;
static unsigned short fill_queue[4096];
static P86Image scratch;
static Project project;

static int discard_changes(void)
{
    return !editor_is_dirty(&editor) ||
           dialog_confirm("Unsaved changes", "Discard unsaved artwork?");
}

/* Returns true only for confirmed exit. All modal paths repaint afterwards. */
static int file_command(int key)
{
    char path[FILE_PATH_MAX];
    int status, side;
    editor_finish(&editor);
    if (key == 27) return discard_changes();
    if (key == 'n' || key == 'N') {
        if (!discard_changes()) return 0;
        side = dialog_new_size(editor.canvas.width);
        if (side) project_new(&project, (unsigned)side);
    } else if (key == KEY_F3) {
        if (!discard_changes()) return 0;
        if (dialog_filename("Open .P86", project.filename, path)) {
            status = project_open(&project, path);
            dialog_message(status == FILE_OK ? "File opened" : "Open failed",
                           status == FILE_OK ? project.filename : file_message(status), NULL);
        }
    } else {
        if (key == KEY_SAVE_AS || !project.filename[0]) {
            if (!dialog_filename("Save .P86", project.filename[0] ?
                                 project.filename : "UNTITLED.P86", path)) return 0;
        } else strcpy(path, project.filename);
        status = project_save(&project, path, 0);
        if (status == FILE_EXISTS) {
            if (!dialog_confirm("Overwrite file?", path)) return 0;
            status = project_save(&project, path, 1);
        }
        if (status == FILE_OK)
            dialog_message("File saved", project.filename, NULL);
        else
            dialog_message(status == FILE_BACKUP_LEFT ? "Saved; backup kept" : "Save failed",
                           file_message(status), &project.report);
    }
    return 0;
}

int main(int argc, char **argv)
{
    MouseState mouse, previous;
    unsigned side, flags;
    int has_mouse, disable_mouse, key, i, size_set, blocked;
    int command, consumed, was_active, old_heading, modal;
    Menu menu;
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
    project_init(&project, &editor, &scratch);
    blocked = 0;
    menu_init(&menu);
    if (!video_init()) {
        puts("PIX86: VGA Mode 13h required; launch from DOS text mode.");
        return 1;
    }
    has_mouse = !disable_mouse && mouse_init();
    dialog_mouse(has_mouse);
    video_clear(3);
    render_ui(&editor);
    render_canvas(&editor);
    mouse_poll(&mouse);
    render_status(&editor, mouse.x, mouse.y, has_mouse);
    menu_draw(&menu);
    if (editor_is_dirty(&editor)) video_text(304, 4, "*", 15);
    if (has_mouse) cursor_show(mouse.x, mouse.y);
    previous = mouse;
    for (;;) {
        key = keyboard_poll();
        mouse_poll(&mouse);
        was_active = menu.active;
        old_heading = menu.heading;
        consumed = menu_key(&menu, key, &command);
        if (!consumed || key < 0) consumed = menu_mouse(&menu, mouse.x, mouse.y, mouse.buttons, &command);
        else menu.buttons = mouse.buttons;
        flags = 0;
        modal = 0;
        if (consumed) {
            editor_finish(&editor);
            blocked = 1;
            if (key >= 0 || menu.active != was_active || mouse.buttons != previous.buttons) {
                flags = EDIT_STATUS;
                if (!menu.active || (was_active && old_heading != menu.heading))
                    flags |= EDIT_UI | EDIT_CANVAS;
            }
        }
        if (!consumed) command = key;
        if (command == 27 || command == KEY_F2 || command == KEY_F3 ||
            command == KEY_SAVE_AS || command == 'n' || command == 'N') {
            cursor_hide();
            if (file_command(command)) break;
            modal = 1;
        } else if (command == CMD_HELP || command == CMD_ABOUT) {
            cursor_hide();
            if (command == CMD_HELP)
                dialog_message("Pix86 controls", "P E F I: tools. U R: undo/redo. G: grid. +/-: zoom. F2: save F3: open. F10: menu.", NULL);
            else dialog_message("About Pix86", "Original 16-bit DOS pixel editor. Animation is planned.", NULL);
            modal = 1;
        } else if (command > 0) flags |= editor_key(&editor, command);
        if (modal) {
            mouse_poll(&mouse);
            menu.buttons = mouse.buttons;
            blocked = 1;
            flags = EDIT_UI | EDIT_CANVAS | EDIT_STATUS;
        }
        if (blocked || menu.active || was_active) {
            editor.previous_buttons = mouse.buttons;
            if (!mouse.buttons && !menu.active) blocked = 0;
        } else if (!consumed && has_mouse &&
                   (mouse.x != previous.x || mouse.y != previous.y || mouse.buttons != previous.buttons))
            flags |= editor_mouse(&editor, mouse.x, mouse.y, mouse.buttons);
        if (flags) {
            cursor_hide();
            if (modal || (was_active && (!menu.active || old_heading != menu.heading)))
                video_clear(3);
            if (flags & EDIT_UI) render_ui(&editor);
            if (flags & EDIT_CANVAS) render_canvas(&editor);
            else if (flags & EDIT_STROKE) render_stroke(&editor);
            render_status(&editor, mouse.x, mouse.y, has_mouse);
            menu_draw(&menu);
            if (editor_is_dirty(&editor)) video_text(304, 4, "*", 15);
        }
        if (has_mouse) {
            cursor_hide();
            cursor_show(mouse.x, mouse.y);
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
