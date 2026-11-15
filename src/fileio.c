#ifndef __WATCOMC__
#define _POSIX_C_SOURCE 200809L
#endif
#include <errno.h>
#include <ctype.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#ifdef __WATCOMC__
#include <io.h>
#define host_open _open
#define host_close _close
#define host_fdopen _fdopen
#define host_stat _stat
#define StatInfo struct _stat
#define CREATE_FLAGS (_O_CREAT | _O_EXCL | _O_WRONLY | _O_BINARY)
#define CREATE_MODE (_S_IREAD | _S_IWRITE)
#define PATH_SEP '\\'
#else
#include <unistd.h>
#define host_open open
#define host_close close
#define host_fdopen fdopen
#define host_stat stat
#define StatInfo struct stat
#define CREATE_FLAGS (O_CREAT | O_EXCL | O_WRONLY)
#define CREATE_MODE 0600
#define PATH_SEP '/'
#endif
#include "fileio.h"

static int dimensions(unsigned width, unsigned height)
{
    return width == height && (width == 16 || width == 32 || width == 64);
}

static int device(const char *base)
{
    return strcmp(base, "CON") == 0 || strcmp(base, "PRN") == 0 ||
           strcmp(base, "AUX") == 0 || strcmp(base, "NUL") == 0 ||
           strcmp(base, "CLOCK$") == 0 ||
           ((strncmp(base, "COM", 3) == 0 || strncmp(base, "LPT", 3) == 0) &&
            base[3] >= '1' && base[3] <= '9' && base[4] == '\0');
}

int file_name(const char *input, char *output)
{
    char work[FILE_PATH_MAX], base[9];
    unsigned i, start, length, n, ext, dot, j;
    int last;
    if (input == NULL || output == NULL)
        return 0;
    for (length = 0; input[length] && length < FILE_PATH_MAX - 1; ++length)
        work[length] = (char)toupper((unsigned char)input[length]);
    if (length == 0 || input[length]) return 0;
    work[length] = '\0';
    start = 0;
    if (length > 1 && work[1] == ':') {
        if (work[0] < 'A' || work[0] > 'Z') return 0;
        start = 2;
    }
    if (work[start] == '/' || work[start] == '\\') work[start++] = PATH_SEP;
    if (start == length) return 0;
    for (i = start; i <= length; ++i) {
        if (work[i] != '/' && work[i] != '\\' && work[i] != '\0') continue;
        last = i == length;
        n = i - start;
        if (n == 0) return 0;
        if (!last && ((n == 1 && work[start] == '.') ||
            (n == 2 && work[start] == '.' && work[start + 1] == '.'))) {
            work[i] = PATH_SEP;
            start = i + 1;
            continue;
        }
        dot = n;
        for (j = 0; j < n; ++j) {
            if (work[start + j] == '.') {
                if (dot != n) return 0;
                dot = j;
            } else if (!((work[start + j] >= 'A' && work[start + j] <= 'Z') ||
                         (work[start + j] >= '0' && work[start + j] <= '9') ||
                         strchr("_$~!#%&'()-@^{}", work[start + j]) != NULL))
                return 0;
        }
        ext = dot == n ? 0 : n - dot - 1;
        if (dot == 0 || dot > 8 || ext > 3 || (dot < n && ext == 0)) return 0;
        memcpy(base, work + start, dot);
        base[dot] = '\0';
        if (device(base)) return 0;
        if (last) {
            if (dot == n) {
                if (length + 4 >= FILE_PATH_MAX) return 0;
                strcpy(work + length, ".P86");
            }
        } else {
            work[i] = PATH_SEP;
            start = i + 1;
        }
    }
    strcpy(output, work);
    return 1;
}

static unsigned get16(const unsigned char *p)
{
    return (unsigned)p[0] | ((unsigned)p[1] << 8);
}

static unsigned long get32(const unsigned char *p)
{
    return (unsigned long)get16(p) | ((unsigned long)get16(p + 2) << 16);
}

static void put16(unsigned char *p, unsigned value)
{
    p[0] = (unsigned char)value;
    p[1] = (unsigned char)(value >> 8);
}

