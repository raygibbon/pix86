#ifndef PIX86_EXPORT_H
#define PIX86_EXPORT_H
#include "fileio.h"
#define EXPORT_CURRENT 0
#define EXPORT_ALL 1
#define EXPORT_SHEET 2
#define SHEET_HORIZONTAL 0
#define SHEET_VERTICAL 1
#define SHEET_GRID 2
typedef struct ExportOptions { int mode,layout; unsigned columns; } ExportOptions;
typedef struct ExportReport {
    unsigned completed,total;
    char path[FILE_PATH_MAX];
    FileReport recovery;
} ExportReport;
int export_name(const char *input,char *output);
int export_target(const char *base,int mode,unsigned frame,char *output);
int export_size(const Animation ANIM_FAR *doc,const ExportOptions *options,
                unsigned *width,unsigned *height);
/* Synchronous immutable snapshot; frame is zero-based. Protected path may be
   NULL; the controller passes its native filename to prevent self-overwrite. */
int export_run(const Animation ANIM_FAR *doc,unsigned frame,const char *path,
               const ExportOptions *options,int overwrite,const char *protected_path,
               ExportReport *report);
#endif
