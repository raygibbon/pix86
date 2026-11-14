#ifndef PIX86_LAYOUT_H
#define PIX86_LAYOUT_H
#include "font.h"
/* Sidebar only; all remaining width belongs to drawing. Future timeline
   reservations can change these view bounds without changing stored artwork. */
#define UI_WIDTH 320
#define UI_HEIGHT 200
#define UI_MENU_HEIGHT (FONT_HEIGHT + 1)
#define UI_STATUS_Y (UI_HEIGHT - FONT_HEIGHT)
#define UI_LEFT_WIDTH 42
#define UI_RIGHT_X UI_WIDTH
#define DRAW_X 44
#define DRAW_Y 14
#define DRAW_WIDTH 274
#define DRAW_HEIGHT 176
#define TOOL_X 2
#define TOOL_TITLE_Y 14
#define TOOL_Y 25
#define TOOL_WIDTH (6 * FONT_WIDTH + 2)
#define TOOL_HEIGHT (FONT_HEIGHT + 2)
#define TOOL_STEP (TOOL_HEIGHT + 1)
#define TOOL_TEXT_X (TOOL_X + 1)
#define TOOL_TEXT_Y 1
#define PALETTE_X 2
#define PALETTE_Y 113
#define PALETTE_COLS 2
#define PALETTE_ROWS 8
#define PALETTE_CELL_W 19
#define PALETTE_CELL_H 8
#define PALETTE_LABEL_X 2
#define PALETTE_TITLE_Y 76
#define FG_Y 87
#define BG_Y 99
#define SWATCH_X 28
#define SWATCH_WIDTH 12
#define SWATCH_HEIGHT 10
#define PALETTE_MORE_Y 180
#define FULLPAL_X 80
#define FULLPAL_Y 42
#define FULLPAL_CELL_W 10
#define FULLPAL_CELL_H 7
#define UI_STATUS_CHARS (UI_WIDTH / FONT_WIDTH)
#endif