int file_encode(FILE *stream, const P86Image *image)
{
    unsigned char header[P86_HEADER_SIZE],timing[2];
    unsigned size,frames,i,payload;
    int v2;
    if(!stream || !image) return FILE_WRITE_ERROR;
    v2=image->frames!=0; frames=v2?image->frames:1;
    if(!(v2?anim_dimensions(image->width,image->height):dimensions(image->width,image->height))) return FILE_DIMENSIONS;
    size=image->width*image->height;
    if(frames>ANIM_FRAMES || frames>ANIM_PIXELS/size) return FILE_LENGTH;
    for(i=0;v2 && i<frames;++i)
        if(image->duration[i]<20 || image->duration[i]>2000) return FILE_HEADER;
    payload=frames*(size+(v2?2:0));
    memset(header,0,sizeof(header)); memcpy(header,"P86\032",4);
    put16(header+4,v2?2:1); put16(header+6,P86_HEADER_SIZE);
    put16(header+8,image->width); put16(header+10,image->height);
    header[12]=image->foreground; header[13]=image->background;
    put16(header+16,payload);
    if(v2) put16(header+20,frames);
    if(fwrite(header,1,sizeof(header),stream)!=sizeof(header)) return FILE_WRITE_ERROR;
    for(i=0;i<frames;++i) {
        if(v2) {
            put16(timing,image->duration[i]);
            if(fwrite(timing,1,2,stream)!=2) return FILE_WRITE_ERROR;
        }
        if(fwrite(image->pixels+i*size,1,size,stream)!=size) return FILE_WRITE_ERROR;
    }
    return ferror(stream)?FILE_WRITE_ERROR:FILE_OK;
}
int file_decode(FILE *stream,P86Image *image)
{
    unsigned char header[P86_HEADER_SIZE],timing[2];
    unsigned width,height,size,frames,version,i;
    if(!stream || !image) return FILE_READ_ERROR;
    if(fread(header,1,sizeof(header),stream)!=sizeof(header))
        return ferror(stream)?FILE_READ_ERROR:FILE_LENGTH;
    if(memcmp(header,"P86\032",4)) return FILE_SIGNATURE;
    version=get16(header+4);
    if(version!=1 && version!=2) return FILE_VERSION;
    if(get16(header+6)!=P86_HEADER_SIZE || get16(header+14) || get16(header+22)) return FILE_HEADER;
    if(version==1 && get32(header+20)) return FILE_HEADER;
    width=get16(header+8); height=get16(header+10);
    if(!(version==1?dimensions(width,height):anim_dimensions(width,height))) return FILE_DIMENSIONS;
    size=width*height; frames=version==1?1:get16(header+20);
    if(!frames || frames>ANIM_FRAMES || frames>ANIM_PIXELS/size) return FILE_LENGTH;
    if(get32(header+16)!=(unsigned long)frames*(size+(version==2?2:0))) return FILE_LENGTH;
    for(i=0;i<frames;++i) {
        image->duration[i]=ANIM_DEFAULT_MS;
        if(version==2) {
            if(fread(timing,1,2,stream)!=2) return ferror(stream)?FILE_READ_ERROR:FILE_LENGTH;
            image->duration[i]=get16(timing);
            if(image->duration[i]<20 || image->duration[i]>2000) return FILE_HEADER;
        }
        if(fread(image->pixels+i*size,1,size,stream)!=size)
            return ferror(stream)?FILE_READ_ERROR:FILE_LENGTH;
    }
    if(fgetc(stream)!=EOF) return FILE_LENGTH;
    if(ferror(stream)) return FILE_READ_ERROR;
    image->width=width; image->height=height;
    image->foreground=header[12]; image->background=header[13];
    /* Zero is the legacy single-image API marker, never an on-disk frame count. */
    image->frames=version==1?0:frames;
    return FILE_OK;
}

int file_load(const char *path, P86Image *image)
{
    char name[FILE_PATH_MAX];
    FILE *stream;
    int status;
    if (!file_name(path, name)) return FILE_BAD_NAME;
    stream = fopen(name, "rb");
    if (stream == NULL) return FILE_OPEN_ERROR;
    status = file_decode(stream, image);
    if (fclose(stream) != 0 && status == FILE_OK) status = FILE_READ_ERROR;
    return status;
}

/* -1 means inaccessible, not evidence that it is safe to replace a file. */
static int exists(const char *path)
{
    StatInfo info;
    if (host_stat(path, &info) == 0) {
#ifdef __WATCOMC__
        if ((info.st_mode & _S_IFMT) == _S_IFREG) return 1;
#else
        if (S_ISREG(info.st_mode)) return 1;
#endif
        errno = EACCES;
        return -1;
    }
    return errno == ENOENT ? 0 : -1;
}

