# M2.5 architecture

Pix86 remains a 16-bit real-mode DOS application. M2.5 adds native project
saving/loading, original filename/confirmation dialogs and exact saved-state
tracking to M2's indexed canvas, tools, zoom and bounded undo. Animation, layers,
non-native formats and other platforms remain outside this milestone. The
Two display modes share the same editor and artwork: Classic Mode 13h
(320x200x256), the default, and Enhanced VBE 101h (640x480x256), selected with
`/640`. `/320` explicitly selects Classic; `/HIRES` remains an alias. Unsupported
VBE hardware falls back to Classic with a diagnostic.

## Data and tools

`canvas.c` owns the portable interface to caller-supplied pixel storage. A byte
is a palette index, including 0 and 255. Get/set reject out-of-bounds coordinates;
clear fills the canvas. The generic interface caps objects at 32767 bytes, while
The editor restricts a canvas to a 16×16, 32×32 or 64×64 square (4096 bytes maximum).
Size is selected at launch, with New, or by opening a project. There is no
resampling operation; New/Open reset history after user confirmation if dirty.

`tools.c` contains independently callable pencil, eraser, fill and pick functions.
Pencil connects sampled positions with an original integer error-accumulation
line implementation, inclusive of both endpoints; eraser uses the same path
with the configured background. Endpoints must be inside the M2 canvas, bounding
coordinate differences to 63 and intermediate line errors well within signed
16-bit range. No floating-point arithmetic is used.

Fill visits four-connected matching indices using a caller-owned queue of 4096
unsigned-short pixel offsets. It marks a pixel before enqueuing it, so each pixel
enters the queue at most once. There is no recursion or growing allocation.
Same-colour fills return immediately. Boundary checks prevent horizontal row
wrapping and diagonally touching regions do not connect. Pick reads the software
canvas, including positions covered by grid lines or the visible cursor.

## History and action boundaries

`history.c` stores five fixed 4096-byte slots: one base state and at most four
undoable actions. Metadata records canvas dimensions, byte count, current
position and state count. Only the active canvas bytes are compared/copied.
On a fifth edit, the oldest state expires; storage never grows. Full snapshots
are deliberately bounded and simple at these small canvas sizes, rather than
introducing variable-sized change logs. Each history object is below 32 KiB.

Pencil/eraser samples mutate the live canvas during a drag, then commit once on
release, including the final release position. Fill and clear commit once.
A no-op creates no history entry and preserves redo. A real edit after undo
truncates redo; palette selection, picking, tools, grid and zoom do not edit art.
Undo/redo copy pixel data only, leaving foreground/background and view unchanged.
Canvas dimensions stay fixed between New/Open operations, which reset history;
history rejects a mismatched canvas.

`editor.c` owns application state and gesture handling, independently of DOS,
mouse hardware or rendering. A stroke starts only on a new left press inside the
canvas. Moving outside pauses it and breaks interpolation; re-entry starts a new
segment in the same action. A press starting on the toolbar/palette cannot become
a canvas stroke. Tools and foreground/background are captured for a held stroke;
keyboard actions finish it and require release before another can start.
Right-click canvas picks colour; right-click palette configures the background.

## View, rendering and platform boundaries

`view.c` calculates integer zoom, centred origins and mouse-to-canvas coordinates.
Shared `layout.h` defines runtime dimensions and font metrics, the drawing area,
compact tool hit boxes, palette and menu/status boundaries. Mode 13h uses
x=44..317, y=14..189; VBE 640x480 uses x=86..637, y=22..461. Zoom is an integer
from 1 to the largest fitting scale for the active workspace and canvas size.
Mapping uses half-open bounds; the last displayed pixel belongs to the last
canvas cell. Long subtraction protects negative/outside screen coordinates.
Grid pixels map to their containing cell. Palette hit testing maps the sidebar
working slots through workpal.c to exact VGA indices without touching canvas data.

`render.c` draws toolbar, palette, foreground/background swatches, software canvas
and status through the video interface. Shaded grid dots are view-only at zoom >=2 (off initially),
and can be disabled. Pencil/eraser redraw only the affected bounding rectangle;
fill, history and view changes redraw the canvas. Zoom restores only the area outside the new art rectangle, then paints final
pixels/grid directly. Cursor protection hides the pointer only for an intersecting
repaint; unchanged pointer positions perform no VGA reads or writes.

