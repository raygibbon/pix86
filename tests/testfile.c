#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#ifdef __WATCOMC__
#include <direct.h>
#define make_dir(path) _mkdir(path)
#else
#include <sys/stat.h>
#define make_dir(path) mkdir(path, 0700)
#endif
#include "project.h"
#include "dialog.h"
#include "keyboard.h"

static Editor editor;
static unsigned short queue[4096];
static P86Image image;
static Project project;
static FileReport report;
static unsigned char raw[4121];
#ifdef IO_FAULTS
static int write_failure, flush_failure, close_failure, remove_failure;
static unsigned rename_calls, rename_failures;
size_t fault_write(const void *data, size_t size, size_t count, FILE *stream)
{
    return fwrite(data, size, write_failure && count ? count - 1 : count, stream);
}
int fault_flush(FILE *stream)
{
    int result;
    result = fflush(stream);
    return flush_failure ? EOF : result;
}
int fault_close(FILE *stream)
{
    int result;
    result = fclose(stream);
    return close_failure ? EOF : result;
}
int fault_rename(const char *from, const char *to)
{
    ++rename_calls;
    if (rename_calls <= 3 && (rename_failures & (1U << rename_calls))) {
        errno = EACCES;
        return -1;
    }
    return rename(from, to);
}
int fault_remove(const char *path)
{
    if (remove_failure) { errno = EACCES; return -1; }
    return remove(path);
}
#endif

static void pattern(unsigned side)
{
    unsigned i;
    image.width = image.height = side;
    image.foreground = 255;
    image.background = 0;
    for (i = 0; i < side * side; ++i) image.pixels[i] = (unsigned char)i;
}

static void check_pattern(unsigned side)
{
    unsigned i;
    assert(image.width == side && image.height == side);
    assert(image.foreground == 255 && image.background == 0);
    for (i = 0; i < side * side; ++i) assert(image.pixels[i] == (unsigned char)i);
}

static void write_bytes(const char *path, const unsigned char *bytes, unsigned size)
{
    FILE *stream;
    stream = fopen(path, "wb");
    assert(stream != NULL);
    assert(fwrite(bytes, 1, size, stream) == size);
    assert(fclose(stream) == 0);
}

static void test_names(void)
{
    char name[FILE_PATH_MAX], long_name[90];
    assert(file_name("player", name) && strcmp(name, "PLAYER.P86") == 0);
    assert(file_name("12345678.p86", name) && strcmp(name, "12345678.P86") == 0);
    assert(file_name(name, name));
    assert(file_name("c:\\art\\player", name));
#ifdef __WATCOMC__
    assert(strcmp(name, "C:\\ART\\PLAYER.P86") == 0);
#else
    assert(strcmp(name, "C:/ART/PLAYER.P86") == 0);
#endif
    assert(file_name("..\\.\\PLAYER", name));
    assert(file_name("C:PLAYER", name));
    assert(file_name("/PLAYER", name));
    assert(!file_name("", name));
    assert(!file_name(".", name));
    assert(!file_name("..", name));
    assert(!file_name("ART/", name));
    assert(!file_name("ART//PLAYER", name));
    assert(!file_name("ART/../", name));
    assert(!file_name("LONGFNAME.P86", name));
    assert(!file_name("PLAYER.LONG", name));
    assert(!file_name("PLAYER.", name));
    assert(!file_name("A B.P86", name));
    assert(!file_name("A*.P86", name));
    assert(!file_name("A:B:C", name));
    assert(!file_name("1:PLAYER", name));
    assert(!file_name("CON.P86", name));
    assert(!file_name("NUL", name));
    assert(!file_name("LPT1.X", name));
    assert(!file_name("COM9", name));
    assert(!file_name("CLOCK$", name));
    memset(long_name, 'A', sizeof(long_name));
    long_name[sizeof(long_name) - 1] = '\0';
    assert(!file_name(long_name, name));
}

