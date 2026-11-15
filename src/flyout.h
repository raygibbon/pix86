#ifndef PIX86_FLYOUT_H
#define PIX86_FLYOUT_H
#include "region.h"
#define TOOL_GROUPS 2
#define TOOL_ALL 6
typedef struct Flyout { int group,hover; } Flyout;
int tool_group(int tool);
int tool_command(int tool);
int flyout_count(int group);
int flyout_tool(int group,int slot);
void flyout_bounds(int group,Rect *rect);
int flyout_hit(int group,int x,int y);
unsigned nav_first(unsigned active,unsigned count);
int nav_slots(void);
int nav_hit(unsigned active,unsigned count,int x,int y);
#endif