`video.c` owns the shared mode selection/restoration, clipping, palette setup
and dispatch. `videovga.c` implements packed Mode 13h pixels; `videovbe.c`
validates VBE 101h and accesses packed pixels through the BIOS-selected banked
window. The 307,200-byte Enhanced image is not treated as a linear pointer and
no screen-sized shadow buffer is allocated. Rectangle endpoints use long
arithmetic before clipping; bank errors are surfaced to the main loop for safe
restoration. Text uses original bitmap faces: Classic has 6x8 editor/status
text and 4x6 menu/dialog text; Enhanced uses an independently authored 8x16
face. Shared `font.h`/`layout.h` metrics drive rendering and hit geometry.
`theme.h` chooses UI colours from the existing palette without DAC changes.

`mouse.c` owns INT 33h polling, null/IRET vector checks, driver reset, position
and button reads. Its horizontal range is scaled to the active screen width;
vertical bounds follow the active mode.
`menu.c` owns portable keyboard/mouse menu state, bounded command descriptions,
separator/letter/row geometry and six bytes of per-heading selection memory;
`menudraw.c` paints overlays. `keymap.c` interprets extended menu mnemonics.
The editor remains unaware of these modules.
`keyboard.c` polls BIOS keyboard words (AH scan, AL character) and current shift
flags, preserving information lost by console getch(). `keymap.c` decodes these
portably; Ctrl+\ opens/closes File, with conventional FS (1Ch) plus scan/logical
fallbacks. Plain/AltGr backslash remains editable. Extended keys stay 256 + BIOS
scan code (F2=60, F3=61, Shift+F2=85). ASCII drawing keys retain their values.
See UI_DESIGN.md for layout/queued-modifier limitations and factual references.
`cursor.c` saves/restores 64 bytes for the original pointer using only the video
interface. `main.c` parses launch options, polls input, dispatches dirty render
flags, dispatches file commands, and confirms dirty ESC before normal cleanup. No interrupt handlers are installed.

## UI state, layout and invalidation

`ui.c` owns the live Menu and held-button suppression state; it has no document,
file or video dependency. Dialog focus/input state is local to each synchronous
modal invocation. `main.c` executes routed file/help/editor commands.
`dlggeom.c` computes bounded content-sized dialog rectangles; `dlginput.c` edits
text without painting. `uipaint.c` shares thin borders and flat highlighted
buttons; menu rows and filename fields use opaque font cells with local clipping.

`screen.c` caches the previous visible menu, tool/colours, status string and
bounded basename and working-palette snapshot (102 bytes with 16-bit ints). It compares state and emits only
changed rows, controls and status character runs. `region.c` subtracts the new
menu bounds from the old bounds into at most four disjoint exposed rectangles.
`render_restore` clips model repainting to each rectangle; no saved-screen
allocation, damage queue or framebuffer is needed. Actual document flags still
select full-canvas updates for history/zoom/fill and bounded stroke updates for
pencil/eraser. There is one full-screen clear, at startup.

The bottom status row uses complete font cells at runtime: 53 six-pixel cells
in Classic and 80 eight-pixel cells in Enhanced. It shows a 12-character
DOS basename, dirty marker, dimensions, coordinates, tool, FG/BG and zoom;
paths remain fully available in the filename field and project model. The
Mode 13h workspace is 274x176; VBE 640x480 provides 552x440. A future timeline
can reserve layout space without changing canvas/file storage; no timeline is
implemented here.

## Working colour selection

`workpal.c` owns sixteen UI shortcuts to unchanged VGA indices: eight fixed
defaults and eight recent colours. `palette_hit` translates sidebar geometry to
these indices; editor painting/picking and persisted FG/BG semantics are unchanged.
`screen.c` remembers new FG/BG from picker, file load or eyedropper, compares the
previous sixteen slots and repaints changed swatches/markers. It has no palette
RGB editor. The module uses 16 bytes of writable data and a 16-byte default table.

