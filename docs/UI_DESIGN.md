# Pix86 compact DOS interface

Pix86 stays in VGA Mode 13h (320x200, 256 colours) with a single-image document.
This is an interface redesign; canvas pixels, history, file format and save
replacement guarantees are unchanged. Animation remains planned.

## Reference review and original implementation

Reviewed the local TDE 5.1v distribution at
`/home/rgibbon/Downloads/tde51vs/` on 2026-11-14. The earlier review used
`readme_s.txt`, limited menu/prompt behaviour review, and `update.txt`. This
redesign additionally reviewed `config/tde.cfg`, `config/menu.cfg` and the help
screen in `config/tde.hlp`: compact character-cell menus, dense typography,
light menu/dialog surfaces against a dark work area, contrasting selected rows,
muted unavailable commands, thin framing, visible shortcuts, and a separate
single-line status area. The supplied distribution contains no reference
screenshots or complete runnable TDE executable; its tdv.exe is a loader stub.

These are visual/functional observations, not implementation templates. Pix86's
coordinates, font, colours, module interfaces and bitmap rows were designed
independently. No TDE code, structures, strings, layouts, fonts or binary glyphs
were copied, translated or adapted. Its public-domain description did not
supply permission for imports. Reference URL:
<http://adoxa.altervista.org/tde/dl.php?f=source>.
LibreSprite remains a workflow aspiration. GrafX2 and Animator/Animator Pro
source/assets were not consulted or imported.

## Typography and colour

`font.c` contains 95 original hand-drawn printable ASCII glyphs, each eight
rows of five ink bits: 760 bytes of bitmap data. Every cell advances six pixels,
leaving a blank spacing column. Lowercase has distinct shapes and descenders;
there is no antialiasing, scaling, ROM-font lookup or conversion from another
font. Unsupported bytes use the original question-mark glyph. The renderer
clips transparent ink at all four edges using long coordinate intermediates;
it never changes pixels outside the screen or outside a glyph's ink. `font.h`
defines the shared 6x8 metrics. Rendering tests exercise the actual bitmap path.

`theme.h` names UI indices from the existing fixed palette: deep blue work area,
cyan menu/status bar, light grey dropdowns/dialogs, dark ink, green selection and
muted disabled text. The theme deliberately does not reprogram DAC entries: all
256 artwork indices retain their RGB values and existing `.P86` files render
identically. These original choices reflect conventional DOS colour
relationships, without reproducing a reference palette or screen.

## Workspace

The menu occupies y=0..9. The drawing region is x=24..245, y=14..189:
222x176 pixels, versus the previous 176x168 (about 32% larger). A narrow strip at
left holds four 20x12 text controls, with two-pixel gaps. Render and hit testing
share geometry in `view.h`; the editor only receives a tool index.

The right palette occupies x=254..317, y=24..119: all 256 indices in a 16x16
array of 4x6 cells. Left/right click chooses FG/BG. Compact labels and swatches
beneath it identify both selections. The palette no longer consumes most of
the screen height. Canvas dimensions remain 16, 32 or 64 pixels square; integer
zoom maxima are 11, 5 and 2. Grid lines remain a view overlay.

The single eight-pixel status line at y=192 shows modified marker, dimensions,
X/Y, tool, FG/BG indices, zoom, grid and undo/redo counts. Without a mouse it
retains document information and shows `No mouse`. Menus temporarily replace it
with navigation guidance. View changes do not mark the project modified.

## Menus and keyboard

The single menu row reads File, Edit, View, Frame, Options, Help, with a compact
`Ctrl+\` reminder at right. Ctrl+\ opens File from the editor and closes any
open menu on the next press. F10 and Alt+F/E/V/R/O/H remain alternatives. The R
in Frame avoids colliding with File; headings underline their mnemonic.
Left/Right wraps headings, Up/Down wraps rows, Enter executes and Escape closes.
Drawing shortcuts are consumed while a menu is open and retain their bindings
when closed. Frame operations remain disabled placeholders.

Dropdowns use a one-pixel border and ten-pixel rows, left-aligned command names
and right-aligned shortcut labels. Disabled commands stay visible in muted ink
and cannot execute. Fresh mouse presses activate headings/items. An outside
left click closes the menu and is consumed; release before drawing again.
Dropdowns disappear before commands open dialogs. Opening/moving within a menu
repaints only the overlay/status; switching headings
or closing restores the covered workspace. No screen-sized backing buffer or
widget framework is used.

`keyboard.c` polls BIOS INT 16h through Watcom's `_bios_keybrd`, retaining AH's
scan code and AL's character. `keymap.c` interprets this word separately from
DOS acquisition. Ctrl+\ conventionally produces character 1Ch (FS); recognizing
that queued word works even after Ctrl is released. When a BIOS/layout returns
zero, backslash or pipe instead, the decoder also recognizes Ctrl plus US scan
2Bh, or Ctrl plus a logical backslash on another scan code. Plain backslash
and Ctrl+Alt/AltGr backslash remain filename characters. Extended keys preserve
the existing `256 + BIOS scan` representation. No interrupt handler is installed.

A BIOS word does not carry per-event modifier flags. The fallback uses the
current BIOS Ctrl/Alt flags and is best-effort when a nonstandard driver omits
FS and Ctrl has already been released. Layouts remapping both scan and control
character may require F10/Alt+F. The application does not claim universal layout
support. Standard XT-compatible BIOS keyboard services are used; additional
keys outside Pix86's documented bindings are not promised.

Factual contracts: [RBIL INT 16h get keystroke](https://delorie.com/djgpp/doc/rbinter/id/63/17.html)
(AH scan, AL character and legacy extended-key limitations), local Watcom
`h/bios.h`, and [DOSBox-X regional settings](https://dosbox-x.com/wiki/Guide:Regional-settings-in-DOSBox%E2%80%90X)
(layout selection with `KEYB`, including UK). These are specifications, not
source-code dependencies. Actual tested layouts/results are in TESTING.md.

![Original compact Save As dialog, DOSBox-X at 2x](DIALOG.PNG)

## Dialogs and input ownership

Centred 260x104 windows have thin dark borders, a compact title and 6x8 text.
Controls are flat text choices, 60x10 pixels, with a contrasting selected row.
Filename fields show 39 characters, scroll to the insertion cursor, and are
highlighted when focused. Typing replaces the selected prefill; cursor movement
ends selection. Left/Right/Home/End, Backspace/Delete, Tab/Shift+Tab, Enter and
Escape retain their behaviours. Click inside the field to place the cursor.
Open, Save and Save As share this path; validation remains in the file adapter.

New offers 16/32/64 sizes using arrows, number keys or mouse. Confirmation
prompts initially select Cancel; Y/Yes approves, N/Escape/Cancel abandons the
operation. Enter activates the highlighted control. Error messages retain room
for both recovery paths, with forty characters per wrapped line. No file browser
is introduced.

All overlay painting hides the software cursor. Dialog polling consumes fresh
presses, including between consecutive prompts; dismissal blocks held buttons
until release before returning to the editor. Cancelled or failed Open retains
pixels, dimensions, filename, history and saved baseline. File-format handling
remains independent from UI code.