static void test_roundtrip(void)
{
    unsigned side;
    FILE *stream;
    (void)make_dir("SUB");
    for (side = 16; side <= 64; side *= 2) {
        pattern(side);
        remove("SUB/PLAYER.P86");
        assert(file_save("SUB/PLAYER", &image, 0, &report) == FILE_OK);
        memset(&image, 0, sizeof(image));
        assert(file_load("SUB/PLAYER.P86", &image) == FILE_OK);
        check_pattern(side);
        assert(file_save("SUB/PLAYER", &image, 0, &report) == FILE_EXISTS);
        assert(file_save("SUB/PLAYER", &image, 1, &report) == FILE_OK);
        assert(report.temporary[0] == '\0' && report.backup[0] == '\0');
        stream = fopen("SUB/PLAYER.P86", "rb");
        assert(stream != NULL);
        assert(fread(raw, 1, P86_HEADER_SIZE, stream) == P86_HEADER_SIZE);
        assert(memcmp(raw, "P86\032\001\000\030\000", 8) == 0);
        assert(raw[8] == side && raw[9] == 0 && raw[10] == side && raw[11] == 0);
        assert(raw[12] == 255 && raw[13] == 0);
        assert(raw[16] == 0 && raw[17] == (side * side) / 256);
        assert(raw[18] == 0 && raw[19] == 0);
        assert(fclose(stream) == 0);
    }
    assert(file_save("NOEXIST/PLAYER", &image, 0, &report) == FILE_OPEN_ERROR);
    (void)make_dir("DIR.P86");
    assert(file_save("DIR.P86", &image, 1, &report) == FILE_OPEN_ERROR);
    assert(file_load("MISSING", &image) == FILE_OPEN_ERROR);
    assert(file_load("NUL", &image) == FILE_BAD_NAME);
    image.width = 65535U;
    assert(file_save("INVALID", &image, 0, &report) == FILE_DIMENSIONS);
}

static int decode(unsigned length)
{
    FILE *stream;
    int status;
    stream = tmpfile();
    assert(stream != NULL);
    assert(fwrite(raw, 1, length, stream) == length);
    rewind(stream);
    status = file_decode(stream, &image);
    assert(fclose(stream) == 0);
    return status;
}

static void test_malformed(void)
{
    FILE *stream;
    unsigned length, i;
    unsigned char original;
    static const unsigned offsets[] = {0,4,6,8,9,10,14,16,18,20};
    static const int errors[] = {FILE_SIGNATURE, FILE_VERSION, FILE_HEADER,
        FILE_DIMENSIONS, FILE_DIMENSIONS, FILE_DIMENSIONS, FILE_HEADER,
        FILE_LENGTH, FILE_LENGTH, FILE_HEADER};
    pattern(16);
    stream = tmpfile();
    assert(stream != NULL);
    assert(file_encode(stream, &image) == FILE_OK);
    rewind(stream);
    length = (unsigned)fread(raw, 1, sizeof(raw), stream);
    assert(fclose(stream) == 0);
    assert(length == 280);
    for (i = 0; i < length; ++i) assert(decode(i) == FILE_LENGTH);
    for (i = 0; i < sizeof(offsets) / sizeof(offsets[0]); ++i) {
        original = raw[offsets[i]];
        raw[offsets[i]] ^= 1;
        assert(decode(length) == errors[i]);
        raw[offsets[i]] = original;
    }
    raw[length] = 0;
    assert(decode(length + 1) == FILE_LENGTH);
    assert(decode(length) == FILE_OK);
    check_pattern(16);
    /* Truncated or unsupported input must not touch live art or its history. */
    editor_dispose(&editor);
    assert(editor_init(&editor, 32, queue));
    project_init(&project, &editor, &image);
    canvas_clear(&editor.canvas, 117);
    history_commit(&editor.history, &editor.canvas);
    write_bytes("BAD.P86", raw, length - 1);
    assert(project_open(&project, "BAD") == FILE_LENGTH);
    assert(editor.canvas.width == 32 && editor.history.position == 1);
    assert(editor_is_dirty(&editor) && project.filename[0] == '\0');
    for (i = 0; i < 1024; ++i) assert(editor.pixels[i] == 117);
    write_bytes("GOOD.P86", raw, length);
    assert(project_open(&project, "GOOD") == FILE_OK);
    assert(editor.canvas.width == 16 && editor.canvas.height == 16);
    assert(editor.foreground == 255 && editor.background == 0);
    for (i = 0; i < 256; ++i) assert(editor.pixels[i] == (unsigned char)i);
    assert(!editor_is_dirty(&editor));
    assert(!history_undo(&editor.history, &editor.canvas));
    assert(!history_redo(&editor.history, &editor.canvas));
    editor_key(&editor, 'c');
    assert(editor_is_dirty(&editor));
    editor_key(&editor, 'u');
    assert(!editor_is_dirty(&editor));
    assert(!project_new(&project, 8));
    assert(editor.canvas.width == 16);
    assert(project_new(&project, 64));
    assert(editor.canvas.width == 64 && !editor_is_dirty(&editor));
    assert(project.filename[0] == '\0');
}

