#include <string.h>
#include "dialog.h"
#include "keyboard.h"

void dialog_input(DialogInput *input, const char *initial)
{
    unsigned i;
    for (i = 0; initial[i] && i < FILE_PATH_MAX - 1; ++i)
        input->text[i] = initial[i];
    input->text[i] = '\0';
    input->length = input->cursor = i;
    input->replace = 1;
}

int dialog_edit(DialogInput *input, int key)
{
    if (key == 27) return -1;
    if (key == 13) return input->length ? 1 : 0;
    if (key == KEY_LEFT || key == KEY_RIGHT || key == KEY_HOME || key == KEY_END) {
        input->replace = 0;
        if (key == KEY_LEFT && input->cursor) --input->cursor;
        if (key == KEY_RIGHT && input->cursor < input->length) ++input->cursor;
        if (key == KEY_HOME) input->cursor = 0;
        if (key == KEY_END) input->cursor = input->length;
    } else if (key == 8 || key == KEY_DELETE) {
        if (input->replace) {
            input->length = input->cursor = 0;
            input->text[0] = '\0';
        } else if (key == 8 && input->cursor) {
            memmove(input->text + input->cursor - 1, input->text + input->cursor,
                    input->length - input->cursor + 1);
            --input->cursor; --input->length;
        } else if (key == KEY_DELETE && input->cursor < input->length) {
            memmove(input->text + input->cursor, input->text + input->cursor + 1,
                    input->length - input->cursor);
            --input->length;
        }
        input->replace = 0;
    } else if (key >= 32 && key <= 126) {
        if (input->replace) input->length = input->cursor = 0;
        input->replace = 0;
        if (input->length < FILE_PATH_MAX - 1) {
            memmove(input->text + input->cursor + 1, input->text + input->cursor,
                    input->length - input->cursor);
            input->text[input->cursor++] = (char)key;
            input->text[++input->length] = '\0';
        }
    }
    return 0;
}
