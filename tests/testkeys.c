/* Emulator-only: insert synthetic BIOS words, then use the real keyboard driver.
   /LOG instead records actual BIOS input for keyboard-layout acceptance checks. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <bios.h>
#include <dos.h>
#include <i86.h>
#include "keyboard.h"
static void insert(unsigned word)
{
    union REGS r;
    r.h.ah=5; r.w.cx=word;
    int86(0x16,&r,&r);
    assert(r.h.al==0);
}
int main(int argc, char **argv)
{
    unsigned word, shifts, i;
    FILE *log;
    static const unsigned words[] = {0x2b1c,0x4b00,0x4400,0x2100,0x5500,0x1c0d,0x1970};
    static const int keys[] = {KEY_MENU,KEY_LEFT,KEY_F10,KEY_ALT_F,KEY_SAVE_AS,13,'p'};
    if (argc>1 && !strcmp(argv[1],"/LOG")) {
        log=fopen("KEYS.TXT","w"); assert(log!=NULL);
        do {
            word=_bios_keybrd(_KEYBRD_READ);
            shifts=_bios_keybrd(_KEYBRD_SHIFTSTATUS);
            fprintf(log,"%04x flags=%02x decoded=%d\n",word,shifts,keyboard_decode(word,shifts));
            fflush(log);
        } while ((word&255)!=27);
        fclose(log); return 0;
    }
    while (_bios_keybrd(_KEYBRD_READY)) (void)_bios_keybrd(_KEYBRD_READ);
    assert(keyboard_poll()==-1);
    for(i=0;i<sizeof(words)/sizeof(words[0]);++i) {
        insert(words[i]); assert(keyboard_poll()==keys[i]); assert(keyboard_poll()==-1);
    }
    puts("PASS: BIOS scan/character acquisition, Ctrl-menu and extended keys.");
    return 0;
}
