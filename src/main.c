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
#include "screen.h"
#include "ui.h"
#include "workpal.h"

/* No large automatic objects or full-screen buffers in the small model. */
static Editor editor;
static unsigned short fill_queue[4096];
static P86Image scratch;
static Project project;
static Screen screen;
static int mouse_available;
static int video_abort(void)
{
    editor_finish(&editor);
    cursor_hide();
    mouse_shutdown();
    video_shutdown();
    puts("PIX86: video memory access failed; restored DOS text mode.");
    return 1;
}

static void refresh_editor(unsigned flags)
{
    MouseState mouse;
    mouse_poll(&mouse);
    screen_filename(&screen,project.filename);
    screen_update(&screen,&editor,&screen.menu,flags,&mouse,mouse_available);
}

static int discard_changes(void)
{
    return !editor_is_dirty(&editor) ||
           dialog_confirm("Unsaved changes", "Discard unsaved artwork?");
}

/* Returns true only for confirmed exit. Dialogs restore their own footprints. */
static int file_command(int key)
{
    char path[FILE_PATH_MAX];
    int status, side;
    editor_finish(&editor);
    refresh_editor(EDIT_STATUS);
    if (key == CMD_EXIT) return discard_changes();
    if (key == CMD_NEW) {
        if (!discard_changes()) return 0;
        side = dialog_new_size(editor.canvas.width);
        if (side) {
            project_new(&project, (unsigned)side);
            refresh_editor(EDIT_UI | EDIT_CANVAS | EDIT_STATUS);
        }
    } else if (key == CMD_OPEN) {
        if (!discard_changes()) return 0;
        if (dialog_filename("Open .P86", project.filename, path)) {
            status = project_open(&project, path);
            if (status == FILE_OK) refresh_editor(EDIT_UI | EDIT_CANVAS | EDIT_STATUS);
            dialog_message(status == FILE_OK ? "File opened" : "Open failed",
                           status == FILE_OK ? project.filename : file_message(status), NULL);
        }
    } else {
        if (key == CMD_SAVE_AS || !project.filename[0]) {
            if (!dialog_filename(key == CMD_SAVE_AS ? "Save As .P86" : "Save .P86", project.filename[0] ?
                                 project.filename : "UNTITLED.P86", path)) return 0;
        } else strcpy(path, project.filename);
        status = project_save(&project, path, 0);
        if (status == FILE_EXISTS) {
            if (!dialog_confirm("Overwrite file?", path)) return 0;
            status = project_save(&project, path, 1);
        }
        refresh_editor(EDIT_STATUS);
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
    int has_mouse, disable_mouse, request_hires, mode_set, key, i, size_set;
    int command, consumed, was_active, modal;
    int font_preview;
    UiState ui;
    unsigned char colour;
    int background;
    side = 32;
    size_set = disable_mouse = request_hires = mode_set = font_preview = 0;
    for (i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "/NOMOUSE") == 0 && !disable_mouse)
            disable_mouse = 1;
        else if (strcmp(argv[i], "/FONT") == 0 && !font_preview)
            font_preview = 1;
        else if ((strcmp(argv[i], "/320") == 0 ||
                  strcmp(argv[i], "/640") == 0 ||
                  strcmp(argv[i], "/HIRES") == 0) && !mode_set) {
            request_hires = strcmp(argv[i], "/320") != 0;
            mode_set = 1;
        }
        else if (!size_set && (strcmp(argv[i], "16") == 0 ||
                 strcmp(argv[i], "32") == 0 || strcmp(argv[i], "64") == 0)) {
            side = (unsigned)(argv[i][0] - '0') * 10 + (unsigned)(argv[i][1] - '0');
            size_set = 1;
        } else {
            puts("Usage: PIX86 [16|32|64] [/320|/640] [/NOMOUSE] [/FONT]");
            return 1;
        }
    }
    if (!editor_init(&editor, side, fill_queue)) {
        puts("PIX86: could not initialize editor.");
        return 1;
    }
    project_init(&project, &editor, &scratch);
    ui_init(&ui);
    if (!video_init(request_hires)) {
        puts("PIX86: VGA Mode 13h required; launch from DOS text mode.");
        return 1;
    }
    layout_set(video_width(), video_height(), video_hires());
    view_set(&editor.view, &editor.canvas, view_max_zoom(&editor.canvas));
    has_mouse = !disable_mouse && mouse_init();
    mouse_available=has_mouse;
    dialog_mouse(has_mouse);
    dialog_editor(&editor);
    mouse_poll(&mouse);
    screen_init(&screen,&editor,&mouse,has_mouse);
    if(video_failed()) return video_abort();
    if(has_mouse) cursor_show(mouse.x,mouse.y);
    if(request_hires && !video_hires())
        dialog_message("Enhanced mode unavailable",
                       "VBE 640x480x256 is unsupported.\nUsing Classic 320x200.",NULL);
    if(font_preview) {
        dialog_font();
        mouse_poll(&mouse);
        ui_resume(&ui,&mouse);
    }
    previous = mouse;
    for (;;) {
        key = keyboard_poll();
        mouse_poll(&mouse);
        was_active = ui.menu.active;
        consumed = ui_input(&ui,key,&mouse,&command);
        flags = 0;
        modal = 0;
        if (consumed) editor_finish(&editor);
        /* Restore an exposed menu before dispatching a modal command. */
        screen_update(&screen,&editor,&ui.menu,0,&mouse,has_mouse);
        if(video_failed()) return video_abort();
        if (command == CMD_EXIT || command == CMD_SAVE || command == CMD_OPEN ||
            command == CMD_SAVE_AS || command == CMD_NEW) {
            if (file_command(command)) break;
            modal = 1;
        } else if(command==CMD_PALETTE || command==CMD_BG) {
            editor_finish(&editor); refresh_editor(EDIT_STATUS);
            background=command==CMD_BG;
            colour=background?editor.background:editor.foreground;
            if(dialog_palette(&colour,&background)) {
                if(background) editor.background=colour;
                else editor.foreground=colour;
                flags|=EDIT_UI|EDIT_STATUS;
            }
            modal=1;
        } else if(command==CMD_COLOUR_PREV || command==CMD_COLOUR_NEXT) {
            editor_finish(&editor);
            editor.foreground=workpal_step(editor.foreground,command==CMD_COLOUR_PREV?-1:1);
            flags|=EDIT_UI|EDIT_STATUS;
        } else if(command==CMD_DISPLAY_MODE) {
            editor_finish(&editor);
            refresh_editor(EDIT_STATUS);
            if(video_hires())
                dialog_message("Display mode",
                    "Enhanced 640x480 is active.\nRestart with PIX86 /320 for Classic.",NULL);
            else
                dialog_message("Display mode",
                    "Classic 320x200 is active.\nRestart with PIX86 /640 for Enhanced.",NULL);
            modal=1;
        } else if (command == CMD_HELP || command == CMD_ABOUT) {
            if (command == CMD_HELP)
                dialog_message("Pix86 controls", "P E F I: tools. U R: undo/redo. G: grid. +/-: zoom. A/B: all colours FG/BG. [ ]: working colours. F2: save F3: open. Ctrl+\\: menu.", NULL);
            else dialog_message("About Pix86", "Original 16-bit DOS pixel editor. Animation is planned.", NULL);
            modal = 1;
        } else if (command > 0) flags |= editor_key(&editor, command);
        if (modal) {
            mouse_poll(&mouse);
            ui_resume(&ui,&mouse);
        }
        if (!ui_pointer(&ui,&mouse,consumed,was_active))
            editor.previous_buttons = mouse.buttons;
        else if (has_mouse &&
                   (mouse.x != previous.x || mouse.y != previous.y || mouse.buttons != previous.buttons))
            flags |= editor_mouse(&editor, mouse.x, mouse.y, mouse.buttons);
        screen_update(&screen,&editor,&ui.menu,flags,&mouse,has_mouse);
        if(video_failed()) return video_abort();
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
