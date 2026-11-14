# Pix86 M2.5 interface

Pix86 uses an original VGA interface for a 320x200, 256-colour screen. Animation
is planned; this milestone remains a single-image pixel editor. No widget
framework, external font, graphics library or reference editor code is used.

## Functional reference and independent design

Reviewed the local TDE 5.1v source distribution at
`/home/rgibbon/Downloads/tde51vs/` on 2026-11-14. Its `readme_s.txt` identifies
menus, prompts, configurable bindings and alternative menu arrangements.
A limited review of `pull.c` and the user-facing prompts in `prompts.c`, plus
`update.txt`, established these functional conventions: arrows move selection,
Enter selects, Escape abandons an operation, unavailable commands are visibly
disabled, shortcuts accompany commands, and status information stays distinct
from the working document. This is a behavioural review, not a code-reuse or
translation exercise. No source, structures, layouts, strings, fonts or assets
were copied or adapted. The distribution's public-domain description was not
used as permission to import anything.

Reference URL: <http://adoxa.altervista.org/tde/dl.php?f=source>.
LibreSprite is a workflow aspiration only; no implementation/assets were used.
GrafX2 and Animator/Animator Pro source/assets were not consulted or imported.

## Menus

The top 16 scanlines contain File, Edit, View, Frame, Options and Help, leaving
the existing canvas, toolbar and palette coordinates unchanged. An asterisk at
the far right marks unsaved content, including persisted FG/BG selections.
Canvas dimensions, coordinates, zoom, grid and history appear below the workspace.

- F10 opens/closes menus; Alt+F/E/V/R/O/H opens the named menu. The R in Frame
  avoids colliding with File; each mnemonic is underlined.
- Left/Right wraps between headings. Up/Down wraps between rows. Enter executes
  the selected enabled command. Escape closes the menu without exiting Pix86.
- File: New, Open, Save, Save As, Exit. Edit: Undo, Redo, Clear.
- View: Zoom in/out and Grid. Options also exposes Grid for convenience.
- Frame shows three disabled future operations. Enter or a click cannot execute
  them; this milestone implements no animation.
- Help offers Controls and About. Shortcut labels match the direct drawing and
  file bindings. Drawing keys are consumed while a menu is open.
- A fresh left press on a heading opens it; a fresh left press on a row executes
  it. A left press outside closes the menu and is consumed. Held buttons cannot
  start a canvas stroke on dismissal; release before drawing again.

Opening a menu and moving within it repaints only the overlay/status; switching
headings or dismissing restores the covered workspace.

Menus use bounded static descriptions and portable state in `menu.c`, with VGA
painting in `menudraw.c`. `keymap.c` centralizes portable Alt-key interpretation;
`keyboard.c` acquires DOS extended-key pairs. Main dispatches commands; neither
menu module owns or alters the drawing engine.

## Dialogs

All dialogs occupy a centred 308x116 box. A title, concise instructions, outlined
buttons and bright selected control distinguish focus from inactive controls.
Keyboard-only operation is supported even without an INT 33h mouse driver.

Filename prompts accept DOS paths, not a file browser. The initial text is
selected: typing replaces it. Left/Right/Home/End leave selection mode and move
the insertion cursor; Backspace/Delete remove characters around that cursor.
The 79-character bounded field scrolls horizontally to keep the cursor visible.
Click inside the field to position the cursor. Tab/Shift+Tab cycle field, OK and
Cancel; Enter accepts the field/OK or cancels from Cancel. Escape cancels.
Validation/default .P86 extension is provided by the independent file adapter.

New offers 16, 32 or 64 pixels square. Arrows cycle sizes; 1/2/3 immediately choose
a size for compatibility. Click a size then Create, or press Enter. Tab selects
Create/Cancel. Escape cancels without touching the current document.

Overwrite and unsaved-content confirmations initially select Cancel. Y or a
click on Yes approves; N/Escape or Cancel abandons the operation. Tab, arrows and
Enter also operate these buttons. New, Open and Exit must pass the unsaved check;
an Open failure still retains the document, filename, history and saved baseline.
Save uses the existing name, Save As always prompts, and all replacements require
confirmation. Messages/errors accept a key or a click on OK, and retain enough
space to show both 79-character recovery paths.

The application hides its software cursor before every overlay repaint. Modal
polling consumes only fresh mouse presses, including between consecutive dialogs.
After dismissal the whole workspace is repainted and held buttons are swallowed
until release. File parsing, replacement and saved-state comparisons remain
independent of menu and dialog rendering. No full-screen backing buffer is used.