static void test_dirty(void)
{
    unsigned i;
    editor_dispose(&editor);
    assert(editor_init(&editor, 64, queue));
    project_init(&project, &editor, &image);
    assert(!editor_is_dirty(&editor));
    editor_key(&editor, '-'); editor_key(&editor, 'g'); editor_key(&editor, 'e');
    assert(!editor_is_dirty(&editor));
    canvas_clear(&editor.canvas, 22);
    history_commit(&editor.history, &editor.canvas);
    assert(editor_is_dirty(&editor));
    remove("SAVED.P86");
    assert(project_save(&project, "SAVED", 0) == FILE_OK);
    assert(!editor_is_dirty(&editor) && strcmp(project.filename, "SAVED.P86") == 0);
    editor_key(&editor, 'u'); assert(editor_is_dirty(&editor));
    editor_key(&editor, 'r'); assert(!editor_is_dirty(&editor));
    for (i = 0; i < 8; ++i) {
        canvas_clear(&editor.canvas, (unsigned char)i);
        history_commit(&editor.history, &editor.canvas);
    }
    canvas_clear(&editor.canvas, 22);
    history_commit(&editor.history, &editor.canvas);
    assert(!editor_is_dirty(&editor)); /* Saved state outlived history eviction. */
    editor.foreground = 0; assert(editor_is_dirty(&editor));
    editor.foreground = 15; assert(!editor_is_dirty(&editor));
    editor.background = 255; assert(editor_is_dirty(&editor));
    editor.background = 0; assert(!editor_is_dirty(&editor));
    canvas_set(&editor.canvas, 0, 0, 255);
    assert(project_save(&project, "SAVED", 0) == FILE_EXISTS);
    assert(editor_is_dirty(&editor));
    assert(file_load("SAVED", &image) == FILE_OK && image.pixels[0] == 22);
    assert(project_save(&project, "NOEXIST/NEW", 0) == FILE_OPEN_ERROR);
    assert(editor_is_dirty(&editor) && strcmp(project.filename, "SAVED.P86") == 0);
    assert(project_save(&project, "SAVED", 1) == FILE_OK);
    assert(!editor_is_dirty(&editor));
    assert(file_load("SAVED", &image) == FILE_OK && image.pixels[0] == 255);
}

static void test_collisions(void)
{
    FILE *stream;
    pattern(16);
    remove("COLLIDE.P86");
    write_bytes("P8600000.TMP", (const unsigned char *)"sentinel", 8);
    write_bytes("P8600000.BAK", (const unsigned char *)"sentinel", 8);
    assert(file_save("COLLIDE", &image, 0, &report) == FILE_OK);
    assert(file_save("COLLIDE", &image, 1, &report) == FILE_OK);
    stream = fopen("P8600000.TMP", "rb"); assert(stream != NULL);
    assert(fread(raw, 1, 8, stream) == 8 && memcmp(raw, "sentinel", 8) == 0);
    fclose(stream);
    stream = fopen("P8600000.BAK", "rb"); assert(stream != NULL);
    assert(fread(raw, 1, 8, stream) == 8 && memcmp(raw, "sentinel", 8) == 0);
    fclose(stream);
    remove("P8600000.TMP"); remove("P8600000.BAK");
}

