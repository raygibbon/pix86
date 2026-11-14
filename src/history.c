#include <stddef.h>
#include <string.h>
#include "history.h"

static int matches(const History *history, const Canvas *canvas)
{
    return history != NULL && canvas != NULL && canvas->pixels != NULL &&
           history->count != 0 && history->width == canvas->width &&
           history->height == canvas->height;
}

int history_init(History *history, const Canvas *canvas)
{
    unsigned long size;
    if (history == NULL)
        return 0;
    history->count = history->position = 0;
    if (canvas == NULL || canvas->pixels == NULL)
        return 0;
    size = (unsigned long)canvas->width * canvas->height;
    if (size == 0 || size > HISTORY_PIXELS)
        return 0;
    history->width = canvas->width;
    history->height = canvas->height;
    history->size = (unsigned)size;
    memcpy(history->states[0], canvas->pixels, history->size);
    history->count = 1;
    return 1;
}

int history_commit(History *history, const Canvas *canvas)
{
    if (!matches(history, canvas) ||
        memcmp(history->states[history->position], canvas->pixels,
               history->size) == 0)
        return 0;
    /* An actual edit discards redo; a no-op leaves it available. */
    history->count = history->position + 1;
    if (history->count == HISTORY_STATES) {
        memmove(history->states[0], history->states[1],
                (HISTORY_STATES - 1) * HISTORY_PIXELS);
        --history->count;
    }
    history->position = history->count++;
    memcpy(history->states[history->position], canvas->pixels, history->size);
    return 1;
}

int history_undo(History *history, Canvas *canvas)
{
    if (!matches(history, canvas) || history->position == 0)
        return 0;
    --history->position;
    memcpy(canvas->pixels, history->states[history->position], history->size);
    return 1;
}

int history_redo(History *history, Canvas *canvas)
{
    if (!matches(history, canvas) || history->position + 1 >= history->count)
        return 0;
    ++history->position;
    memcpy(canvas->pixels, history->states[history->position], history->size);
    return 1;
}
