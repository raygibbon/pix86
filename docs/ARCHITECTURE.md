# Animation workspace architecture

Pix86 targets 8086 real-mode DOS (`-bt=dos -ms -0`) and VGA Mode 13h,
320×200×256, with optional banked VBE 640×480×256. There is no extender,
DPMI, SDL or added application dependency.

## Portable document and editor

`anim.h` defines the original `Animation` storage contract: supported dimensions,
up to 16 frames, 16,384 total indexed pixels and per-frame millisecond durations.
`anim.c` validates the six shapes. All indices can be opaque, including zero; packed masks independently mark transparency. Opaque blank
frames use BG. `fileio.h` aliases this portable structure as `P86Image` for the
codec. Nothing in the animation representation depends on DOS video or file I/O.

`editor.c` retains the existing 4096-byte active Canvas and original pencil,
eraser, fill and pick tools. The active canvas is flushed to its frame before
switching, structural edits, capture or playback advancement. Frame operations
insert after the active frame, preserve at least one frame, wrap navigation,
retain duplicated timing, and enforce both count and total-byte limits.
Selection and playback do not enter history or change saved content. Explicit
20..2000 ms durations default to 100 ms; timing commands adjust by 10 ms.
`editor_tick` accepts elapsed milliseconds and advances at most one frame per
presentation. If a render misses multiple deadlines, it discards that elapsed
backlog instead of catching up across and hiding frames; the next frame receives
its full duration. Stored durations are never changed by playback. The DOS
controller supplies `clock()` elapsed time; BIOS-resolution timing is
approximate, not a precision multimedia timer. No timer interrupt or
polling-count animation is used. Playback blocks drawing and stops for
overlays/editing. Onion skin is deferred.

`editor_init` creates a fresh object and preallocates live document, saved
baseline and five history states. Failure releases every successful allocation.
`editor_dispose` ends its lifetime. New/Open reuse allocated storage and cannot
fail halfway through allocation. New validates shape before changing anything;
Open fully decodes into independent scratch before applying. Callers must dispose
before reinitializing an existing editor. New/Open reset held gesture state.

## History and dirty state

`history.c` retains one base plus four undoable actions, using separate allocated
whole-document snapshots. Snapshots contain all frames and durations and the
frame selection associated with an edit. Undo of pixels after navigation returns
to the edited frame. Structural undo restores deleted pixels and timing. Navigation
alone neither commits nor truncates redo. Actual no-op edits preserve redo.
Oldest snapshots expire by rotating pointers, not moving large history arrays.
Pencil/eraser commit once on release; fill/clear and structural/timing commands
commit once. Tools, palette selection and view remain outside pixel history.

Dirty tracking compares exact saved pixels, dimensions, frame count, durations
and FG/BG, independently of history position. The active frame compares its near
Canvas bytes; other frames compare their document storage. Therefore a held
stroke is dirty immediately, and undo can return to saved content after expiry.
Selection, grid, timeline, zoom, scroll and playback are view state. FG/BG remain
persisted selections and mark dirty. No probabilistic hash is used.

## View, timeline and input

`view.c` uses integer zoom and half-open mouse bounds. `view_fit` accepts the
workspace reservation and recentres/refits on timeline transitions or document
size changes. Closed bounds remain x=44..317, y=14..189 (274×176). Open bounds
use 274×140, leaving a two-pixel gap before the panel at y=156..191. Status stays
at y=192..199; the sidebar is unchanged. 32×32 refits from 5× to 4×; 64×64
remains 2×. All shapes permit 1×. Artwork indices and pixel proportions never change.

`layout.h` shares timeline/control geometry. Nine visible 24-pixel frame cells
carry numbers and original nearest-sample 8×8 thumbnails. Scroll arrows shift the
visible window; selection follows into view. The top row supplies previous,
next, blank, duplicate, delete, play/stop, scroll left/right and close. The panel
also reports active/count and duration. Tab toggles it; comma/period navigate,
Insert/D/Delete edit frames, Space plays, parentheses adjust duration.

`keymap.c` owns aliases; `ui.c` routes menus before timeline and canvas. Fresh
left presses on the timeline are consumed, and UI blocking owns the held button
until release, including when moved over canvas. A drag beginning in canvas
cannot turn into a timeline command. Menu/modal entry finishes a stroke.
Ctrl+\ BIOS decoding, F10, Alt headings and plain/AltGr path separators remain
unchanged. `menu.c` derives widths, prefix matching and rows from command tables,
retains per-heading row memory and prevents disabled commands from executing.
There are no separator rows in the current tables.

## Rendering and typography

`render.c` shares the existing cell painter across full/partial updates. G draws
a black/white contrasting dashed mesh at zoom ≥3. Independent 8/16 guides use
solid boundaries with alternating contrasting companion marks at zoom ≥2.
Luminance comes from the existing fixed DAC formula. These effects never write
to Canvas bytes or change the artwork DAC. Suppressed grids return on zoom-in.

