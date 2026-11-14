#ifndef PIX86_PROJECT_H
#define PIX86_PROJECT_H
#include "editor.h"

typedef struct Project {
    Editor *editor;
    P86Image *scratch;
    char filename[FILE_PATH_MAX];
    FileReport report;
} Project;
void project_init(Project *project, Editor *editor, P86Image *scratch);
int project_save(Project *project, const char *path, int overwrite);
int project_open(Project *project, const char *path);
int project_new(Project *project, unsigned side);
#endif
