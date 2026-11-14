#include <conio.h>
#include "keyboard.h"

int keyboard_poll(void)
{
    int key;
    if (!kbhit())
        return -1;
    key = getch();
    if (key == 0 || key == 0xe0) {
        (void)getch();
        return -1;
    }
    return key;
}
