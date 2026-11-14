#include <dos.h>
#include <i86.h>
#include "mouse.h"

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
    /* Reset leaves driver cursor hidden. We draw our own cursor. Mode 13h
       mouse X coordinates use a 640-wide virtual space (two units/pixel). */
    r.w.ax = 7;
    r.w.cx = 0;
    r.w.dx = 639;
    int86(0x33, &r, &r);
    r.w.ax = 8;
    r.w.cx = 0;
    r.w.dx = 199;
    int86(0x33, &r, &r);
    r.w.ax = 4;
    r.w.cx = 320;
    r.w.dx = 100;
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
    state->x = r.w.cx / 2;
    state->y = r.w.dx;
    if (state->x > 319) state->x = 319;
    if (state->y > 199) state->y = 199;
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
