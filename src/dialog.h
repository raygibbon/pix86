#ifndef PIX86_DIALOG_H
#define PIX86_DIALOG_H
#include "fileio.h"
#include "editor.h"
#include "region.h"
#define DIALOG_FILE 0
#define DIALOG_CONFIRM 1
#define DIALOG_NEW 2
#define DIALOG_MESSAGE 3
#define DIALOG_COLUMNS 40
#define DIALOG_FILE_HINT "DOS 8.3 path; default .P86"
#define DIALOG_NEW_CHOICES "1 16x16 2 32x32 3 64x64"
typedef struct DialogLayout {
    Rect box,body,field,accept,cancel;
    int columns,hint_y;
} DialogLayout;
void dialog_layout(DialogLayout *layout,int kind,const char *title,const char *body);
int dialog_line(const char **text,char *line,unsigned columns);
typedef struct DialogInput {
    char text[FILE_PATH_MAX];
    unsigned length,cursor;
    int replace;
} DialogInput;
void dialog_input(DialogInput *input,const char *initial);
int dialog_edit(DialogInput *input,int key);
void dialog_mouse(int available);
void dialog_editor(const Editor *editor);
int dialog_filename(const char *title,const char *initial,char *output);
int dialog_confirm(const char *title,const char *question);
int dialog_new_size(unsigned current);
void dialog_message(const char *title,const char *message,const FileReport *report);
#endif
