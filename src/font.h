#ifndef PIX86_FONT_H
#define PIX86_FONT_H
extern int font_cell_width;
extern int font_cell_height;
extern int font_hires_mode;
#define FONT_WIDTH font_cell_width
#define FONT_HEIGHT font_cell_height
void font_set_mode(int hires);
int font_width(const char *text);
#endif
