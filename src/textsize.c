#include "font.h"
int font_width(const char *text)
{
    int width;
    width=0;
    while(text && *text++ && width <= 32760-FONT_WIDTH) width+=FONT_WIDTH;
    return width;
}
