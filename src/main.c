#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dos.h>
#include "canvas.h"
#include "video.h"
#include "mouse.h"
#include "keyboard.h"
#include "cursor.h"

#define CANVAS_SIDE 128

static void workspace(const Canvas *canvas)
{
    unsigned x, y;
    unsigned char colour;
    static const unsigned char swatches[16] = {
        0, 4, 8, 15, 21, 46, 51, 76, 91, 106, 141, 166, 181, 196, 216, 231
    };
    video_clear(3);
    video_rect(0, 0, 320, 16, 5);
    video_text(8, 4, "PIX86", 15);
    video_text(208, 4, "M1 ESC: EXIT", 15);
    video_rect(0, 16, 32, 176, 5);
    for (y = 0; y < 4; ++y) {
        video_rect(5, 24 + (int)y * 30, 22, 22, 1);
        video_rect(6, 25 + (int)y * 30, 20, 20, 8);
    }
    video_text(8, 32, "1", 15);
    video_text(8, 62, "2", 15);
    video_text(8, 92, "3", 15);
    video_text(8, 122, "4", 15);
    video_text(8, 176, "M1", 12);
    video_rect(110, 30, 132, 132, 1);
    for (y = 0; y < canvas->height; ++y)
        for (x = 0; x < canvas->width; ++x) {
            if (canvas_get(canvas, x, y, &colour))
                video_pixel(112 + (int)x, 32 + (int)y, colour);
        }
    for (x = 0; x < 16; ++x) {
        video_rect(40 + (int)x * 17, 169, 15, 16, 1);
        video_rect(41 + (int)x * 17, 170, 13, 14, swatches[x]);
    }
}

static void status(const MouseState *mouse, int has_mouse)
{
    char text[41];
    video_rect(0, 192, 320, 8, 5);
    if (has_mouse) {
        sprintf(text, "X:%03d Y:%03d L:%d R:%d  FRAME 1",
                mouse->x, mouse->y,
                (mouse->buttons & MOUSE_LEFT) != 0,
                (mouse->buttons & MOUSE_RIGHT) != 0);
        video_text(0, 192, text, 15);
    } else {
        video_text(0, 192, "NO MOUSE DRIVER - ESC TO EXIT", 15);
    }
}

int main(int argc, char **argv)
{
    Canvas canvas;
    MouseState mouse, previous;
    unsigned char *pixels;
    int has_mouse;
    int disable_mouse;
    disable_mouse = argc == 2 && strcmp(argv[1], "/NOMOUSE") == 0;
    if (argc > 1 && !disable_mouse) {
        puts("Usage: PIX86 [/NOMOUSE]");
        return 1;
    }
    pixels = (unsigned char *)malloc(CANVAS_SIDE * CANVAS_SIDE);
    if (pixels == NULL || !canvas_init(&canvas, CANVAS_SIDE, CANVAS_SIDE,
                                       pixels, CANVAS_SIDE * CANVAS_SIDE)) {
        free(pixels);
        puts("PIX86: not enough conventional memory for the canvas.");
        return 1;
    }
    canvas_clear(&canvas, 11);
    if (!video_init()) {
        free(pixels);
        puts("PIX86: VGA Mode 13h required; launch from DOS text mode.");
        return 1;
    }
    has_mouse = !disable_mouse && mouse_init();
    workspace(&canvas);
    mouse_poll(&mouse);
    status(&mouse, has_mouse);
    if (has_mouse)
        cursor_show(mouse.x, mouse.y);
    previous = mouse;
    while (keyboard_poll() != 27) {
        mouse_poll(&mouse);
        if (mouse.x != previous.x || mouse.y != previous.y ||
            mouse.buttons != previous.buttons) {
            cursor_hide();
            status(&mouse, has_mouse);
            if (has_mouse)
                cursor_show(mouse.x, mouse.y);
            previous = mouse;
        }
        delay(10);
    }
    cursor_hide();
    mouse_shutdown();
    video_shutdown();
    free(pixels);
    puts("PIX86: returned to DOS text mode.");
    return 0;
}
