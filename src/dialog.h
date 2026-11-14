#ifndef PIX86_DIALOG_H
#define PIX86_DIALOG_H
#include "fileio.h"
#define DIALOG_X 30
#define DIALOG_Y 48
#define DIALOG_WIDTH 260
#define DIALOG_HEIGHT 104
#define DIALOG_TEXT_X 38
#define DIALOG_BUTTON_Y 136
#define DIALOG_BUTTON_WIDTH 60
#define DIALOG_BUTTON_HEIGHT 10
#define DIALOG_ACCEPT_X 80
#define DIALOG_CANCEL_X 180

typedef struct DialogInput {
    char text[FILE_PATH_MAX];
    unsigned length, cursor;
    int replace;
} DialogInput;
void dialog_input(DialogInput *input, const char *initial);
/* 1 accept, -1 cancel, 0 continue; ASCII editing plus extended keys. */
int dialog_edit(DialogInput *input, int key);
void dialog_mouse(int available);
int dialog_filename(const char *title, const char *initial, char *output);
int dialog_confirm(const char *title, const char *question);
int dialog_new_size(unsigned current);
void dialog_message(const char *title, const char *message, const FileReport *report);
#endif