static FILE *create_new(const char *path)
{
    int handle, error;
    FILE *stream;
    handle = host_open(path, CREATE_FLAGS, CREATE_MODE);
    if (handle < 0) return NULL;
    stream = host_fdopen(handle, "wb");
    if (stream == NULL) {
        error = errno;
        host_close(handle);
        remove(path);
        errno = error;
    }
    return stream;
}

static int candidate(const char *target, char *output, unsigned sequence,
                     const char *extension)
{
    unsigned i, prefix;
    prefix = 0;
    for (i = 0; target[i]; ++i)
        if (target[i] == PATH_SEP || target[i] == ':') prefix = i + 1;
    if (prefix + 12 >= FILE_PATH_MAX) return 0;
    memcpy(output, target, prefix);
    sprintf(output + prefix, "P86%05u.%s", sequence, extension);
    return strcmp(output, target) != 0;
}

static void discard(char *path)
{
    if (path[0] && remove(path) == 0) path[0] = '\0';
}

int file_save(const char *path, const P86Image *image, int overwrite,
              FileReport *report)
{
    char name[FILE_PATH_MAX];
    FILE *stream;
    int present, status, probe;
    unsigned sequence;
    if (report == NULL) return FILE_WRITE_ERROR;
    report->temporary[0] = report->backup[0] = '\0';
    if (!file_name(path, name)) return FILE_BAD_NAME;
    if (image == NULL || !(image->frames?anim_dimensions(image->width, image->height):dimensions(image->width,image->height))) return FILE_DIMENSIONS;
    present = exists(name);
    if (present < 0) return FILE_OPEN_ERROR;
    if (present && !overwrite) return FILE_EXISTS;
    stream = NULL;
    for (sequence = 0; sequence < 100; ++sequence) {
        if (!candidate(name, report->temporary, sequence, "TMP")) continue;
        stream = create_new(report->temporary);
        if (stream != NULL) break;
        if (errno != EEXIST) {
            report->temporary[0] = '\0';
            return FILE_OPEN_ERROR;
        }
    }
    if (stream == NULL) {
        report->temporary[0] = '\0';
        return FILE_OPEN_ERROR;
    }
    status = file_encode(stream, image);
    if (fflush(stream) != 0) status = FILE_WRITE_ERROR;
    if (fclose(stream) != 0) status = FILE_WRITE_ERROR;
    if (status != FILE_OK) {
        discard(report->temporary);
        return status;
    }
    /* Recheck after writing; never let rename silently replace an unapproved target. */
    present = exists(name);
    if (present < 0 || (present && !overwrite)) {
        discard(report->temporary);
        return present < 0 ? FILE_OPEN_ERROR : FILE_EXISTS;
    }
    if (present) {
        for (sequence = 0; sequence < 100; ++sequence) {
            if (!candidate(name, report->backup, sequence, "BAK")) continue;
            probe = exists(report->backup);
            if (probe == 0) break;
            if (probe < 0) break;
        }
        if (sequence == 100 || probe != 0 || rename(name, report->backup) != 0) {
            report->backup[0] = '\0';
            discard(report->temporary);
            return FILE_RENAME_ERROR;
        }
    }
    if (rename(report->temporary, name) != 0) {
        if (present && rename(report->backup, name) != 0)
            return FILE_RECOVERY;
        report->backup[0] = '\0';
        discard(report->temporary);
        return FILE_RENAME_ERROR;
    }
    report->temporary[0] = '\0';
    if (present) discard(report->backup);
    return report->backup[0] ? FILE_BACKUP_LEFT : FILE_OK;
}

const char *file_message(int status)
{
    static const char *messages[] = {
        "File saved", "Saved; backup kept", "Invalid DOS 8.3 path", "File already exists",
        "Cannot open/create file", "Cannot read project file", "Cannot write: disk full or I/O error",
        "Not a Pix86 file", "Unsupported P86 version", "Unsupported canvas size",
        "Wrong/truncated file length", "Invalid P86 header", "Cannot replace file",
        "Restore failed: keep recovery files"
    };
    if (status < FILE_OK || status > FILE_RECOVERY) return "File error";
    return messages[status];
}
