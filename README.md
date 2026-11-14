# Pix86

A lightweight, DOS-native pixel-art and frame-by-frame animation editor for game creators.

**Status:** M2 pixel drawing editor, targeting native 16-bit real-mode DOS.

Draw on a 16×16, 32×32 or 64×64 indexed canvas with pencil, eraser, four-connected
fill and eyedropper tools. Integer zoom, an optional grid, all 256 palette indices
and four undoable actions support small sprite work. Canvas pixels stay in
software memory. Animation, saving/loading and layers are not implemented.
There is no persistence: exiting discards the artwork.

## Build on Linux

Install [Open Watcom v2](https://github.com/open-watcom/open-watcom-v2) with its
Linux-hosted tools, DOS headers and 16-bit DOS libraries. The default location
is `$HOME/opt/watcom`; override it with `WATCOM` if needed.

```sh
export WATCOM="$HOME/opt/watcom"
./scripts/build.sh
file mnt/PIX86.EXE
```

The script checks `wcl`, `wcc` and `wlink`, builds with `-bt=dos -ms -0` (16-bit
DOS, small model, 8086 instructions), and places `PIX86.EXE` in `mnt/`. Compiler
warnings are errors. No DOS extender, DPMI, SDL or graphics library is required
by the application. Build intermediates and `PIX86.MAP` stay in `build/`.
`mnt/` and `build/` are ignored by Git. DOS-facing source and executable names
fit 8.3; host scripts and documentation run on Linux.

## Run in DOSBox-X

From the repository root:

```sh
dosbox-x -defaultconf -c "mount c ./mnt" -c "c:" -c "PIX86.EXE"
```

Alternatively, mount `mnt/` in an existing DOSBox-X session, switch to `C:`,
and run `PIX86`. Use VGA emulation and launch from a DOS text mode. DOSBox-X may
require a click or Ctrl+F10 to capture the mouse; Ctrl+F10 releases it. On real
DOS, load a compatible INT 33h mouse driver first. Without a driver, the workspace
opens with an explanatory message and keyboard shortcuts/ESC still work.

Choose canvas size when launching (default: 32×32):

```text
PIX86 16
PIX86 32
PIX86 64
PIX86 32 /NOMOUSE
```

Size is fixed for the session. `/NOMOUSE` skips driver initialization for diagnosis.
Exit restores the prior text mode/page/cursor, clearing the prior screen. Copy
only `PIX86.EXE` to DOS; no runtime DLL or data assets are needed.

## Drawing controls

| Control | Action |
| --- | --- |
| Left toolbar or P / E / F / I | Pencil / eraser / fill / eyedropper |
| Left-drag on canvas | Draw or erase; one drag is one undo action |
| Left-click with fill | Fill four-connected matching pixels; one action |
| Left-click with eyedropper, or right-click canvas | Pick foreground index |
| Left-click palette | Select foreground (FG) |
| Right-click palette | Configure eraser/clear background (BG) |
| + / = and - | Increase/decrease integer zoom within workspace limits |
| G | Toggle pixel grid (visible at zoom 2 and above) |
| U / Ctrl+Z | Undo |
| R / Ctrl+Y | Redo |
| C | Clear to BG, undoable |
| ESC | Return to DOS; artwork is discarded |

All 256 indices appear in the right-hand 16×16 palette. The selected foreground
has a bright outline, numeric label and swatch; background has a separate label
and swatch. These selections do not alter existing pixels. Palette RGB editing
is outside M2. Canvas coordinates and zoom appear in the status bar; `--,--`
means the pointer is outside the canvas. `G` shows grid state; `U` and `R` show
available history counts. Grid lines belong to the view, never the artwork.

The canvas is centred, initially at the largest fitting zoom: 10× for 16×16,
5× for 32×32 and 2× for 64×64. Zoom 1× is available for every size. Drawing pauses
outside the canvas; returning while holding the button starts a new segment in
the same action, without drawing a bridge across the interface. Tool, zoom,
grid, clear or history shortcuts finish a held stroke; release before starting
a new one. Four actions are retained; older actions expire. Only actual pixel
edits clear redo, so colour selection, zoom and no-op tools preserve it.

## Tests and development notes

```sh
./scripts/test.sh                     # strict C89 host suites; requires cc
python3 tests/mzcheck.py mnt/PIX86.EXE  # executable container check
./scripts/dostest.sh                  # build EDTEST.EXE, portable suite for DOS
./scripts/doscheck.sh                 # build interactive restoration harness
```

Run `EDTEST > EDTEST.TXT` in DOSBox-X to exercise tools, coordinates, history and
gestures with Watcom's 16-bit integers. The larger mock-framebuffer suites are
host-only. For interactive cleanup checks, run `DOSCHECK`, then ESC. Also run
`DOSCHECK null 16` and `DOSCHECK iret 64` to exercise missing mouse vectors and
other canvas sizes. Reports go to `MOUSE.TXT`, `NULL.TXT` and `IRET.TXT` in `mnt/`.
The harness deliberately changes the driver vector temporarily; use an emulator.
These executables are development tools and are not needed to run Pix86.

See [actual test results and checklist](docs/TESTING.md) and
[architecture, memory budget and provenance](docs/ARCHITECTURE.md). Native
FreeDOS/MS-DOS and real VGA/mouse hardware checks remain outstanding.

## Goals

- Run on FreeDOS, MS-DOS and DOSBox.
- Implement the editor independently in C, targeting 16-bit DOS initially with Open Watcom.
- Make animation a first-class feature: frames, timeline, playback, duplication, and onion skinning.
- Provide an approachable mouse-driven pixel editor with VGA 256-colour palette tools.
- Export sprites and animation frames in useful formats for DOS game development.
- Keep drawing and animation logic separate from platform-specific video, input, and UI code.

These are planned features, not claims of current implementation.

## DOS engineering reference: Zine

[Zine](https://github.com/raygibbon/zine) is our existing 16-bit DOS C project. Use it as a practical reference for the Open Watcom build workflow, DOSBox-X testing, real-mode memory constraints, DOS-safe filenames, small C modules, and keeping platform-specific pieces isolated.

Zine is a static-site generator, **not** a graphics editor. Its Watt-32 networking and bundled third-party dependencies are not required for Pix86 and must not be imported merely because Zine uses them. Review licences and implementation suitability before any intentional reuse; by default write Pix86-specific code.

## References, not dependencies

[GrafX2](https://grafx2.chez.com/) and [Autodesk Animator / Animator Pro](https://github.com/AnimatorPro) may be studied **only as references** for user workflows, expected features, and broad design ideas.

**Pix86 must be independently implemented.** Do not copy, translate, adapt, import, or reuse any source code, assembly, algorithms expressed in source, artwork, icons, fonts, palettes, UI graphics, documentation text, or other assets from either project. Do not vendor their repositories or link against their code. Do not reproduce distinctive UI artwork or branding.

Instead, write original specifications describing observable behaviour and design our own implementation. For file-format interoperability, rely on independently published format specifications and write original parsers/encoders. Record any external references in documentation, including what was learned, without reproducing protected material.

See [AGENTS.md](AGENTS.md) for contributor and AI coding-agent rules.

## Initial development direction

- Language: portable C.
- Compiler: Open Watcom for DOS targets.
- First target: 16-bit VGA DOS.
- Licence: [MIT](LICENSE) for original Pix86 code. New dependencies require a separate licence and redistribution review.
