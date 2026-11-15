#ifndef PIX86_FILEIO_H
#define PIX86_FILEIO_H
#include <stdio.h>
#include "anim.h"
#define FILE_PATH_MAX 80
#define P86_HEADER_SIZE 24

typedef Animation P86Image;

typedef struct FileReport {
    char temporary[FILE_PATH_MAX];
    char backup[FILE_PATH_MAX];
} FileReport;

enum FileStatus {
    FILE_OK, FILE_BACKUP_LEFT, FILE_BAD_NAME, FILE_EXISTS, FILE_OPEN_ERROR,
    FILE_READ_ERROR, FILE_WRITE_ERROR, FILE_SIGNATURE, FILE_VERSION,
    FILE_DIMENSIONS, FILE_LENGTH, FILE_HEADER, FILE_RENAME_ERROR, FILE_RECOVERY, FILE_MEMORY
};
/* Canonical uppercase DOS 8.3 path, default .P86; output may alias input. */
int file_name(const char *input, char *output);
const char *file_message(int status);
int file_encode(FILE *stream, const P86Image *image);
/* Decode output is scratch: unspecified on failure. Never decode into live art. */
int file_decode(FILE *stream, P86Image *image);
int file_load(const char *path, P86Image *image);
/* Single-writer DOS transaction; report names retained recovery files. */
typedef int (*FileWriter)(FILE *stream,const void *context);
/* Same checked transaction for independent encoders. Paths must have an extension. */
int file_write(const char *path,FileWriter writer,const void *context,
               int overwrite,FileReport *report);
/* Canonical path probe: 1 regular file, 0 absent, -1 inaccessible/non-file. */
int file_exists(const char *path);
int file_save(const char *path, const P86Image *image, int overwrite,
              FileReport *report);
#endif
