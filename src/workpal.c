#include <string.h>
#include "workpal.h"
#include "layout.h"
static const unsigned char defaults[16]={0,4,8,12,15,21,39,51,46,226,196,199,201,213,112,231};
static unsigned char colours[16]={0,4,8,12,15,21,39,51,46,226,196,199,201,213,112,231};
void workpal_init(void) { memcpy(colours,defaults,sizeof(colours)); }
unsigned char workpal_colour(int slot) { return colours[slot&15]; }
void workpal_remember(unsigned char colour)
{
    int i;
    for(i=0;i<16;++i) if(colours[i]==colour) return;
    memmove(colours+8,colours+9,7); colours[15]=colour;
}
unsigned char workpal_step(unsigned char colour,int direction)
{
    int i;
    for(i=0;i<16;++i) if(colours[i]==colour) return colours[(i+16+direction)&15];
    return colours[direction<0?15:0];
}
int workpal_more(int x,int y)
{ return x>=PALETTE_X && x<PALETTE_X+TOOL_WIDTH && y>=PALETTE_MORE_Y && y<PALETTE_MORE_Y+TOOL_HEIGHT; }
int workpal_full_hit(int x,int y,unsigned char *colour)
{
    if(x<FULLPAL_X || x>=FULLPAL_X+16*FULLPAL_CELL_W || y<FULLPAL_Y || y>=FULLPAL_Y+16*FULLPAL_CELL_H) return 0;
    *colour=(unsigned char)((y-FULLPAL_Y)/FULLPAL_CELL_H*16+(x-FULLPAL_X)/FULLPAL_CELL_W);
    return 1;
}
