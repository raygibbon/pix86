#include "font.h"
static int measure(const char *text,int advance)
{
    int width;
    width=0;
    while(text && *text++ && width <= 32760-advance) width+=advance;
    return width;
}

int font_width(const char *text) { return measure(text,FONT_WIDTH); }
int font_ui_width(const char *text) { return measure(text,UI_FONT_WIDTH); }
