#include "keyboard.h"
/* Portable key interpretation; DOS scan-pair acquisition stays in keyboard.c. */
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
