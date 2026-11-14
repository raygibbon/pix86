#ifndef PIX86_COMMAND_H
#define PIX86_COMMAND_H
#include "keyboard.h"
/* Stable logical commands. Character values preserve the small original API;
   shortcut aliases are translated only by keyboard_command in keymap.c. */
#define CMD_NONE 0
#define CMD_NEW 'n'
#define CMD_OPEN KEY_F3
#define CMD_SAVE KEY_F2
#define CMD_SAVE_AS KEY_SAVE_AS
#define CMD_EXIT 27
#define CMD_UNDO 26
#define CMD_REDO 25
#define CMD_CLEAR 'c'
#define CMD_GRID 'g'
#define CMD_ZOOM_IN '+'
#define CMD_ZOOM_OUT '-'
#define CMD_PENCIL 'p'
#define CMD_ERASER 'e'
#define CMD_FILL 'f'
#define CMD_PICK 'i'
#define CMD_HELP 600
#define CMD_ABOUT 601
int keyboard_command(int key);
#endif
