# Pix86

A lightweight, DOS-native pixel-art and frame-by-frame animation editor for game creators.

**Status:** M2.5 pixel editor with compact 6x8 text, keyboard/mouse menus, file dialogs
and native .P86 saving/loading, targeting
16-bit real-mode DOS.

Draw on a 16×16, 32×32 or 64×64 indexed canvas with pencil, eraser, four-connected
fill and eyedropper tools. Integer zoom, an optional grid, all 256 palette indices
and four undoable actions support small sprite work. Canvas pixels stay in
software memory. Save and reopen native `.P86` projects, including canvas size
and foreground/background selections. Animation and layers are not implemented.

![Compact Pix86 workspace in DOSBox-X at 2x display size](docs/UI.PNG)

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

Choose another size with New or by opening a project. `/NOMOUSE` skips driver
initialization for diagnosis.
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
| Ctrl+\ (primary), F10 / Alt+F,E,V,R,O,H | Menu bar / File, Edit, View, Frame, Options, Help |
| Arrows / Enter / Esc (menu open) | Navigate / execute / close menu |
| F2 | Save (asks for a name on an untitled canvas) |
| Shift+F2 | Save As |
| F3 | Open a .P86 project |
| N | New canvas; select 16×16, 32×32 or 64×64 |
| ESC | Exit; confirm before discarding unsaved changes |

All 256 indices appear in the right-hand 16×16 palette. The selected foreground
has a bright outline, numeric label and swatch; background has a separate label
and swatch. These selections do not alter existing pixels. Palette RGB editing
is outside M2. The narrow toolbar and 64x96 palette leave a 222x176 drawing
region. Text uses an original crisp 6x8 bitmap font; UI colours use existing
palette entries so saved artwork retains its colours. The single compact status
line shows dimensions, coordinates, tool, FG/BG, zoom,
grid, modified state and history; `X--Y--`
means the pointer is outside the canvas. `G` shows grid state; `U` and `R` show
available history counts. Grid lines belong to the view, never the artwork.

The canvas is centred, initially at the largest fitting zoom: 11× for 16×16,
5× for 32×32 and 2× for 64×64. Zoom 1× is available for every size. Drawing pauses
outside the canvas; returning while holding the button starts a new segment in
the same action, without drawing a bridge across the interface. Tool, zoom,
grid, clear or history shortcuts finish a held stroke; release before starting
a new one. Four actions are retained; older actions expire. Only actual pixel
edits clear redo, so colour selection, zoom and no-op tools preserve it.

## Files and unsaved changes

Press F2, type `PLAYER` and press Enter to save `PLAYER.P86`. The filename field
starts selected: typing or Backspace replaces the prefill. Left/Right/Home/End
move the cursor; Backspace/Delete edit around it. Enter accepts, Esc cancels.
Tab/Shift+Tab move between the field and OK/Cancel; click controls or the field
to operate them with the mouse. Long paths scroll within the field. Save As
always opens this dialog. There is no graphical file browser.

Use DOS 8.3 filenames and directory components, for example `C:\ART\PLAYER.P86`
or `..\ART\PLAYER`. The default extension is `.P86`. Paths are limited to 79
characters, and the directory prefix must also leave room for an 8.3 temporary
name. Spaces, wildcards, device names (CON, NUL, COM1, etc.) and long filenames
are rejected. Existing directories must already exist; drive-relative and rooted
DOS paths are supported. File I/O uses uppercase canonical names.

Saving asks before overwriting any existing file, including the current project.
The old file stays intact until the new temporary file has been written, flushed
and closed. Replacement keeps a backup until installation succeeds. Errors leave
the canvas unsaved; a rollback failure displays retained BAK/TMP paths for manual
recovery. See [replacement guarantees and format](docs/FILE_FORMAT.md).

An asterisk at the start of the status line means the project differs from the
saved state.
This compares actual pixels, dimensions and saved FG/BG selections, independently
of undo-history position. Choosing another FG/BG therefore also marks a project
modified. Undo can return to the saved artwork, including after history expiry;
view changes alone do not affect it. Successful loading resets undo/redo and
restores the file's size and FG/BG. Invalid or unreadable files leave live artwork
and the current filename unchanged.

