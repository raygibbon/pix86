# Pix86 agent and contributor instructions

## Project

Pix86 is an original pixel-art and frame-based animation editor, planned for DOS with 16-bit C/Open Watcom as the first target.

## First-party DOS project reference

- [Zine](https://github.com/raygibbon/zine) is an existing 16-bit DOS C/Open Watcom project to consult for build scripts, DOSBox-X testing, memory and filename constraints, and modular structure.
- Treat Zine as an engineering reference rather than a Pix86 dependency. Prefer original Pix86 implementations. Any deliberate reuse of Zine code must be separately assessed for relevance and licensing and explicitly documented.
- Do not bring over Zine's Watt-32 networking, bundled third-party code, or unrelated site-generator functionality.

## TDE user-interface reference

- [TDE source download](http://adoxa.altervista.org/tde/dl.php?f=source) can be consulted for DOS UI concepts (menus, dialogs, shortcuts, status lines, keyboard navigation).
- Treat it as a functional and engineering reference; implement an original graphical user interface for Pix86. Do not copy TDE source or assets as part of this reference-only workflow.
- The local M2.5 behaviour review is recorded in `docs/UI_DESIGN.md`. TDE and
  LibreSprite remain reference-only: do not copy, port, translate or adapt their
  code, structures, layouts or assets.
- The public-domain description is not a substitute for reviewing the actual provenance and notices of any third-party material before intentional reuse.

## Non-negotiable clean-room policy

- GrafX2 and Autodesk Animator / Animator Pro are **reference-only** projects.
- Do **not** copy, port, translate, modify, adapt, embed, vendor, or link their code (including snippets, assembly, shaders, data tables, and generated output derived from their implementations).
- Do **not** import or reproduce their icons, fonts, artwork, palettes, UI layouts pixel-for-pixel, branding, text, or other assets.
- Never ask an AI coding agent to reproduce or transform their code. Avoid pasting reference source into implementation prompts or issues.
- Study observable behaviours and user-facing workflows at a functional level, then write an original specification and independently implement it.
- For compatibility with known file formats, prefer independently published specifications, original test fixtures, and independent code. Track the provenance of any external dependency or fixture.
- Record the source of factual reference material, and distinguish ideas/specifications from third-party source code.
- If a proposed change risks importing protected work or conflicts with licence terms, stop and flag it for review.

## Engineering principles

- Keep the original core (canvas, animation document, frame operations, palette model) independent from DOS-specific video and input drivers.
- Keep the UI simple: animation is central, not an add-on.
- Target FreeDOS/MS-DOS/DOSBox and VGA 256-colour drawing first; consider future 32-bit or native ports only when justified.
- Prefer small modules, ANSI C-compatible interfaces where practical, straightforward tests, and documented memory constraints for 16-bit DOS.
- Avoid adding external dependencies without explicitly reviewing licences and redistribution requirements.
- Do not assert features work without compiling and testing them.

## Documentation

Update the README and development notes when project scope changes. Document original architecture and decisions; cite reference software by name or URL without incorporating its implementation.

## Interface and verification

- Keep portable menu/key interpretation and filename editing separate from DOS
  polling, VGA painting and file-format code. Do not add a widget framework.
- Route overlays before editor input. Consume dismissal clicks and wait for
  button release before resuming canvas strokes. Protect the cursor only when repaint bounds intersect it.
- Run `scripts/test.sh`, `scripts/build.sh` and `scripts/dostest.sh`; execute
  EDTEST, IOTEST, UITEST and KBDTEST in DOSBox-X. IOTEST requires a disposable directory.
- Keep menu/dialog source names DOS 8.3, buffers bounded and DGROUP below 64 KiB.

- Interface text uses original 6x8 editor and 4x6 menu/dialog glyphs, never
  a copied or scaled reference font. Share font metrics and hit geometry in `layout.h` when changing controls.
- Keep Ctrl+\ menu activation in BIOS decoding/keymap, including scan codes;
  preserve plain and AltGr path separators. Record tested layouts and limits.
- UI theme indices must not change the fixed artwork DAC palette.

- Keep shortcut aliases in keymap.c and route overlays through ui.c before
  application commands. Keep logical commands independent from file formats.
- No ordinary menu/modal full-screen clears or polling-driven paints. Preserve
  testpaint's idle-write, exact restoration and intercepted-Escape regressions.
- Working-palette slots contain VGA indices, never colour remapping rules. Keep
  all 256 indices accessible through the popup and preserve left/right FG/BG.
- Grid styling is view-only. Verify dark/light artwork and native 320x200 font
  readability when changing glyphs, palette geometry or the grid painter.

- Keep menu prefix letters and hit/row geometry derived from command rows.
  Separator rows cannot be selected or executed. Preserve per-heading row memory.