static void test_dialog(void)
{
    DialogInput input;
    unsigned i;
    dialog_input(&input, "PLAYER.P86");
    assert(dialog_edit(&input, KEY_F2) == 0 && input.replace);
    assert(dialog_edit(&input, 27) == -1);
    assert(dialog_edit(&input, 'A') == 0 && strcmp(input.text, "A") == 0);
    dialog_edit(&input, 'B'); dialog_edit(&input, 8);
    assert(strcmp(input.text, "A") == 0 && dialog_edit(&input, 13) == 1);
    dialog_input(&input, "PLAYER.P86");
    dialog_edit(&input, 8);
    assert(input.length == 0 && dialog_edit(&input, 13) == 0);
    for (i = 0; i < 100; ++i) dialog_edit(&input, 'X');
    assert(input.length == FILE_PATH_MAX - 1 && input.text[input.length] == '\0');
}

#ifdef IO_FAULTS
static void test_failures(void)
{
    unsigned mode;
    int status;
    editor_dispose(&editor);
    assert(editor_init(&editor, 16, queue));
    project_init(&project, &editor, &image);
    canvas_clear(&editor.canvas, 11);
    remove("FAIL.P86");
    assert(project_save(&project, "FAIL", 0) == FILE_OK);
    for (mode = 0; mode < 6; ++mode) {
        canvas_clear(&editor.canvas, 22);
        write_failure = mode == 0;
        flush_failure = mode == 1;
        close_failure = mode == 2;
        rename_calls = 0;
        rename_failures = mode == 3 ? 2U : (mode == 4 ? 4U : 12U);
        status = project_save(&project, "FAIL", 1);
        assert(status == (mode < 3 ? FILE_WRITE_ERROR :
               (mode == 5 ? FILE_RECOVERY : FILE_RENAME_ERROR)));
        assert(editor_is_dirty(&editor));
        write_failure = flush_failure = close_failure = 0;
        rename_failures = 0;
        if (mode == 5) {
            assert(project.report.backup[0] && project.report.temporary[0]);
            assert(file_load(project.report.backup, &image) == FILE_OK);
            assert(image.pixels[0] == 11);
            assert(file_load(project.report.temporary, &image) == FILE_OK);
            assert(image.pixels[0] == 22);
            assert(rename(project.report.backup, "FAIL.P86") == 0);
            remove(project.report.temporary);
        }
        assert(file_load("FAIL", &image) == FILE_OK && image.pixels[0] == 11);
    }
    close_failure = 1;
    assert(project_open(&project, "FAIL") == FILE_READ_ERROR);
    close_failure = 0;
    assert(editor.canvas.width == 16 && editor.pixels[0] == 22);
    assert(editor_is_dirty(&editor));
    remove_failure = 1;
    assert(project_save(&project, "FAIL", 1) == FILE_BACKUP_LEFT);
    assert(!editor_is_dirty(&editor) && project.report.backup[0]);
    remove_failure = 0;
    assert(file_load(project.report.backup, &image) == FILE_OK && image.pixels[0] == 11);
    remove(project.report.backup);
    assert(file_load("FAIL", &image) == FILE_OK && image.pixels[0] == 22);
    remove("FAIL.P86");
}
#endif

int main(void)
{
    test_names(); test_roundtrip(); test_malformed(); test_dirty();
    test_collisions(); test_dialog();
#ifdef IO_FAULTS
    test_failures();
#endif
    remove("SUB/PLAYER.P86"); remove("BAD.P86"); remove("GOOD.P86");
    remove("SAVED.P86"); remove("COLLIDE.P86");
    puts("PASS: P86 roundtrips, validation, replacement, dirty state and dialogs.");
    return 0;
}
