#ifndef PIX86_UI_H
#define PIX86_UI_H
#include "menu.h"
#include "mouse.h"
/* Live interaction state, independent of document, renderer and file adapter. */
typedef struct UiState { Menu menu; int blocked; } UiState;
int ui_timeline(UiState *ui,int open,unsigned first,unsigned count,const MouseState *mouse,int *command);
void ui_init(UiState *ui);
/* Route overlays first. Consumed events must not reach drawing tools. */
int ui_input(UiState *ui,int key,const MouseState *mouse,int *command);
/* Resume after a modal window; swallow its held dismissal buttons. */
void ui_resume(UiState *ui,const MouseState *mouse);
/* Return true only when the editor may receive this pointer event. */
int ui_pointer(UiState *ui,const MouseState *mouse,int consumed,int was_active);
#endif
