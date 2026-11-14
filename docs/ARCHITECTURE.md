# M2 architecture

Pix86 remains a 16-bit real-mode DOS application. M2 adds one indexed drawing
canvas, four drawing tools, integer zoom, grid, palette selection and bounded
undo/redo. Animation, file formats, saving/loading, layers, alternative graphics
modes and additional platforms remain outside this milestone.

## Data and tools

`canvas.c` owns the portable interface to caller-supplied pixel storage. A byte
is a palette index, including 0 and 255. Get/set reject out-of-bounds coordinates;
clear fills the canvas. The generic interface caps objects at 32767 bytes, while
M2 restricts the session to a 16×16, 32×32 or 64×64 square (4096 bytes maximum).
Size is selected at launch; there is no destructive runtime resize operation.

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
Canvas dimensions stay fixed, and history rejects a mismatched canvas.

`editor.c` owns application state and gesture handling, independently of DOS,
mouse hardware or rendering. A stroke starts only on a new left press inside the
canvas. Moving outside pauses it and breaks interpolation; re-entry starts a new
segment in the same action. A press starting on the toolbar/palette cannot become
a canvas stroke. Tools and foreground/background are captured for a held stroke;
keyboard actions finish it and require release before another can start.
Right-click canvas picks colour; right-click palette configures the background.

## View, rendering and platform boundaries

`view.c` calculates integer zoom, centred origins and mouse-to-canvas coordinates.
The drawing area is x=40..215, y=20..187. All zooms from 1 to the largest fitting
integer are accepted: maximum 10, 5 or 2 for the three sizes respectively.
Mapping uses half-open bounds; the last displayed pixel belongs to the last
canvas cell. Long subtraction protects negative/outside screen coordinates.
Grid pixels map to their containing cell. Palette hit testing maps the right
16×16 array to indices 0..255 without touching canvas data.

`render.c` draws toolbar, palette, foreground/background swatches, software canvas
and status through the video interface. Grid lines are view-only at zoom >=2,
and can be disabled. Pencil/eraser redraw only the affected bounding rectangle;
fill, history and view changes redraw the canvas. Zoom redraw clears the old
canvas area. The cursor is hidden before changing any screen pixels and shown
afterwards, so its saved pixels cannot overwrite an edit.

`video.c` exclusively owns VGA memory, BIOS video calls and DAC I/O. It checks
Mode 13h entry, clips drawing, and uses unsigned offsets up to 63999 for VGA.
Rectangle endpoints use long arithmetic before clipping; rectangles write directly
to VGA within this module to avoid a per-pixel function call. Text uses the
machine's BIOS 8×8 font; no font is bundled. The original M1 DAC palette remains
unchanged, with all indices exposed in M2 (some RGB entries intentionally repeat).

`mouse.c` owns INT 33h polling, null/IRET vector checks, driver reset, position
and button reads. Mode 13h's 640-unit horizontal range is divided by two.
`keyboard.c` owns nonblocking DOS console input and consumes extended-key pairs.
`cursor.c` saves/restores 64 bytes for the original pointer using only the video
interface. `main.c` parses launch options, polls input, dispatches dirty render
flags, and performs normal ESC cleanup. No interrupt handlers are installed.

## 16-bit memory budget

The build retains `wcl -bt=dos -ms -0`: 16-bit DOS, small model, 8086 instructions,
with a 4096-byte stack. No DPMI, extender, graphics framework or new dependency
is used. All DOS-facing source and executable names fit 8.3.

| Fixed application buffer | Bytes |
| --- | ---: |
| Canvas, reserved for largest size | 4096 |
| Five history slots | 20480 |
| Fill queue, 4096 16-bit offsets | 8192 |
| Cursor saved pixels | 64 |
| Total pixel/history/work buffers | 32832 |

The Editor object adds small metadata; the fill queue is a separate static
object to keep individual near objects below 32767 bytes. Smaller canvases
retain the same fixed reservation. Large objects are static, never automatic;
there is no application heap allocation and no full-screen shadow buffer.
The link map, including runtime code/data and the 4096-byte stack, reports about
54 KiB; DGROUP remains below 64 KiB. Exact measured values are in TESTING.md.
The executable file is smaller because DOS zero-initializes BSS. Loader/PSP,
environment and runtime bookkeeping are additional to the linked footprint.

Launch from a DOS text mode (0..3 or 7) on VGA hardware/emulation. ESC saves no
artwork, resets an initialized mouse driver and restores the prior BIOS mode,
active page, text cursor position and shape. Restoration clears prior screen
contents; custom fonts/palettes and previous text contents are not preserved.
Abrupt process termination/emulator closure is outside normal cleanup.

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
  [BIOS font information](https://www.delorie.com/djgpp/doc/rbinter/id/63/1.html)
  specifies the runtime ROM font pointer interface.
- [DOSBox-X documentation](https://dosbox-x.com/wiki/Home): emulator configuration
  and launch options. The emulator is a test tool, not a shipped dependency.

GrafX2 and Autodesk Animator/Animator Pro remain functional references only.
Neither their source nor their assets were consulted or incorporated for M1 or M2.
The workspace, pointer, swatches and module implementations are original.
