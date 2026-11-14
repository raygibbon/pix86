# Pix86 agent and contributor instructions

## Project

Pix86 is an original pixel-art and frame-based animation editor, planned for DOS with 16-bit C/Open Watcom as the first target.

## First-party DOS project reference

- [Zine](https://github.com/raygibbon/zine) is an existing 16-bit DOS C/Open Watcom project to consult for build scripts, DOSBox-X testing, memory and filename constraints, and modular structure.
- Treat Zine as an engineering reference rather than a Pix86 dependency. Prefer original Pix86 implementations. Any deliberate reuse of Zine code must be separately assessed for relevance and licensing and explicitly documented.
- Do not bring over Zine's Watt-32 networking, bundled third-party code, or unrelated site-generator functionality.

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
