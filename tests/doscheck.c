/* Interactive DOS integration harness, separate from the shipped application. */
#include <stdio.h>
#include <string.h>
#include <process.h>
#include <dos.h>
#include <i86.h>

static unsigned char iret_opcode = 0xcf;

int main(int argc, char **argv)
{
    union REGS r;
    void (__interrupt __far *old_mouse)(void);
    unsigned mode, page, cursor, shape, actual_mode, actual_page;
    int result, no_mouse, iret_mouse, ok;
    FILE *report;
    no_mouse = argc > 1 && strcmp(argv[1], "null") == 0;
    iret_mouse = argc > 1 && strcmp(argv[1], "iret") == 0;
    old_mouse = _dos_getvect(0x33);
    r.w.ax = no_mouse ? 1 : 3;
    int86(0x10, &r, &r);
    r.h.ah = 5;
    r.h.al = no_mouse ? 0 : 1;
    int86(0x10, &r, &r);
    r.h.ah = 0x0f;
    int86(0x10, &r, &r);
    mode = r.h.al;
    page = r.h.bh;
    r.h.ah = 1;
    r.w.cx = 0x0607;
    int86(0x10, &r, &r);
    r.h.ah = 2;
    r.h.bh = page;
    r.w.dx = 0x050a;
    int86(0x10, &r, &r);
    r.h.ah = 3;
    r.h.bh = page;
    int86(0x10, &r, &r);
    shape = r.w.cx;
    if (no_mouse) _dos_setvect(0x33, NULL);
    if (iret_mouse)
        _dos_setvect(0x33, (void (__interrupt __far *)(void))
                     (unsigned char __far *)&iret_opcode);
    result = _spawnl(_P_WAIT, "PIX86.EXE", "PIX86.EXE", NULL);
    _dos_setvect(0x33, old_mouse);
    r.h.ah = 0x0f;
    int86(0x10, &r, &r);
    actual_mode = r.h.al;
    actual_page = r.h.bh;
    ok = result == 0 && actual_mode == mode && actual_page == page;
    r.h.ah = 3;
    r.h.bh = page;
    int86(0x10, &r, &r);
    cursor = r.w.dx;
    /* PIX86 prints one line after restoration; the cursor advances a row. */
    ok = ok && cursor == (no_mouse ? 0x0700 : 0x0600) && r.w.cx == shape;
    report = fopen(no_mouse ? "NULL.TXT" :
                   (iret_mouse ? "IRET.TXT" : "MOUSE.TXT"), "w");
    if (report == NULL)
        return 1;
    fprintf(report, "%s: exit=%d mode=%u page=%u cursor=%04x shape=%04x\n",
            ok ? "PASS" : "FAIL", result, actual_mode, actual_page,
            cursor, r.w.cx);
    fclose(report);
    r.w.ax = 3;
    int86(0x10, &r, &r);
    puts(ok ? "PASS: DOS mode/page/cursor restored." : "FAIL: see report.");
    return ok ? 0 : 1;
}
