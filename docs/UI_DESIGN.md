# Pix86 compact DOS interface

Pix86 stays in VGA Mode 13h (320x200, 256 colours), now with opaque frame
animation. The 36-pixel collapsible timeline keeps the sidebar/status visible,
refits integer zoom and offers nine numbered thumbnails, scrolling, navigation,
add/duplicate/delete, playback and timing. See [ANIMATION.md](ANIMATION.md) and
[ARCHITECTURE.md](ARCHITECTURE.md) for the current contracts and verification.
The reference review below records the earlier original interface foundations;
current controls supersede references below to planned/disabled animation.

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
transparent text never changes pixels outside the screen or a glyph's ink.
Opaque text writes final ink/paper per cell, avoiding a background wipe followed
by text. Control-local clipping bounds filename/title text. `font.h`
defines the shared 6x8 metrics. Rendering tests exercise the actual bitmap path.

`theme.h` names UI indices from the existing fixed palette: deep blue work area,
neutral menu/status bar, grouped dark sidebar, light grey dropdowns/dialogs,
dark ink, deep blue selection and
muted disabled text. The theme deliberately does not reprogram DAC entries: all
256 artwork indices retain their RGB values and existing `.P86` files render
identically. These original choices reflect conventional DOS colour
relationships, without reproducing a reference palette or screen.

## Workspace

The menu occupies y=0..8. The drawing region is x=44..317, y=14..189:
274x176 pixels (about 23% larger than the previous 222x176). The 42-pixel left
strip groups Tools, Colour, current FG/BG, sixteen working swatches and All.
Four 38x10 rows carry P/E/F/I shortcut labels and a full-row selection highlight.
Render and hit testing share layout.h. No right-hand panel remains.

Working swatches use 19x8 cells with 17x6 interiors. Eight stable defaults and
eight recent indices provide quick access; left/right selects FG/BG. A/B and
All open the full 16x16 popup, whose 10x7 cells expose all indices 0..255.
Arrows wrap, Home/End select 0/255, Tab changes FG/BG, Enter accepts, and Escape
or an outside click cancels. Mouse left/right immediately chooses FG/BG. [ and ]
cycle working foreground colours. File Open and eyedropper selections also
enter the recent strip, without changing any pixel index or saved file format.

Canvas sizes and maximum integer zooms remain 16/32/64 and 11/5/2. Grid starts
off, and G toggles contrasting dashed pixel boundaries at zoom >=3; 8/6 select solid tile guides at zoom >=2.
Black/white pixels use adjacent grey indices; colour-cube pixels use a nearby
shade. Dots reduce coverage compared with solid mesh lines. Rendering never
changes artwork. A future timeline can reserve view space without storage changes.

The single eight-pixel status line at y=192 shows the bounded DOS basename, modified marker, dimensions,
X/Y, tool, FG/BG indices and zoom. Without a mouse it retains document information
and uses X--Y--. Menus temporarily replace it
with navigation guidance. View changes do not mark the project modified.

## Menus and keyboard

The single menu row reads File, Edit, View, Frame, Options, Help, with a compact
`Ctrl+\` reminder at right. Ctrl+\ opens File from the editor and closes any
open menu on the next press. F10 and Alt+F/E/V/R/O/H remain alternatives. The R
in Frame avoids colliding with File; headings underline their mnemonic.
Left/Right wraps headings, Up/Down wraps rows, Enter executes and Escape closes.
Drawing shortcuts are consumed while a menu is open and retain their bindings
when closed. Frame operations now execute; only deferred onion skin is disabled.

Dropdowns use a one-pixel border and nine-pixel rows, left-aligned command names
and right-aligned shortcut labels. Disabled commands stay visible in muted ink
and cannot execute. Fresh mouse presses activate headings/items. An outside
left click closes the menu and is consumed; release before drawing again.
Dropdowns disappear before commands open dialogs. Opening a menu paints its bounds and changed headings/status; moving the
selection paints the old/new rows only. Switching headings or closing restores
only exposed rectangles from the editor model. No screen-sized backing buffer or
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

Content-sized centred windows (a filename prompt is 200x66; a short confirmation
is 140–146x52) have thin dark borders, a compact title and 6x8 text.
Controls are flat text choices, 44x10 pixels, with a contrasting selected row.
Filename fields show 32 characters, scroll to the insertion cursor, and are
highlighted when focused. Typing replaces the selected prefill; cursor movement
ends selection. Left/Right/Home/End, Backspace/Delete, Tab/Shift+Tab, Enter and
Escape retain their behaviours. Click inside the field to place the cursor.
Open, Save and Save As share this path; validation remains in the file adapter.

New offers 16/32/64 sizes using arrows, number keys or mouse. Confirmation
prompts initially select Cancel; Y/Yes approves, N/Escape/Cancel abandons the
operation. Enter activates the highlighted control. Error messages retain room
for both recovery paths, with forty characters per wrapped line. No file browser
is introduced.

Overlay painting protects the cursor only when their regions overlap. Dialog polling consumes fresh
presses, including between consecutive prompts; dismissal blocks held buttons
until release before returning to the editor. Cancelled or failed Open retains
pixels, dimensions, filename, history and saved baseline. File-format handling
remains independent from UI code.

## Refactor review

The additional source review, responsibility boundaries, supplied screencast
analysis and before/after measurements are recorded in [REFACTOR.md](REFACTOR.md).
The original font predates this refactor; a later refinement updates selected
glyphs while retaining its format and metrics. Layout,
opaque text and dirty painting make the interface denser and steadier without
scaling or importing any reference glyphs.

## Font and working-palette refinement

Eighteen original glyphs were redrawn to open counters, simplify heavy diagonals
and use single-pixel punctuation. The cell advance, baseline and 760-byte font
size remain unchanged. The original zero is narrower than O; lowercase remains
distinct. Help → Font sample shows all 95 printable characters and filename
ambiguities. [REFINE.md](REFINE.md) records the reference comparison and captures.

The all-colour popup shares existing modal polling, input ownership and covered
rectangle restoration. It paints once, then repaints only changed cells/value
text. Both pointer buttons and dismissal are consumed before drawing resumes.
The working palette is UI state, not a new artwork palette or remapping table.

## BMP export modal

File → Export adds an original compact modal with a shared bounded filename
field and three option rows: mode, sheet layout and grid columns. Relevant
rows use the existing flat selected-row style; disabled rows are dim and skipped
by Tab/Shift+Tab. Arrows/space or fresh mouse clicks change values. Enter exports,
Escape cancels, and idle polling paints nothing. After modal restoration and
result/overwrite prompts, held buttons remain consumed. The menu stays nine
pixels high. [Export details and captures](EXPORT.md).