`font.c` keeps the original 760-byte, 95-glyph, 6×8 face, with selected original
lowercase/symbol redraws; spacing column and transparent/opaque clipping remain.
No third-party glyph data or scaling is used. Menu bar stays nine pixels; wider
dropdowns preserve right-aligned shortcuts and exact shared hit geometry.

`screen.c`, `region.c`, `menudraw.c`, `uipaint.c` and `dialog.c` retain changed-row
painting, clipped model restoration, cursor intersection protection and a single
startup full-screen clear. Ordinary strokes and fills collect changed cell indices
in the existing fill queue and share the cell painter; they do not allocate another
dirty buffer. Frame navigation compares visible pixels/transparency and updates
only changed cells and frame indicators. Timeline thumbnails are repainted only
when their visible content changes; structural timeline changes repaint the
timeline. Timeline collapse repaints its complete old footprint, including the
right/bottom margins. No screen buffer or widget framework is added.
Playback compares the new active frame to the stored previous frame and selects
changed cells, same-colour/opacity spans, or complete artwork-row scanlines.
The last path uses one 640-byte indexed line, never a whole-artwork framebuffer.
Dense texture benefits from contiguous VGA/VBE copies; flat colour and checker
runs use fills. Grid, guide and selection overlays retain the cell compositor.
Cursor protection covers actual writes, including unchanged artwork within a
complete-row copy. Menus, toolbars, palettes and unrelated workspace do not
repaint on frame transitions. The first transition also uses the diff path.
The thresholds, memory measurements and native evidence are recorded in
[PERFORMANCE.md](PERFORMANCE.md).

`timer.c` provides read-only AT/8254 timing with an XT/coarse fallback. It owns
only timer sampling, not animation state, durations or interrupt handlers.
Presentation diagnostics include all drawing through cursor restoration and
exclude waiting. `main.c` retains fractional poll time and counts drawing time
toward deadlines; the sequencer still displays every frame in order. Stopped
polling retains the existing DOS delay and writes no pixels.
Canvas edits outside playback also refresh the compact panel thumbnails. Font
diagnostics use a bounded 280-byte automatic string; normal control buffers
remain bounded.

The video drivers exclusively own VGA/VBE memory, BIOS mode entry/restoration and fixed DAC
setup. `mouse.c`, `keyboard.c` and `cursor.c` retain their hardware boundaries.
`main.c` owns elapsed-time acquisition, logical command dispatch and modal routing.

## Persistence and failure boundaries

`fileio.c` explicitly encodes little endian fields, validating all dimensions,
counts, durations and exact lengths before application. Version 1 remains its
24-byte header plus exactly one supported square image. Version 2 stores an
explicit count and sequential duration/pixel records, including rectangles.
See [FILE_FORMAT.md](FILE_FORMAT.md). The low-level version 1 writer is retained
for independent compatibility fixtures; application saves use version 3.

`project.c` captures into one near scratch image, preserving the checked exclusive
TMP creation, fwrite/flush/close checks, approved replacement, BAK rollback and
retained-recovery reporting. Failed reads/writes leave artwork, filename and saved
baseline intact. The codec's failed scratch contents are unspecified. The native path remains independent from exported images.

`bmp.c` streams original uncompressed 8-bit BMP using a 128-byte span buffer,
explicit little-endian headers and the unchanged fixed DAC palette. `export.c`
maps Animation frames/sheets into spans, validates DOS names and dimensions,
preflights complete numbered batches and reports partial completion. `file_write`
shares the checked transaction through a synchronous encoder callback; `file_save`
retains the native adapter. `dialog_export` reuses bounded filename editing,
modal restoration and input suppression. The controller captures the existing
scratch image without advancing the saved baseline. [Export specification and
verification](EXPORT.md) records the Microsoft factual references and streaming
bounds; no dependency or full-sheet allocation is added.

## Measured 16-bit memory budget

The 4096-byte stack, strict ordinary modules and extended-CRT file adapter remain.
Watcom uses `_fmalloc`/`_ffree` and declared CRT far-memory interfaces for seven
separate document blocks; hosts use malloc/free. Each block is 18,474 bytes in
DOS, below 32 KiB. All seven reserve 129,318 bytes outside DGROUP. The near active
canvas is 4096 bytes; fill queue 8192; near file scratch 18,474. No large automatic
images or large frame arrays live in DGROUP. Smaller documents retain this fixed
bounded reservation so editing/history do not allocate during an operation.

The current linked map reports DGROUP 43,280 bytes including the stack (22,256
bytes below 64 KiB), near code 60,830 bytes (4,706 bytes below 64 KiB).
`PIX86.EXE` is 66,382 bytes; seven far blocks reserve another 129,318 bytes.
PSP/environment, heap bookkeeping, stdio and loader overhead are additional.
Initialization on insufficient conventional memory fails with a diagnostic.
Up to 16 frames are allowed for every shape except 64×64 (four). Packed transparency is implemented; onion skin and layers remain deferred.
BMP and bounded streaming PNG export are implemented. Native hardware and
FreeDOS/MS-DOS remain untested; DOSBox-X evidence is in TESTING.md.

