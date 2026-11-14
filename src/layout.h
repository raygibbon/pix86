#ifndef PIX86_LAYOUT_H
#define PIX86_LAYOUT_H
#include "font.h"
extern int layout_screen_width;
extern int layout_screen_height;
void layout_set(int width, int height, int hires);
#define UI_WIDTH layout_screen_width
#define UI_HEIGHT layout_screen_height
#define UI_MENU_HEIGHT (FONT_HEIGHT + 1)
#define UI_STATUS_Y (UI_HEIGHT - FONT_HEIGHT)
#define UI_LEFT_WIDTH (FONT_HEIGHT == 8 ? 42 : 84)
#define UI_RIGHT_X UI_WIDTH
#define DRAW_X (UI_LEFT_WIDTH + 2)
#define DRAW_Y (UI_MENU_HEIGHT + 5)
#define DRAW_WIDTH (UI_WIDTH - DRAW_X - 2)
#define DRAW_HEIGHT (UI_STATUS_Y - DRAW_Y - 2)
#define TOOL_X (FONT_HEIGHT == 8 ? 2 : 4)
#define TOOL_TITLE_Y DRAW_Y
#define TOOL_Y (TOOL_TITLE_Y + FONT_HEIGHT + 3)
#define TOOL_WIDTH (6 * FONT_WIDTH + 2)
#define TOOL_HEIGHT (FONT_HEIGHT + 2)
#define TOOL_STEP (TOOL_HEIGHT + 1)
#define TOOL_TEXT_X (TOOL_X + 1)
#define TOOL_TEXT_Y 1
#define PALETTE_X TOOL_X
#define PALETTE_TITLE_Y (TOOL_Y + 4 * TOOL_STEP + 7)
#define PALETTE_Y (BG_Y + FONT_HEIGHT + 6)
#define PALETTE_COLS 2
#define PALETTE_ROWS 8
#define PALETTE_CELL_W (FONT_HEIGHT == 8 ? 19 : 27)
#define PALETTE_CELL_H FONT_HEIGHT
#define PALETTE_LABEL_X TOOL_X
#define FG_Y (PALETTE_TITLE_Y + FONT_HEIGHT + 3)
#define BG_Y (FG_Y + FONT_HEIGHT + 4)
#define SWATCH_WIDTH (2 * FONT_WIDTH)
#define SWATCH_HEIGHT (FONT_HEIGHT + 2)
#define SWATCH_X (UI_LEFT_WIDTH - SWATCH_WIDTH - 2)
#define PALETTE_MORE_Y (PALETTE_Y + PALETTE_ROWS * PALETTE_CELL_H + 2)
#define FULLPAL_CELL_W (FONT_WIDTH + 4)
#define FULLPAL_CELL_H (FONT_HEIGHT - 1)
#define FULLPAL_BOX_W (16 * FULLPAL_CELL_W + 28)
#define FULLPAL_BOX_H (16 * FULLPAL_CELL_H + (FONT_HEIGHT == 8 ? 52 : 72))
#define FULLPAL_BOX_X ((UI_WIDTH - FULLPAL_BOX_W) / 2)
#define FULLPAL_BOX_Y ((UI_HEIGHT - FULLPAL_BOX_H) / 2)
#define FULLPAL_X (FULLPAL_BOX_X + 14)
#define FULLPAL_Y (FULLPAL_BOX_Y + 24)
#define FULLPAL_VALUE_Y (FULLPAL_Y + 16 * FULLPAL_CELL_H + 4)
#define FULLPAL_HINT_Y (FULLPAL_BOX_Y + FULLPAL_BOX_H - FONT_HEIGHT - 4)
#define UI_STATUS_CHARS (UI_WIDTH / FONT_WIDTH)
#define UI_STATUS_MAX_CHARS (640 / 6)
#endif
