#ifndef PIX86_HISTORY_H
#define PIX86_HISTORY_H
#include "canvas.h"
#include "anim.h"
#define HISTORY_STATES 5
#define HISTORY_PIXELS 4096

typedef struct History {
    Animation ANIM_FAR *states[HISTORY_STATES];
    Animation ANIM_FAR *document;
    unsigned active, selected[HISTORY_STATES];
    unsigned width, height, size, count, position;
} History;
int history_create(History *history,Animation ANIM_FAR *document);
void history_destroy(History *history);
/* One base state + four actions. Commit only at action boundaries. */
int history_init(History *history, const Canvas *canvas);
int history_commit(History *history, const Canvas *canvas);
int history_undo(History *history, Canvas *canvas);
int history_redo(History *history, Canvas *canvas);
#endif