## Provenance and engineering references

- [Zine build script](https://github.com/raygibbon/zine/blob/49d292f7cfcfd7766cf6616ba4c7f65fea7b57b0/scripts/build-zine.sh),
  inspected in `~/Sync/git/zine` at that commit. Used as an engineering reference
  for Linux-hosted Watcom, `WATCOM`, `binl`, `h`, `lib286`, shell builds and `mnt/`.
  Pix86's script and C modules were written independently; no Zine source or
  third-party components were imported. Zine's large memory model is unnecessary
  for this small canvas, so Pix86 uses small model with bounded far document blocks.
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

## Selection and integrated display drivers

`select.c` owns the transient rectangle/floating state machine. Pixel-space
coordinates do not depend on the video mode. The 8192-byte fill queue is exclusively
reused while floating: 4096 index bytes followed by 512 packed mask bytes. Source
is captured once; preview sampling reads original destination, source capture and
clear policy. Transparent source mask bits leave the cleared/original destination;
opaque black is copied as index zero. Apply samples each destination once, commits
through existing history, and clips the active marquee to the visible result.
No-op history remains deduplicated. Cancel and selection-only actions never touch
pixels, masks or history. New/Open, frame transitions and undo clear stale state.

`EDIT_SELECTION` reuses the bounded canvas stroke repaint without updating frame
thumbnails during preview. Previous/current selection bounds form the damage;
Cancel also restores the source. Static dotted edges are composed by canvas cells,
so clipped overlay restoration redraws the same floating result. Starting a float,
pressing inside it, or releasing without movement only changes status; it does not
repaint the canvas. Idle paints remain suppressed. Tool buttons repaint only old
and new selections. Cursor protection remains intersection-based.

`layout.c` supplies validated 320×200/640×480 dimensions; `layout.h` derives
palette, right toolbar, drawing and timeline reservations. UI text uses the original
6×8 face at native size in both modes; larger tool bitmaps are separately drawn,
not scaled copies. Status buffers reserve the maximum 640-pixel text width.

The first-party video drivers and driver test are integrated from
`feature/dual-display-modes` commit `70d79e5bab4ae285814e5c4a5dc0ca5b5917efac`.
That branch predates animation/transparency/export. Its video adapter was reused
under this repository's existing MIT licence while current document/UI features
were retained. `video.c` dispatches clipped spans to VGA or banked VBE; `mouse.c`
maps the driver's 640-wide virtual coordinates to the active mode. `/HIRES` requests
mode 101h and falls back to VGA on unavailable VBE. Bank failure exits to text
with a diagnostic. No external library or reference-application code was imported.
See [current measurements and limitations](SELECTION.md); the memory summary above includes this integrated build.

## Compact workspace and temporary overlays

The current layout reserves only left palette, right groups and the bottom status
row. `DRAW_Y=1`; the 9-pixel menu bar is an overlay, independent of view fitting.
Normal drawing bounds are 256×189 or 566×469. Tab retains the 36-pixel secondary
timeline. Frame navigation in the status row is independent of that timeline:
`flyout.c` derives five/sixteen available number targets from screen/font metrics,
plus previous/next arrows. A deterministic page around the active frame keeps
numbers visible without adding document or scrolling state. Only actual frames
are drawn; selecting/navigation uses existing logical frame commands.

`Flyout` is two transient integers (group, hover), stored in `UiState` and cached
by `Screen`. Two remembered icon bytes in Editor are view state only. Keyboard
shortcuts and popup choices update the same memories; Select All has an original
seventh icon but remains an action, not a persistent image tool. No unimplemented
shape buttons are shown. Group geometry, icon size and navigation cells share
`layout.h`/`flyout.c`; no widget framework or allocation is introduced.

`ui.c` gives menus priority over flyouts and both priority over editor input.
Menu activation closes a flyout; right-clicking a group can replace an open menu.
A dismissal press and its held release remain owned by UI blocking. Modal loops
retain first ownership. Opening an overlay finishes an ordinary stroke and pauses
a selection drag at its current rectangle/destination, without applying/cancelling
a floating move. Choosing Move again preserves the capture and offset.

`screen.c` restores closed/switching overlay footprints via `render_restore`,
then paints the new overlay. `menudraw.c` paints a complete bar only on opening;
row/heading changes stay incremental, and dismissal paints no headings. Menu
opening/closing leaves the status row untouched. Flyout hover paints only its
small region; tool changes repaint affected group buttons. Status text is cached
separately from frame-number highlighting, which redraws only when frame/count
changes. There are no idle writes or full-screen overlay buffers.

Clipped editor restoration includes row zero, artwork, checkerboard, grid, guides,
marquee/floating sampling, palette, groups and optional timeline. An outer cursor
protection covers the actual clipped footprint; nested renderer protections are
suppressed during restoration so an unrelated cursor is not needlessly hidden.
[COMPACT.md](COMPACT.md) records native evidence, measured limits and reference
provenance. Persistent v3 data, history snapshots and export encoders are unchanged.