New, Open and Exit ask before discarding unsaved changes. Y or Yes permits
discarding; N, Esc or Cancel abandons the operation. Cancel is initially selected;
Tab/arrows and Enter operate the buttons. A cancelled or failed Open retains the
current artwork. New starts an unnamed blank canvas. Saving/loading messages
are acknowledged with any key. Menus display file/drawing shortcuts. Ctrl+\ opens File and closes an open
menu when pressed again. F10 and Alt+F/E/V/R/O/H are alternatives;
Left/Right switches headings, Up/Down selects items, Enter executes and Esc
closes. Frame (Alt+R) contains disabled future commands. Click a heading/item
to use menus with the mouse; outside clicks close and are consumed. Release
held buttons after closing a menu/dialog before drawing. See
[interface design and reference review](docs/UI_DESIGN.md).

## Tests and development notes

```sh
./scripts/test.sh                     # strict C89 host suites; requires cc
python3 tests/mzcheck.py mnt/PIX86.EXE  # executable container check
./scripts/dostest.sh                  # build EDTEST.EXE, IOTEST.EXE UITEST.EXE and KBDTEST.EXE for DOS
./scripts/doscheck.sh                 # build interactive restoration harness
```

Run `EDTEST > EDTEST.TXT` in DOSBox-X to exercise tools, coordinates, history and
gestures with Watcom's 16-bit integers. The larger mock-framebuffer suites are
host-only. Run `IOTEST > RESULT.TXT` in a separate scratch directory to test native
file roundtrips, malformed files, dirty tracking, dialogs and injected write,
flush, close, rename and rollback failures. It creates/removes test fixtures;
run it only in a disposable directory. The host test script uses `build/iotest/`. UITEST exercises menu navigation,
mouse hit testing, real dialog loops with scripted input, cursor editing and
drawing handoff; it uses mock video/input and performs no hardware checks. For interactive cleanup checks, run
`DOSCHECK`, then ESC. KBDTEST injects BIOS words to check the real keyboard
module; `KBDTEST /LOG` records raw physical keystrokes to KEYS.TXT until Escape. Also run
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

## DOS interface reference: TDE

[TDE (Thomson-Davis Editor)](http://adoxa.altervista.org/tde/dl.php?f=source) is a reference for the look and operation of compact DOS interfaces: menu bars, keyboard-first navigation, dialogs, status bars, and consistency between mouse and keyboard controls. Its software is described as public domain, but Pix86 should not incorporate TDE source or assets without a separate explicit review. Develop original VGA UI code suited to a graphics editor, rather than porting TDE's text-mode UI.

## References, not dependencies

[GrafX2](https://grafx2.chez.com/) and [Autodesk Animator / Animator Pro](https://github.com/AnimatorPro) may be studied **only as references** for user workflows, expected features, and broad design ideas.

**Pix86 must be independently implemented.** Do not copy, translate, adapt, import, or reuse any source code, assembly, algorithms expressed in source, artwork, icons, fonts, palettes, UI graphics, documentation text, or other assets from either project. Do not vendor their repositories or link against their code. Do not reproduce distinctive UI artwork or branding.

Instead, write original specifications describing observable behaviour and design our own implementation. For file-format interoperability, rely on independently published format specifications and write original parsers/encoders. Record any external references in documentation, including what was learned, without reproducing protected material.

LibreSprite and TDE are also workflow/UI references only; no source or assets
were imported for M2.5. The limited local TDE behaviour review and original
Pix86 design are recorded in [UI_DESIGN.md](docs/UI_DESIGN.md).

See [AGENTS.md](AGENTS.md) for contributor and AI coding-agent rules.

## Initial development direction

- Language: portable C.
- Compiler: Open Watcom for DOS targets.
- First target: 16-bit VGA DOS.
- Licence: [MIT](LICENSE) for original Pix86 code. New dependencies require a separate licence and redistribution review.
