#include "layout.h"
int layout_screen_width=320,layout_screen_height=200;
void layout_set(int width,int height)
{
    layout_screen_width=width==640 && height==480?640:320;
    layout_screen_height=width==640 && height==480?480:200;
}
