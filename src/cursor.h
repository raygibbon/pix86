#ifndef PIX86_CURSOR_H
#define PIX86_CURSOR_H
/* Hide before changing ANY pixels, then show after drawing finishes. */
void cursor_show(int x, int y);
void cursor_hide(void);
#endif
