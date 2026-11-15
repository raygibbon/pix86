# Animation workspace architecture

Pix86 targets 8086 real-mode DOS (`-bt=dos -ms -0`) and VGA Mode 13h,
320×200×256. There is no SVGA, extender, DPMI, SDL or added dependency.

## Portable document and editor

`anim.h` defines the original `Animation` storage contract: supported dimensions,
up to 16 frames, 16,384 total indexed pixels and per-frame millisecond durations.
`anim.c` validates the six shapes. All indices are opaque, including zero; blank
frames use BG. `fileio.h` aliases this portable structure as `P86Image` for the
codec. Nothing in the animation representation depends on DOS video or file I/O.

`editor.c` retains the existing 4096-byte active Canvas and original pencil,
eraser, fill and pick tools. The active canvas is flushed to its frame before
switching, structural edits, capture or playback advancement. Frame operations
insert after the active frame, preserve at least one frame, wrap navigation,
retain duplicated timing, and enforce both count and total-byte limits.
Selection and playback do not enter history or change saved content. Explicit
20..2000 ms durations default to 100 ms; timing commands adjust by 10 ms.
`editor_tick` accepts elapsed milliseconds, loops through frame durations and
reduces long delays modulo total cycle length. The DOS controller supplies
`clock()` elapsed time; BIOS-resolution timing is approximate, not a precision
multimedia timer. No timer interrupt or polling-count animation is used.
Playback blocks drawing and stops for overlays/editing. Onion skin is deferred.

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
startup full-screen clear. Timeline collapse repaints its complete old footprint,
including the right/bottom margins. No screen buffer or widget framework is added.
Playback repaints only after frame advancement; stopped/idle polling writes no
pixels. Canvas updates also refresh the compact panel thumbnails. Font diagnostics
use a bounded 280-byte automatic string; normal control buffers remain bounded.

`video.c` exclusively owns VGA memory, BIOS mode entry/restoration and fixed DAC
setup. `mouse.c`, `keyboard.c` and `cursor.c` retain their hardware boundaries.
`main.c` owns elapsed-time acquisition, logical command dispatch and modal routing.

## Persistence and failure boundaries

`fileio.c` explicitly encodes little endian fields, validating all dimensions,
counts, durations and exact lengths before application. Version 1 remains its
24-byte header plus exactly one supported square image. Version 2 stores an
explicit count and sequential duration/pixel records, including rectangles.
See [FILE_FORMAT.md](FILE_FORMAT.md). The low-level version 1 writer is retained
for independent compatibility fixtures; application saves use version 2.

`project.c` captures into one near scratch image, preserving the checked exclusive
TMP creation, fwrite/flush/close checks, approved replacement, BAK rollback and
retained-recovery reporting. Failed reads/writes leave artwork, filename and saved
baseline intact. The codec's failed scratch contents are unspecified. There are
no per-frame filesystem operations or external format/export dependencies.

## Measured 16-bit memory budget

The 4096-byte stack, strict ordinary modules and extended-CRT file adapter remain.
Watcom uses `_fmalloc`/`_ffree` and declared CRT far-memory interfaces for seven
separate document blocks; hosts use malloc/free. Each block is 16,424 bytes in
DOS, below 32 KiB. All seven reserve 114,968 bytes outside DGROUP. The near active
canvas is 4096 bytes; fill queue 8192; near file scratch 16,424. No large automatic
images or large frame arrays live in DGROUP. Smaller documents retain this fixed
bounded reservation so editing/history do not allocate during an operation.

The final linked map reports DGROUP 38,432 bytes including the stack (27,104 bytes
below 64 KiB), near code 45,615 bytes, and total linked image 84,064 bytes.
`PIX86.EXE` is 50,430 bytes. Linked image plus reserved far blocks is 199,032 bytes;
PSP/environment, heap bookkeeping, stdio and loader overhead are additional.
Initialization on insufficient conventional memory fails with a diagnostic.
Up to 16 frames are allowed for every shape except 64×64 (four). There is no
transparency, onion skin, layers or export in this change. Native hardware and
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
