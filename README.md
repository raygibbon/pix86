# Pix86

A lightweight, DOS-native pixel-art and frame-by-frame animation editor for game creators.

**Status:** M1 DOS foundation implemented and tested in DOSBox-X. Native
FreeDOS/MS-DOS hardware testing remains outstanding.

M1 displays an original 320x200 VGA workspace with a 128x128 software canvas,
placeholder toolbar and palette, mouse cursor and coordinate/button status.
ESC returns to DOS text mode. Painting and animation are planned; the toolbar,
swatches and frame label are not functional editing controls.

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
and run `PIX86`. Use VGA emulation and launch from a DOS text mode. Move the
mouse to update screen coordinates, hold buttons to inspect `L`/`R`, and press
ESC to exit. DOSBox-X may require a click or Ctrl+F10 to capture the mouse;
Ctrl+F10 releases it. On real DOS, load a compatible INT 33h mouse driver first.
Without a driver the workspace still opens, shows a message, and ESC works.
`PIX86 /NOMOUSE` explicitly disables mouse initialization for diagnosis.

Exit restores the prior text mode/page/cursor, clearing the prior screen.
Copy only `PIX86.EXE` to a DOS machine; no runtime DLL or data assets are needed.

## Tests and development notes

```sh
./scripts/test.sh                    # C89 host tests; requires a C compiler
python3 tests/mzcheck.py mnt/PIX86.EXE # executable container check
./scripts/doscheck.sh                # build optional DOS integration harness
```

In DOSBox-X, run `DOSCHECK`, exercise input, then press ESC. Run `DOSCHECK null`
and `DOSCHECK iret` to exercise genuinely missing driver vectors. Each requires
ESC and writes a PASS/FAIL report to `MOUSE.TXT`, `NULL.TXT` or `IRET.TXT` in
`mnt/`. These harness files are development tools, not part of the application.
See [test results and manual checklist](docs/TESTING.md) and
[architecture, memory limits and provenance](docs/ARCHITECTURE.md).

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
