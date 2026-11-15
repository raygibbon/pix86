#include <dos.h>
#include <i86.h>
#include "mouse.h"
#include "video.h"

static int available;

int mouse_init(void)
{
    union REGS r;
    unsigned char __far *handler;
    handler = (unsigned char __far *)_dos_getvect(0x33);
    available = 0;
    /* Some DOS systems leave this vector null or pointing to IRET. */
    if (handler == 0 || *handler == 0xcf)
        return 0;
    r.w.ax = 0;
    int86(0x33, &r, &r);
    if (r.w.ax != 0xffff)
        return 0;
    available = 1;
    /* Mouse X uses a 640-unit virtual space in both display modes. */
    r.w.ax = 7;
    r.w.cx = 0;
    r.w.dx = 639;
    int86(0x33, &r, &r);
    r.w.ax = 8;
    r.w.cx = 0;
    r.w.dx = (unsigned short)(video_height() - 1);
    int86(0x33, &r, &r);
    r.w.ax = 4;
    r.w.cx = 320;
    r.w.dx = (unsigned short)(video_height() / 2);
    int86(0x33, &r, &r);
    return 1;
}

void mouse_poll(MouseState *state)
{
    union REGS r;
    state->x = state->y = 0;
    state->buttons = 0;
    if (!available)
        return;
    r.w.ax = 3;
    int86(0x33, &r, &r);
    state->x = (int)((long)r.w.cx * video_width() / 640);
    state->y = r.w.dx;
    if (state->x >= video_width()) state->x = video_width() - 1;
    if (state->y >= video_height()) state->y = video_height() - 1;
    state->buttons = r.w.bx & (MOUSE_LEFT | MOUSE_RIGHT);
}

void mouse_shutdown(void)
{
    union REGS r;
    if (available) {
        r.w.ax = 0;
        int86(0x33, &r, &r);
        available = 0;
    }
}
