# M1 architecture

M1 is a native 16-bit real-mode DOS foundation. There is one 128x128 indexed
canvas and an original, static editor workspace. Painting, animation operations,
file formats, palette editing and additional platforms are outside M1. The frame
label reserves a place for animation context; it is not an animation document.

## Modules

- `canvas.c`: portable ANSI C, dimensions and caller-owned byte storage. No DOS
  headers, video addresses or palette RGB values. Initialization rejects zero
  dimensions, insufficient storage and objects larger than 32767 bytes. The
  caller must keep the initialized dimensions and storage valid.
- `video.c`: all VGA memory access, video BIOS calls and DAC port I/O. Enters
  320x200 Mode 13h, checks the BIOS mode, and clips pixel/rectangle operations.
  Screen offsets use unsigned 16-bit arithmetic (maximum 63999). Rectangle
  endpoints use long arithmetic before clipping. Text uses the machine BIOS's
  8x8 font pointer; there is no bundled font or font dependency.
- `mouse.c`: INT 33h polling only. Checks for a null vector and an IRET stub
  before driver reset. Reset detects availability and leaves the driver cursor
  hidden. Sets a 0..639 horizontal virtual range and divides X by two for
  Mode 13h pixels; vertical range is 0..199. Left/right bits are exposed as
  portable state. Shutdown resets an initialized driver before text restoration.
- `keyboard.c`: nonblocking DOS console polling, consumes extended-key pairs.
- `cursor.c`: portable saved-pixel cursor using only the video interface. Saves
  64 bytes for an original 8x8 pointer. Hide before modifying the status bar or
  moving the pointer, then show after drawing; video clipping handles edges.
- `main.c`: owns the canvas allocation, draws the workspace and controls the
  polling loop. Redraws only the status bar and cursor on input changes. ESC
  hides the cursor, resets the mouse, restores video and frees storage.

The toolbar and palette swatches are placeholders. Mouse coordinates describe
screen pixels; the canvas occupies x=112..239, y=32..159. The bottom bar shows
left/right button states. `/NOMOUSE` is an explicit diagnostic option, also useful
for running with a problematic driver; automatic detection remains the default.

## Real-mode memory and cleanup

Open Watcom `wcl -bt=dos -ms -0` selects the 16-bit DOS target, small model and
8086 code generation. Near code/data each stay within a 64 KiB segment. Only the
VGA and BIOS pointers and interrupt-vector inspection need far pointers.
No extender, DPMI, graphics framework or external library is linked beyond the
Watcom DOS C runtime. Source files and DOS executables use 8.3-compatible names;
Linux-only documentation filenames need not be copied into DOS.

The canvas uses 16384 heap bytes. There is no 64000-byte shadow screen and no
per-widget buffer. The cursor uses 64 static bytes; the status buffer is 41 stack
bytes. The linker reserves a 4096-byte stack. The initial build's map reports
17968 bytes of linked code/data/stack, excluding the canvas heap, PSP/environment
and runtime heap bookkeeping. This is a footprint observation, not a claim about
the minimum DOS memory required. Allocation failure occurs before changing video.
The small model deliberately limits future canvas growth; M2 must reassess memory
rather than silently growing near objects past segment limits.

Launch from a DOS text mode (0..3 or 7) on VGA hardware or VGA emulation.
The previous BIOS mode, active page, text cursor position and shape are saved.
Mode restoration clears the old text contents; it does not restore the prior
screen contents or custom fonts/palette. Normal ESC and failed mode entry restore
video. Abrupt process termination and emulator closure are outside this cleanup
contract. There are no installed interrupt handlers.

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
Neither their source nor their assets were consulted or incorporated for M1.
The workspace, pointer, swatches and module implementations are original.
