#include "flyout.h"
#include "layout.h"
#include "command.h"
int tool_group(int tool) { return tool<4?0:1; }
int tool_command(int tool)
{
    static const int commands[7]={CMD_PENCIL,CMD_ERASER,CMD_FILL,CMD_PICK,CMD_SELECT,CMD_MOVE,CMD_SELECT_ALL};
    return tool>=0 && tool<=TOOL_ALL?commands[tool]:0;
}
int flyout_count(int group) { return group==0?4:group==1?3:0; }
int flyout_tool(int group,int slot)
{
    static const int selection[3]={4,6,5};
    if(slot<0 || slot>=flyout_count(group)) return -1;
    return group==0?slot:selection[slot];
}
void flyout_bounds(int group,Rect *r)
{
    r->w=flyout_count(group)*FLY_CELL+2; r->h=FLY_CELL+2;
    r->x=TOOL_X-r->w; r->y=TOOL_Y+group*TOOL_STEP;
    if(r->x<0) r->x=0;
    if(r->y+r->h>UI_STATUS_Y) r->y=UI_STATUS_Y-r->h;
}
int flyout_hit(int group,int x,int y)
{
    Rect r;
    if(group<0) return -1;
    flyout_bounds(group,&r);
    if(x<=r.x || x>=r.x+r.w-1 || y<=r.y || y>=r.y+r.h-1) return -1;
    return (x-r.x-1)/FLY_CELL;
}
int nav_slots(void)
{
    int n;
    n=(NAV_RIGHT-NAV_X-2*NAV_ARROW)/NAV_CELL;
    return n>16?16:n<1?1:n;
}
unsigned nav_first(unsigned active,unsigned count)
{
    unsigned n,first;
    n=(unsigned)nav_slots(); first=active/n*n;
    if(count>n && first+n>count) first=count-n;
    return first;
}
int nav_hit(unsigned active,unsigned count,int x,int y)
{
    unsigned first,slot;
    int end;
    if(y<UI_STATUS_Y || y>=UI_HEIGHT || x<NAV_X) return 0;
    first=nav_first(active,count); end=NAV_X+NAV_ARROW;
    if(x<end) return CMD_FRAME_PREV;
    slot=(unsigned)((x-end)/NAV_CELL);
    if(slot<(unsigned)nav_slots() && first+slot<count) return CMD_SELECT_FRAME+(int)(first+slot);
    end+=(int)(count-first<(unsigned)nav_slots()?count-first:(unsigned)nav_slots())*NAV_CELL;
    return x>=end && x<end+NAV_ARROW?CMD_FRAME_NEXT:0;
}
