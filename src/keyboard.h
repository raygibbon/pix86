#ifndef PIX86_KEYBOARD_H
#define PIX86_KEYBOARD_H
#define KEY_MENU 602
#define KEY_F2 (256 + 60)
#define KEY_F3 (256 + 61)
#define KEY_SAVE_AS (256 + 85)
#define KEY_F10 (256 + 68)
#define KEY_LEFT (256 + 75)
#define KEY_RIGHT (256 + 77)
#define KEY_UP (256 + 72)
#define KEY_DOWN (256 + 80)
#define KEY_HOME (256 + 71)
#define KEY_END (256 + 79)
#define KEY_DELETE (256 + 83)
#define KEY_SHIFT_TAB (256 + 15)
#define KEY_ALT_F (256 + 33)
#define KEY_ALT_E (256 + 18)
#define KEY_ALT_V (256 + 47)
#define KEY_ALT_R (256 + 19)
#define KEY_ALT_O (256 + 24)
#define KEY_ALT_H (256 + 35)
/* Return File/Edit/View/Frame/Options/Help index or -1. */
int keyboard_menu(int key);
/* Decode BIOS AH scan/AL character plus BIOS shift flags (Ctrl=4, Alt=8). */
int keyboard_decode(unsigned word, unsigned shifts);
/* Nonblocking ASCII or 256+BIOS scan code; -1 means no key. */
int keyboard_poll(void);
#endif
