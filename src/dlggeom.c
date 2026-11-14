#include <string.h>
#include "dialog.h"
#include "layout.h"

int dialog_line(const char **text, char *line, unsigned columns)
{
    unsigned i;
    if (!*text || !**text || !columns)
        return 0;
    if (columns > DIALOG_COLUMNS)
        columns = DIALOG_COLUMNS;
    i = 0;
    while (**text && **text != '\n' && i < columns)
        line[i++] = *(*text)++;
    if (**text == '\n')
        ++*text;
    line[i] = '\0';
    return 1;
}

void dialog_layout(DialogLayout *layout, int kind, const char *title,
                   const char *body)
{
    const char *p;
    char part[DIALOG_COLUMNS + 1];
    int columns, n, lines, width, height, y;
    columns = kind == DIALOG_FILE ?
        (UI_FONT_WIDTH == 4 ? 32 : 40) :
        (UI_FONT_WIDTH == 4 ? 22 : 30);
    n = font_ui_width(title) / UI_FONT_WIDTH;
    if (n > columns)
        columns = n;
    p = body;
    while (dialog_line(&p, part, DIALOG_COLUMNS)) {
        n = (int)strlen(part);
        if (n > columns)
            columns = n;
    }
    if (columns > DIALOG_COLUMNS)
        columns = DIALOG_COLUMNS;
    lines = 0;
    p = body;
    while (lines < 16 && dialog_line(&p, part, (unsigned)columns))
        ++lines;
    if (!lines)
        lines = 1;
    width = columns * UI_FONT_WIDTH + 8;
    height = lines * UI_FONT_HEIGHT + 3 * UI_FONT_HEIGHT + 22;
    if (kind == DIALOG_FILE)
        height += UI_FONT_HEIGHT + 6;
    layout->columns = columns;
    layout->box.x = (UI_WIDTH - width) / 2;
    layout->box.y = (UI_HEIGHT - height) / 2;
    layout->box.w = width;
    layout->box.h = height;
    layout->body.x = layout->box.x + 4;
    layout->body.y = layout->box.y + UI_FONT_HEIGHT + 8;
    layout->body.w = width - 8;
    layout->body.h = lines * UI_FONT_HEIGHT;
    y = layout->body.y + layout->body.h + 4;
    layout->field.x = layout->body.x;
    layout->field.y = y;
    layout->field.w = layout->body.w;
    layout->field.h = kind == DIALOG_FILE ? UI_FONT_HEIGHT + 2 : 0;
    if (kind == DIALOG_FILE)
        y += layout->field.h + 4;
    layout->hint_y = y;
    layout->accept.w = layout->cancel.w = font_ui_width("Cancel") + 8;
    layout->accept.h = layout->cancel.h = UI_FONT_HEIGHT + 2;
    layout->accept.y = layout->cancel.y = y + UI_FONT_HEIGHT + 4;
    layout->accept.x = (UI_WIDTH - 2 * layout->accept.w -
                        2 * UI_FONT_WIDTH) / 2;
    layout->cancel.x = layout->accept.x + layout->accept.w +
                       2 * UI_FONT_WIDTH;
}
