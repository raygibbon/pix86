#ifndef PIX86_LAYOUT_H
#define PIX86_LAYOUT_H
#include "font.h"
/* Shared graphical layout. View mapping and painters use the same bounds.
   A future timeline can reserve space here without changing document storage. */
#define UI_WIDTH 320
#define UI_HEIGHT 200
#define UI_MENU_HEIGHT (FONT_HEIGHT + 1)
#define UI_STATUS_Y (UI_HEIGHT - FONT_HEIGHT)
#define UI_LEFT_WIDTH 22
#define UI_RIGHT_X 250
#define DRAW_X 24
#define DRAW_Y 14
#define DRAW_WIDTH 222
#define DRAW_HEIGHT 176
#define PALETTE_X 254
#define PALETTE_Y 24
#define PALETTE_CELL_W 4
#define PALETTE_CELL_H 6
#define PALETTE_LABEL_X PALETTE_X
#define PALETTE_TITLE_Y 13
#define FG_Y 126
#define BG_Y 140
#define SWATCH_X 300
#define SWATCH_WIDTH 16
#define SWATCH_HEIGHT 10
#define PALETTE_HINT_Y 165
#define GRID_HINT_Y 178
#define TOOL_X 1
#define TOOL_Y 14
#define TOOL_WIDTH (3 * FONT_WIDTH + 2)
#define TOOL_HEIGHT (FONT_HEIGHT + 2)
#define TOOL_STEP (TOOL_HEIGHT + 1)
#define TOOL_TEXT_X (TOOL_X + 1)
#define TOOL_TEXT_Y 1
#define UI_STATUS_CHARS (UI_WIDTH / FONT_WIDTH)
#endif
