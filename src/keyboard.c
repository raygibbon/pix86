#include <bios.h>
#include "keyboard.h"

int keyboard_poll(void)
{
    unsigned word, shifts;
    /* BIOS retains the scan byte that DOS getch() loses for ASCII controls.
       Standard services work on XT-class BIOSes as well as enhanced keyboards. */
    if (!_bios_keybrd(_KEYBRD_READY)) return -1;
    shifts = _bios_keybrd(_KEYBRD_SHIFTSTATUS);
    word = _bios_keybrd(_KEYBRD_READ);
    return keyboard_decode(word, shifts);
}
