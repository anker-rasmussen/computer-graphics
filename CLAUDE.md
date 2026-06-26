# CLAUDE.md — computer-graphics

## Tech stack

C++ / GLSL / OpenGL coursework. Build target is **Visual Studio 2019/2022
on Windows** via the supplied `.vcxproj` files; the development machine is
Linux but the project ships and is graded on MSVC. Win32 API is used for
the window/context (no SDL/GLFW abstraction beyond GLFW for input). Third-
party libs in tree: Assimp, FMOD, GLEW, GLFW, FreeImage, FreeType.

The writeup is LaTeX, built with `pdflatex` from
`Coursework/writeup-final/`. Bibliography is biber.

## Toolchain rules — overrides

The global `~/CLAUDE.md` mentions `cargo clippy` / `cargo test` / `cargo
fmt`. **None of those apply here.** This is not a Rust project. Do not
run cargo commands, do not suggest Rust idioms, and do not check
`Cargo.toml`. The Veilid / MP-SPDZ / EE8 architecture constraints in
that file are also irrelevant — they belong to a different repository.

What does apply from the global file:
- No Co-Authored-By trailers on commits.
- Group changes into logical, semantic commits.
- Identify root cause before patching; don't add fallback mechanisms
  unless asked.
- Run cheap subagents (Haiku/Sonnet) for codebase exploration; only the
  main loop or Plan subagents need Opus.

## Project layout

- `Coursework/Template2026/OpenGLTemplate/` — C++ source, shaders, and
  resources. Shaders live in `resources/shaders/`. Main loop is
  `Game.cpp`; tactical-phase logic is `TacticalGame.cpp`.
- `Coursework/Template2026/build/` — local CMake/clang artefacts the user
  uses for syntax-checking on Linux. Not the submission build.
- `Coursework/writeup-final/` — current submission writeup (`main.tex`,
  `main.pdf`).
- `Coursework/writeup/` — older interim-coursework writeup. Don't edit
  unless explicitly asked; it's the wrong target by default.
- `Coursework/IN3005_2026.GraphicsCoursework.pdf` — the marking spec.
- `Coursework/archive/` — old captures. Don't depend on.

## Submission constraints

Moodle ZIP cap is **200 MB**. Total project assets currently exceed this;
slimming `resources/models/` (especially `Mieli/`, `bridge.glb`, and the
non-base-color PBR maps under `MonitoringStation/textures/`) is part of
prepping for submission. Anything not loaded by `*.cpp` is fair game to
exclude from the submission bundle (but don't delete from the working
tree without confirmation).

## Diagnostics noise to ignore

Clang's `pp_file_not_found` and "incomplete type" errors on
`CatmullRom.h` are pre-existing — clang doesn't see the MSVC project's
include paths for the `CatmullRom/` subfolder. The MSVC build is fine.
Don't try to "fix" these.

## Personal context

User is doing this for a 75%-weighted INM376 Computer Graphics
coursework. They prefer terse iteration: implement and rebuild, no
over-explanation. Quantum-Thief themed game (Perhonen ship, Mieli, Jean
le Flambeur, Sobornost ambush) — keep that voice.
