#include "layout.h"

int layout_screen_width = 320;
int layout_screen_height = 200;
int font_cell_width = 6;
int font_cell_height = 8;
int ui_font_cell_width = 4;
int ui_font_cell_height = 6;
int font_hires_mode;

void font_set_mode(int hires)
{
    font_hires_mode = hires != 0;
    font_cell_width = font_hires_mode ? 8 : 6;
    font_cell_height = font_hires_mode ? 16 : 8;
    ui_font_cell_width = font_hires_mode ? 8 : 4;
    ui_font_cell_height = font_hires_mode ? 16 : 6;
}

void layout_set(int width, int height, int hires)
{
    layout_screen_width = width;
    layout_screen_height = height;
    font_set_mode(hires);
}
