#ifndef PIX86_FONT_H
#define PIX86_FONT_H
extern int font_cell_width;
extern int font_cell_height;
extern int font_hires_mode;
extern int ui_font_cell_width;
extern int ui_font_cell_height;
#define FONT_WIDTH font_cell_width
#define FONT_HEIGHT font_cell_height
#define UI_FONT_WIDTH ui_font_cell_width
#define UI_FONT_HEIGHT ui_font_cell_height
extern const unsigned char font_ui_glyphs[95][6];
void font_set_mode(int hires);
int font_width(const char *text);
int font_ui_width(const char *text);
#endif
