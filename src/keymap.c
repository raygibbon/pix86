#include <ctype.h>
#include "command.h"
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

int keyboard_command(int key)
{
    if(key>=0 && key<=255) key=tolower((unsigned char)key);
    if(key=='u') return CMD_UNDO;
    if(key=='r') return CMD_REDO;
    if(key=='=') return CMD_ZOOM_IN;
    switch(key) {
    case CMD_NEW: case CMD_OPEN: case CMD_SAVE: case CMD_SAVE_AS:
    case CMD_EXIT: case CMD_UNDO: case CMD_REDO: case CMD_CLEAR:
    case CMD_GRID: case CMD_ZOOM_IN: case CMD_ZOOM_OUT:
    case CMD_PENCIL: case CMD_ERASER: case CMD_FILL: case CMD_PICK:
    case CMD_PALETTE: case CMD_BG: case CMD_COLOUR_PREV: case CMD_COLOUR_NEXT:
    case CMD_FONT: case CMD_HELP: case CMD_ABOUT: return key;
    default: return CMD_NONE;
    }
}
