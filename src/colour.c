#include "colour.h"
void colour_rgb(unsigned char index,unsigned char *rgb)
{
    unsigned r,g,b,n;
    if(index<16) r=g=b=index*4U;
    else { n=index-16; r=n%6*12; g=n/6%6*12; b=n/36%6*12; }
    rgb[0]=(unsigned char)((r*255U+31U)/63U);
    rgb[1]=(unsigned char)((g*255U+31U)/63U);
    rgb[2]=(unsigned char)((b*255U+31U)/63U);
}
