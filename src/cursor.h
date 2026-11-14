#ifndef PIX86_CURSOR_H
#define PIX86_CURSOR_H
/* Protect only the dirty rectangle; unchanged positions perform no VGA writes. */
void cursor_protect(int x, int y, int width, int height);
void cursor_show(int x, int y);
void cursor_hide(void);
#endif
