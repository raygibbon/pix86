#include "keyboard.h"
/* Portable key interpretation; DOS BIOS-word acquisition stays in keyboard.c. */
int keyboard_menu(int key)
{
    switch (key) {
    case KEY_ALT_F: return 0;
    case KEY_ALT_E: return 1;
    case KEY_ALT_V: return 2;
    case KEY_ALT_R: return 3;
    case KEY_ALT_O: return 4;
    case KEY_ALT_H: return 5;
    default: return -1;
    }
}

int keyboard_decode(unsigned word, unsigned shifts)
{
    unsigned ascii, scan;
    ascii = word & 255U;
    scan = (word >> 8) & 255U;
    /* Conventional FS character survives modifier release while queued.
       US physical-key fallback handles BIOS/layouts returning zero or slash.
       Logical slash also supports layouts placing it on another scan code.
       AltGr (Ctrl+Alt) must still be able to type DOS path separators. */
    if (ascii == 28 || ((shifts & 4) && !(shifts & 8) &&
        (ascii == '\\' || (scan == 0x2b && (ascii == 0 || ascii == '|')))))
        return KEY_MENU;
    if (ascii == 0 || ascii == 0xe0) return 256 + (int)scan;
    return (int)ascii;
}
