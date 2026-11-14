#ifndef PIX86_HISTORY_H
#define PIX86_HISTORY_H
#include "canvas.h"
#define HISTORY_STATES 5
#define HISTORY_PIXELS 4096

typedef struct History {
    unsigned char states[HISTORY_STATES][HISTORY_PIXELS];
    unsigned width, height, size, count, position;
} History;
/* One base state + four actions. Commit only at action boundaries. */
int history_init(History *history, const Canvas *canvas);
int history_commit(History *history, const Canvas *canvas);
int history_undo(History *history, Canvas *canvas);
int history_redo(History *history, Canvas *canvas);
#endif
