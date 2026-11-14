# M2 verification

Verified on 2026-11-14. Tests cover Linux-hosted compilation, portable behavior
and DOSBox-X's built-in DOS. Native FreeDOS/MS-DOS, real VGA hardware and other
mouse drivers remain untested. Saving/loading and animation are not implemented.

## Automated host checks

`./scripts/test.sh` builds three suites with strict C89, pedantic diagnostics,
all warnings enabled and warnings treated as errors:

- M1 canvas validation and exact cursor save/restore at all 64000 screen positions,
  overlapping moves, repeated hide and status redraw ordering.
- Pixel get/set, all 256 indices (including 0/255), rejected bounds, canvas clear
  and guard bytes around a maximum-size canvas.
- Every displayed subpixel of every canvas cell at every fitting zoom for
  16×16, 32×32 and 64×64; centring, all four boundaries, invalid zooms and unchanged
  artwork. All 256 palette hit-test indices and outside boundaries.
- Line connectivity to every endpoint on a 64×64 canvas, covering all slopes and
  octants; inclusive endpoints, no-op strokes, invalid endpoints and eraser/pick.
- Whole 4096-pixel fills from opposite corners, no-op fills, invalid starts,
  enclosed regions, diagonal disconnection and a winding one-cell corridor.
- History exhaustion, oldest-state expiry, complete undo/redo traversal, branch
  edits, no-op preservation of redo and undoable clear.
- Editor gestures: a multi-sample drag is one action, final release movement is
  included, held fill is one action, picking preserves redo, new edits discard
  redo, leaving/re-entering breaks interpolation, toolbar presses cannot paint,
  undo while held stops the stroke, palette selection leaves pixels unchanged,
  background selection, grid toggle and zoom preservation.
- Mock-framebuffer rendering at every zoom, all 256 palette cells, grid on/off,
  clearing old zoom bounds and partial stroke updates without changing neighbors.

The portable editor suite also passes with AddressSanitizer and
UndefinedBehaviorSanitizer (`cc -fsanitize=address,undefined` using the same
portable sources). Hardware-dependent VGA/input implementations are excluded
from host tests; renderer and cursor suites use clipped mock framebuffers.

## Open Watcom and DOS-side automated checks

`./scripts/build.sh` passes with the installed Open Watcom v2 Linux-hosted
16-bit toolchain, build dated 2026-04-04. All eleven application modules compile
with `-bt=dos -ms -0 -za -wx -w4 -os`, zero warnings/errors. The link map uses
only Watcom's `lib286/dos/clibs.lib`, with no extender or DPMI startup.

- `mnt/PIX86.EXE`: 17448 bytes; `file` identifies an MS-DOS MZ executable.
- `python3 tests/mzcheck.py`: valid plain MZ, four relocations, valid entry/stack,
  no extended executable header. MZ inspection is a container check, supported
  by the explicit compiler target and real-mode execution tests.
- Link map: 54688 bytes of linked code/data/stack, including a 4096-byte stack.
  DGROUP is 0x9560 = 38240 bytes, below the 65536-byte segment limit. These values
  exclude PSP/environment and other DOS/runtime allocation overhead.
- `./scripts/dostest.sh` builds `EDTEST.EXE` from the same portable editor suite.
  It was executed in DOSBox-X with output redirected to `EDTEST.TXT` and reported
  `PASS: M2 pixels, zoom, lines, fill, history, palette and editor gestures.`
  This exercises actual 16-bit `int`/`unsigned` arithmetic, not only host integers.

The two larger framebuffer suites are host-only; their mock 64000-byte arrays
are not part of the DOS application. DOS-side tests run with a normal 386 CPU
core at maximum cycles; the compiled executable still uses 16-bit/8086 code.
The application also launched/rendered under the experimental 8086 emulator core.

## Interactive DOSBox-X checks

DOSBox-X 2025.02.01 SDL2 (Debian package 2025.02.01+dfsg-3), VGA-only emulation,
normal CPU core, 386 CPU, fixed 40000 cycles, 4 MiB configured memory. No guest
OS boot, DOS extender or DPMI host was used. Toolchain/emulator dependencies are
installed externally or extracted under `/tmp`, never vendored into Pix86.

