# AGENTS.md

## Scope
- Applies to the entire repository.

## Quick Start (Read First)
- `README.md`
- Other `AGENTS.md` files in subdirectories

## Hard Rules
- CRITICAL: Follow `.editorconfig`, including SAL annotations conventions.
- CRITICAL: Preserve original file encoding (usually UTF8 or UTF8-BOM) and line-ending style (usually CRLF) when editing files.
- CRITICAL: Keep diffs minimal when you touch Visual Studio project files (*.sln, *.slnx, *.vcxproj, *.props, *.targets, ...).
- Keep diffs minimal; do not refactor unrelated code.
- Use concise, technical comments only when needed.

## Rules
- Some files are auto-generated and usually end with `.g.*` (for example, `I18N.xml.g.c` and `I18N.xml.g.h`); do not modify them manually.
- The output directory is usually named `OutDir` and is located next to the solution file; the exact path depends on `.props` files and project settings.

## Tool
- You can use Visual Studio and the Windows SDK when needed.

## Build

- Use `msbuild` to build the entire solution (`*.sln`, `*.slnx`).
