#include "project.h"

void project_init(Project *project, Editor *editor, P86Image *scratch)
{
    project->editor = editor;
    project->scratch = scratch;
    project->filename[0] = '\0';
    project->report.temporary[0] = project->report.backup[0] = '\0';
}

int project_save(Project *project, const char *path, int overwrite)
{
    int status;
    editor_finish(project->editor);
    editor_image(project->editor, project->scratch);
    status = file_save(path, project->scratch, overwrite, &project->report);
    if (status == FILE_OK || status == FILE_BACKUP_LEFT) {
        file_name(path, project->filename);
        editor_mark_saved(project->editor);
    }
    return status;
}

int project_open(Project *project, const char *path)
{
    int status;
    status = file_load(path, project->scratch);
    if (status == FILE_OK) {
        editor_load(project->editor, project->scratch);
        file_name(path, project->filename);
    }
    return status;
}

int project_new(Project *project, unsigned side)
{
    if (!editor_init(project->editor, side, project->editor->fill_queue)) return 0;
    project->filename[0] = '\0';
    return 1;
}
