#ifndef PIX86_COMMAND_H
#define PIX86_COMMAND_H
#include "keyboard.h"
/* Stable logical commands. Character values preserve the small original API;
   shortcut aliases are translated only by keyboard_command in keymap.c. */
#define CMD_NONE 0
#define CMD_SELECT_ALL 670
#define CMD_DESELECT 671
#define CMD_SELECT 672
#define CMD_MOVE 673
#define CMD_APPLY 674
#define CMD_NEW 'n'
#define CMD_OPEN KEY_F3
#define CMD_SAVE KEY_F2
#define CMD_SAVE_AS KEY_SAVE_AS
#define CMD_FILL_BG 660
#define CMD_PREVIEW 661
#define CMD_PREVIEW_LINK 662
#define CMD_CHECKER 663
#define CMD_TRANS_BLANK 664
#define CMD_TRANS_ERASE 665
#define CMD_EXPORT 604
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
#define CMD_PALETTE 'a'
#define CMD_BG 'b'
#define CMD_COLOUR_PREV '['
#define CMD_COLOUR_NEXT ']'
#define CMD_FRAME_ADD 620
#define CMD_FRAME_DUP 621
#define CMD_FRAME_DELETE 622
#define CMD_FRAME_PREV 623
#define CMD_FRAME_NEXT 624
#define CMD_PLAY 625
#define CMD_FASTER 626
#define CMD_SLOWER 627
#define CMD_SCROLL_LEFT 628
#define CMD_SCROLL_RIGHT 629
#define CMD_SELECT_FRAME 640
#define CMD_TIMELINE 612
#define CMD_GUIDE8 610
#define CMD_GUIDE16 611
#define CMD_FONT 603
#define CMD_HELP 600
#define CMD_ABOUT 601
int keyboard_command(int key);
#endif
