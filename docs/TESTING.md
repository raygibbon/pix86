# M1 verification

Verified on 2026-11-14. These results cover the local build and DOSBox-X's
built-in DOS environment. They do not claim execution on native FreeDOS,
MS-DOS, real VGA hardware or every mouse driver.

## Automated checks

- `scripts/build.sh`: Open Watcom v2 Linux-hosted 16-bit compiler/linker,
  toolchain build dated 2026-04-04. All six application modules compiled with
  `-bt=dos -ms -0 -za -wx -w4 -os`; zero warnings/errors. Only the DOS `lib286`
  runtime appears in the link map. Stack: 4096 bytes. Output: 13550 bytes.
  Setting `WATCOM` to a nonexistent installation returned a nonzero status
  and a useful `Missing wcl` diagnostic.
- `file mnt/PIX86.EXE`: `MS-DOS executable, MZ for MS-DOS`.
- `python3 tests/mzcheck.py mnt/PIX86.EXE`: valid plain MZ container, four
  relocations, valid entry/stack, no extended executable header. Container
  inspection alone does not establish CPU bitness; the compiler target, link
  map and successful 8086-emulated execution provide additional evidence.
- `scripts/test.sh`: strict C89 host compilation, canvas capacity and bounds,
  invalid/overflow dimensions, guard bytes, cursor save/restore at all 64000
  screen positions, overlapping moves, repeated hide and status redraw ordering.
  Tests link the real canvas/cursor code to a mock clipped framebuffer; they
  do not test VGA or DOS interrupts.

## DOSBox-X runtime checks

DOSBox-X 2025.02.01 SDL2 (Debian package 2025.02.01+dfsg-3), VGA-only machine,
normal CPU core, `cputype=8086`, fixed 12000 cycles, 4 MiB configured memory.
DOSBox-X labels its 8086 core experimental. No extender or DPMI host was loaded.
The emulator and its missing host dependency were extracted under `/tmp`;
no emulator code/libraries were added to Pix86.

Interactive input was supplied through X11 automation and rendered windows were
visually inspected. The DOS harness automatically checks the child's exit code
and restored BIOS state after each interactive ESC:

| Harness case | Observed result |
| --- | --- |
| `DOSCHECK` | PASS, exit 0, mode 3, page 1, cursor 0600, shape 0607 |
| `DOSCHECK null` | PASS, exit 0, mode 1, page 0, cursor 0700, shape 0607 |
| `DOSCHECK iret` | PASS, exit 0, mode 3, page 1, cursor 0600, shape 0607 |

The harness starts at row 5, column 10. The application's exit message advances
the restored cursor to row 6 (80 columns), or row 7 (40 columns, due to wrapping).
It restores the mouse vector immediately after the child exits. Run this
harness in an emulator; it deliberately changes the driver vector temporarily.
It is separate from `PIX86.EXE` and uses Watcom extensions for child spawning.

Visual checks: title, toolbar placeholders, canvas, palette and status rendered
correctly; the BIOS font was legible. Coordinates changed with motion and
reached X=319/Y=199 at the lower-right edge. The left-button indicator changed
while held; the right-button indicator was also verified with mouse capture.
Arrow and F1 input left the application running. The pointer remained visible
and movement left no visible trails.
Both absent-driver cases displayed `NO MOUSE DRIVER - ESC TO EXIT` without a
pointer, and ESC restored text mode. The `/NOMOUSE` option also displayed the
fallback workspace and exited with ESC. Automated framebuffer tests cover cursor
restoration more exhaustively than visual inspection.

## Repeating interactive acceptance checks

1. Build, launch `PIX86` in DOSBox-X, and capture the mouse if needed.
2. Check title/workspace/toolbar/palette/status and a visible pointer. Move over
   the canvas, palette and status bar, then all four edges. Check no trails and
   coordinates within X=0..319, Y=0..199.
3. Hold left, right and both buttons. Check `L:1`, `R:1`, and both respectively;
   release and check both return to zero. No painting should occur.
4. Press extended keys (arrows/F1); the loop should continue. ESC should exit
   promptly, leave readable DOS text and return an exit code of zero.
5. Build the harness and run its three cases above. Inspect the reports on the
   host. Also try `PIX86 /NOMOUSE` for the explicit fallback option.
6. Repeat on actual FreeDOS and MS-DOS with real mouse drivers/VGA hardware.
   These native-system checks remain outstanding.

Known limits: one 128x128 canvas, static placeholder controls, BIOS 8x8 ASCII
labels, conventional near-memory canvas storage, normal ESC cleanup only, and
no painting, animation, formats, VESA or alternative platforms.
