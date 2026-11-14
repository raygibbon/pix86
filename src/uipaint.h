#ifndef PIX86_UIPAINT_H
#define PIX86_UIPAINT_H
#include "region.h"
/* Small shared painters, with no input or document ownership. */
void ui_border(const Rect *rect,unsigned char colour);
/* Buttons have six text cells and four pixels of horizontal inset. */
void ui_button(const Rect *rect,const char *label,int selected);
#endif