Mouse/keyboard input and screenshots were checked using X11 automation. The
initial desktop macro was unreliable due to input/timing interactions, so final
workflow checks use an isolated Xvfb display with `autolock=false` and
`mouse_emulation=never`. This is test configuration, not an application dependency.
The [DOSBox-X mouse guide](https://dosbox-x.com/wiki/Guide%3AMouse-support-in-DOSBox%E2%80%90X)
describes those emulator options.

The interface renders the centred canvas/grid, four selectable tools, 256-cell
palette, foreground/background indicators and canvas-coordinate/zoom/history
status. The isolated screenshot pixel checks passed for a continuous pencil
drag, undo/redo, configuring BG and erasing the entire drag, a closed outline
with a differently coloured fill contained inside, picking that colour,
fill undo/redo, zooming the drawing without changing its pixels, and grid toggle.
Drawing strokes, undo/redo, fill and eraser were also visually inspected on the
desktop. The repeatable checklist below covers the full intended
interactive acceptance sequence; passing portable tests do not substitute for
native DOS/hardware checks.

The DOS restoration harness validates return code, prior mode/page and text cursor
state after interactive ESC. Its optional third argument selects canvas size:
`DOSCHECK mouse 32`, `DOSCHECK null 16`, `DOSCHECK iret 64`. The null and IRET cases
temporarily replace INT 33h and should be run only in an emulator. The driver's
vector is restored immediately after the child exits. Cursor position includes
the application's final console message (row 6 at 80 columns; row 7 at 40).

| Executed case | Result after ESC |
| --- | --- |
| Working driver, 32×32 | PASS, exit 0, mode 3, page 1, cursor 0600, shape 0607 |
| Null driver vector, 16×16 | PASS, exit 0, mode 1, page 0, cursor 0700, shape 0607 |
| IRET driver stub, 64×64 | PASS, exit 0, mode 3, page 1, cursor 0600, shape 0607 |

The 16×16 and 64×64 workspaces were visually checked at their initial 10× and
2× zooms, with a missing-driver message and no pointer. `PIX86 64 /NOMOUSE`
also rendered the fallback workspace and exited with ESC.

## Repeating interactive acceptance checks

1. Build, launch `PIX86 32`, and check palette, toolbar, FG/BG, zoom and grid.
2. Choose colours at both ends of the palette. Draw a fast pencil drag; check
   continuity and exactly one undo entry. Undo and redo it.
3. Right-click a palette cell to set BG; erase a drag. Undo and redo it. Clear
   with C and undo the clear.
4. Draw a closed outline, choose another colour and fill inside. Check the
   boundary and outside pixels; undo/redo the fill. Pick a colour with I and
   with right-click. Hold fill to check it does not repeat actions.
5. Zoom down/up and toggle G; check the artwork stays identical and mapping
   selects the intended cells at every edge. Zoom 1 has no grid lines.
6. Make more than four actions, undo to the bound, redo, then undo and draw a
   new edit. Check redo is discarded. Changing FG/BG or zoom should preserve it.
7. Leave the canvas during a held stroke; return and release. Check there is
   no bridge across the interface and no paint from a press starting in the UI.
8. Repeat with `PIX86 16` and `PIX86 64`. Test tool keys P/E/F/I, U/R and
   Ctrl+Z/Ctrl+Y, extended keys, missing mouse drivers and `/NOMOUSE`.
9. ESC should return promptly to readable DOS text. Run all harness cases and
   inspect `MOUSE.TXT`, `NULL.TXT` and `IRET.TXT` on the host.
10. Repeat on native FreeDOS/MS-DOS and real VGA/mouse hardware (outstanding).

Known limits: canvas size is chosen at launch, maximum zoom is constrained by
320×200, grid lines consume one displayed pixel per cell, history retains four
actions, and polling can miss a complete click that occurs between samples.
No artwork survives exit; there are no file operations or animation features.
