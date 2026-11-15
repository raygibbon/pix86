#ifndef PIX86_LAYOUT_H
#define PIX86_LAYOUT_H
#include "font.h"
/* Palette on the left, original tool icons on the right. All canvas/view
   geometry follows the active display; stored artwork dimensions never change. */
#define UI_WIDTH layout_screen_width
extern int layout_screen_width,layout_screen_height;
void layout_set(int width,int height);
#define UI_HEIGHT layout_screen_height
#define UI_MENU_HEIGHT (FONT_HEIGHT + 1)
#define UI_STATUS_Y (UI_HEIGHT - FONT_HEIGHT)
#define UI_LEFT_WIDTH 42
#define UI_RIGHT_X (UI_WIDTH - (UI_WIDTH==640?44:24))
#define DRAW_X 44
#define DRAW_Y 14
#define DRAW_WIDTH (UI_RIGHT_X-DRAW_X-2)
#define DRAW_HEIGHT (UI_STATUS_Y-DRAW_Y-2)
#define TIMELINE_HEIGHT 36
#define TIMELINE_Y (UI_STATUS_Y - TIMELINE_HEIGHT)
#define TIMELINE_CELL 24
#define TIMELINE_SLOTS ((unsigned)((DRAW_WIDTH-36)/TIMELINE_CELL))
#define TOOL_X (UI_RIGHT_X+2)
#define TOOL_TITLE_Y 14
#define TOOL_Y 25
#define TOOL_WIDTH (UI_WIDTH==640?40:20)
#define TOOL_HEIGHT (UI_WIDTH==640?32:22)
#define TOOL_STEP (TOOL_HEIGHT + 1)
#define TOOL_TEXT_X (TOOL_X + 1)
#define TOOL_TEXT_Y 1
#define PALETTE_X 2
#define PALETTE_Y (UI_WIDTH==640?66:57)
#define PALETTE_COLS 2
#define PALETTE_ROWS 8
#define PALETTE_CELL_W 19
#define PALETTE_CELL_H (UI_WIDTH==640?16:8)
#define PALETTE_LABEL_X 2
#define PALETTE_TITLE_Y 14
#define FG_Y 27
#define BG_Y 39
#define SWATCH_X 28
#define SWATCH_WIDTH 12
#define SWATCH_HEIGHT 10
#define PALETTE_MORE_Y (PALETTE_Y+8*PALETTE_CELL_H+3)
#define FULLPAL_X 80
#define FULLPAL_Y 42
#define FULLPAL_CELL_W 10
#define FULLPAL_CELL_H 7
#define EXPORT_OPTION_ROWS 6
#define EXPORT_ROW_HEIGHT FONT_HEIGHT
#define UI_STATUS_CHARS (640 / FONT_WIDTH)
#endif