`dialog_palette` extends the existing modal path with a bounded colour popup
sized for the active font and display. It stores only selection/target and input state; arrow navigation paints
the old/new cells, Tab changes the target and idle polls perform no painting.
Acceptance returns a stored index and FG/BG target; the controller updates those
selections and the existing dirty flags. Cancellation leaves them untouched.
Both mouse buttons and outside dismissal are consumed. `dialog_font` displays
printable ASCII in the active display's font roles when launched with `/FONT`.
Its dismissal uses the same model restoration and pointer suppression.

``render.c` shares one cell painter between full and stroke updates. Grid dots
use nearby shades from the existing fixed DAC palette, never canvas mutations;
single dots use the pixel primitive rather than rectangle setup.

## Native I/O and project state

`fileio.c` defines a versioned binary format and bounds all dimensions/lengths
before reading at most 4096 pixel bytes. It encodes little-endian integers
explicitly; no C structs are serialized. [FILE_FORMAT.md](FILE_FORMAT.md) is the
independent reader specification, including the implicit fixed palette and
future-version boundaries. Version 1 has one image, no animation or palette RGB
block. The low-level decoder writes caller-owned scratch; failed scratch contents
are unspecified and are never applied to live art.

The file adapter accepts bounded DOS 8.3 paths, supplies .P86, canonicalizes case,
rejects device names and uses exclusive temporary creation. DOS uses the Watcom
CRT's `_open`/`_fdopen`/`_stat`; the host test adapter uses POSIX equivalents. This
single module is compiled with Watcom `-ze` for those declarations; the remaining
application modules retain strict ANSI mode `-za`. There is no new library.

A save checks every fwrite, fflush and fclose before touching an existing target.
A completed same-directory TMP is installed by rename. Replacement first renames
the old target to a free BAK, then installs the TMP, then removes BAK. On failed
installation, it attempts to restore BAK; if rollback also fails it retains both
old BAK and new TMP and reports their paths. Failure to remove an old backup after
successful installation is a warning/success, not a failed save. Directories and
inaccessible targets are rejected. This supports single-writer DOS, not concurrent
writers or power-failure atomicity. Recovery details and filename limits are in
the format document; stale recovery files are never automatically discarded.

`project.c` ties file operations to an Editor using one shared P86Image scratch
buffer and a bounded current filename. A failed load never applies scratch.
A successful load replaces pixels/dimensions/FG/BG, resets history, recentres at
maximum fitting zoom, and records the saved baseline. A save records a new
baseline/current filename only after successful installation (including the
retained-backup warning case). A failed write leaves both unchanged. New resets
to an unnamed blank canvas. Saving commits any held stroke before capture.

`editor.c` keeps a separate saved-pixel snapshot and saved dimensions/FG/BG.
`editor_is_dirty` compares actual content, not history position or a hash. It
therefore recognizes a return to saved art even when the original history entry
has expired, and it cannot confuse another edit at the same history position
with saved art. FG/BG are persisted selections, so changing them also marks the
project dirty; view/tool choices do not. A new blank canvas is an untitled clean
baseline until content/selections change.

`dlginput.c` is portable filename editing state: selected prefill, bounded insertion,
Backspace/Delete and Left/Right/Home/End. `dialog.c` draws centred VGA boxes and
polls keyboard/mouse input without knowing file parsing. Tab/Shift+Tab select
controls; fresh mouse presses activate buttons or place the text cursor.
Idle polling moves/restores the software cursor without repainting the box. The
32-character filename window follows the insertion cursor. Destructive prompts
default to Cancel; Y explicitly approves. New offers the three canvas sizes.
Each modal exit restores only its box from the editor model through a VGA clip.
`ui.c` routes overlays before commands, consumes outside clicks and blocks held
buttons until release. `keymap.c` maps shortcut aliases to the logical constants
in `command.h`; editor operations and menu descriptions share those commands. Menu overlays have no screen-sized backing buffer. See
[UI_DESIGN.md](UI_DESIGN.md) for interaction details and reference provenance.

F2 saves the current name (or prompts), Shift+F2 always prompts, F3 opens, and N
starts a new canvas. Existing targets always require overwrite confirmation.
New/Open/Exit require explicit discard confirmation when dirty. Cancel or failed
Open preserves current artwork/name; users can cancel Exit, save, then exit.
Success/failure and retained recovery paths are displayed and acknowledged.

## 16-bit memory budget

The build retains `wcl -bt=dos -ms -0`: 16-bit DOS, small model, 8086 instructions,
with a 4096-byte stack. Only the file adapter enables CRT extension declarations.
No DPMI, extender, graphics framework or new dependency
is used. All DOS-facing source and executable names fit 8.3.

| Fixed application buffer | Bytes |
| --- | ---: |
| Canvas, reserved for largest size | 4096 |
| Five history slots | 20480 |
| Fill queue, 4096 16-bit offsets | 8192 |
| Exact saved-pixel baseline | 4096 |
| Shared load/save scratch pixels | 4096 |
| Cursor saved pixels | 64 |
| Total pixel/history/work buffers | 41024 |
| Original printable-ASCII faces, including Enhanced raster | 2850 |

The Editor object adds small metadata; the fill queue is a separate static
object to keep individual near objects below 32767 bytes. Smaller canvases
retain the same fixed reservation. Large objects are static, never automatic;
there is no explicit application heap allocation and no full-screen shadow
buffer. Standard stdio may allocate bounded runtime buffering internally.
The link map, including runtime code/data and the 4096-byte stack, reports about
89 KiB in total; code and DGROUP each remain below 64 KiB. DGROUP is about
50 KiB, leaving room within the near segment for runtime file buffering. Exact measured values are in TESTING.md.
The executable file is smaller because DOS zero-initializes BSS. Loader/PSP,
environment and runtime bookkeeping are additional to the linked footprint.

Launch from a DOS text mode (0..3 or 7) on VGA hardware/emulation. Confirmed ESC
resets an initialized mouse driver and restores the prior BIOS mode,
active page, text cursor position and shape. Restoration clears prior screen
contents; custom fonts/palettes and previous text contents are not preserved.
Abrupt process termination/emulator closure is outside normal cleanup.

`/640` requests VBE mode 101h, not a nonstandard BIOS 640x400 mode. VBE probing
and bank selection are isolated in `videovbe.c`; unsupported firmware falls
back to Mode 13h with a diagnostic. `/320` explicitly selects Classic and
`/HIRES` is an alias for `/640`. Options → Display Mode explains that changing
mode requires restarting. `docs/VIDEO_MODES.md` records trade-offs, VBE memory
organization, captures and DOSBox-X measurements. Mode 13h remains the
recommended default.

## Provenance and engineering references

- [Zine build script](https://github.com/raygibbon/zine/blob/49d292f7cfcfd7766cf6616ba4c7f65fea7b57b0/scripts/build-zine.sh),
  inspected in `~/Sync/git/zine` at that commit. Used as an engineering reference
  for Linux-hosted Watcom, `WATCOM`, `binl`, `h`, `lib286`, shell builds and `mnt/`.
  Pix86's script and C modules were written independently; no Zine source or
  third-party components were imported. Zine's large memory model is unnecessary
  for this small canvas, so Pix86 uses small model.
- [Open Watcom C/C++ User's Guide](https://open-watcom.github.io/open-watcom-v2-wikidocs/cguide.html):
  compiler targets, memory models and 8086 generation; local Watcom headers define
  the interrupt-call interfaces. The toolchain is installed externally; its
  licensing remains separate from Pix86's MIT-licensed source.
- [Ralf Brown's Interrupt List: mouse reset](https://www.delorie.com/djgpp/doc/rbinter/id/75/59.html)
  and [position/buttons](https://www.delorie.com/djgpp/doc/rbinter/id/78/59.html):
  factual interrupt contracts and absent-driver precautions, not source code.
- [DOSBox-X documentation](https://dosbox-x.com/wiki/Home): emulator configuration
  and launch options. The emulator is a test tool, not a shipped dependency.

GrafX2 and Autodesk Animator/Animator Pro remain functional references only.
Neither their source nor their assets were consulted or incorporated. LibreSprite
and TDE also remain workflow/UI references only. A limited local TDE source
distribution review identified user-facing behaviours; no implementation or assets
were copied, translated or adapted. Exact reference files and the independent
Pix86 design are recorded in UI_DESIGN.md. Zine's util.c was inspected for DOS filename/case
and standard file-handling conventions. No implementation was reused; Pix86's
checked-write/replacement strategy and format are original.
The workspace, pointer, swatches and module implementations are original.
