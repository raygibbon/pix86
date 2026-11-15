#include "anim.h"
int anim_dimensions(unsigned w,unsigned h)
{
    return (w==h && (w==16 || w==32 || w==64)) ||
           (w==16 && (h==24 || h==32)) || (w==32 && h==24);
}
