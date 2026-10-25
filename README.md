# SinkingStarSketchPad

An in-game sketchpad for **Order of the Sinking Star**: draw ideas, add text notes, and plan routes snapped to cell centers. This is a native 64-bit ReShade add-on rendered inside the game's DirectX 12 frame, with English and Simplified Chinese interfaces.

**Source repository:** [github.com/ouyangzhixuan/SinkingStarSketchPad](https://github.com/ouyangzhixuan/SinkingStarSketchPad)

[English usage guide](docs/README_EN.md) · [中文使用说明](docs/README_zh-CN.md) · [报告问题 / Issues](https://github.com/ouyangzhixuan/SinkingStarSketchPad/issues)

## Features

- Experimental free camera: pan, rise/lower, orbit/tilt, lens zoom and restore, sharing the world-path projection.
- Crystal-blade pointer, star drawing crosshair, and rune-ring eraser.
- Pen, lines, arrows, rectangles, ellipses, text, and whole-mark erasing.
- Three automatically saved scratch pages, undo/redo, and SVG export.
- Each level initializes the target plane once from the controlled character logical Z, with a button to set it again.
- [Ground marking](docs/MANUAL_GROUND.md): character-plane and manual-marking buttons share one panel; middle-click marks the ground under the pointer once, with native wall occlusion.
- World-space paths and optional grid drawing: plane pen/shapes and camera-following text anchors.
- Gray grid previews, camera-occluded plane hints and configurable wheel/Shift/Ctrl/Alt actions.
- A live Settings window with persistent hint layers, duration and wheel bindings.
- Cross-height grid lines/arrows and three-axis orthogonal paths, stored in backward-readable draft v4.
- 17×17 white guides with black outlines, drawn below ink; plane hints default to 15% opacity.
- Cross-height Shift lines/arrows lock world X/Y for vertical segments; voxel clipping and draft v4 remain.
- Right click cancels shapes or finishes paths first, otherwise temporarily erases; Ctrl+Z undoes.
- Persistent System / English / Simplified Chinese language selection.

**Beta:** the user confirmed native previews and ground marking in the levels they checked. The user reported the preceding grid drawing extension works well. This settings/preview revision has received static checks and compilation only. Inspect alignment in your level; routes do not check obstacles, solve puzzles, or control the player. See [grid drawing, previews and draft upgrades](docs/GRID_DRAWING.md).

## Use

For a prebuilt manual release, exit the game and copy the contents of `game_files` beside `sinking_star.exe`, following `INSTALL_EN.txt`. For upgrades, preserve existing settings and drafts and set `[INPUT] InputProcessing=1` in `ReShade.ini`. No installation script is required. Do not copy a GitHub source ZIP into the game as if it were a prebuilt mod.

Press **E** to open/close the toolbar, **G** for cursor visibility, and **B** for mark visibility. **E → Alt+H** selects cell paths. Keyboard movement and camera controls remain available while drawing. Alt+letter selects a tool; Alt+1-5 selects ink. Alt+Tab keeps drawing mode, finishes actual drag strokes and cancels unfinished click shapes. Choose the interface language in **Language / 语言**. See the [full English guide](docs/README_EN.md) for installation, upgrades, existing ReShade setups, controls, and uninstalling.

The compatible add-on filename and persistent data folder remain `SinkingStarSketch.addon64` and `SinkingStarSketch-data` so existing drafts and preferences continue to work.

Version 3.4.6 integrates optional retained camera views with default speed 5 and Y inversion enabled; explicit Restore game view remains available.

Free camera: **T**, WASD pan, Q/Z up/down, right-drag orbit/tilt and wheel zoom with drawing closed; T/Esc leaves camera controls according to Keep view. With drawing open, wheel actions follow Settings; defaults are height changes and Shift+wheel color cycling. It defaults off and captures keyboard/mouse while active. This is a render-camera override; game-side culling, shadows, reflections and mouse picking may not follow large changes. The user confirmed actual-game camera movement with the 3.4.4 fix on 23 October 2026. This fixes the CPU-upload-to-GPU camera path missed by earlier builds; broader visual compatibility remains experimental.

## Build from source

Requirements: 64-bit Windows and an x86-64 **LLVM-MinGW** toolchain. The retained build was validated with `llvm-mingw-20261006-ucrt-x86_64`. The ReShade 6.8.0 / Dear ImGui 1.92.5 docking headers used by the add-on are included in `third_party/reshade-sdk` with their original notices.

```powershell
git clone https://github.com/ouyangzhixuan/SinkingStarSketchPad.git
cd SinkingStarSketchPad
.\build.ps1 -Compiler 'C:\path\to\llvm-mingw\bin\clang++.exe'
.\test.ps1 -Compiler 'C:\path\to\llvm-mingw\bin\clang++.exe'
```

`test.ps1` performs static compilation checks only; it does not execute test programs. Ground marking no longer observes or scores GPU instance uploads. Native terrain is refreshed on level entry and marking requests; character positions are read for initialization and the character-plane button. The duplicate backup camera collector is disabled, while effective-camera/free-camera GPU processing remains active.

The add-on is written to `build/SinkingStarSketch.addon64`. Do not substitute an arbitrary MSVC-targeting `clang++`: the ImGui ABI adapter is explicitly designed for the MinGW/MSVC boundary used by this project.

To test in game, exit the game, back up an existing setup, install the compatible **ReShade 6.8.0 full add-on support** runtime, then place the built add-on beside `sinking_star.exe`. Keep compatible ReShade settings and a suitable UI font. The source repository does not include runtime DLLs, font binaries, game files, or a compiler. See [third-party components](THIRD_PARTY_NOTICES.md) for their official sources.

The native free-camera integration test renders a checkerboard and verifies both GPU geometry and world-path projection before/during camera changes and after restore, including resize:

```powershell
.\tests\run_free_camera.ps1 -Compiler 'C:\path\to\llvm-mingw\bin\clang++.exe' -Runtime 'C:\deps\ReShade64.dll' -Font 'C:\deps\NotoSansCJKsc-Regular.otf'
```

Repeat with `-InputSuite` to exercise the production keyboard/mouse routing in a hidden test window, and with `-Cached` to verify movement and restore when the engine reuses camera constants without rewriting them. Use `-LateDescriptor -GpuUpload` to verify descriptors populated after binding and GPU Upload heaps (requires hardware support). These tests do not change desktop focus or generate system-wide input. They do not replace validation in an actual game level.

## Create a distributable package

After building, provide the tested ReShade runtime, the unmodified Noto Sans CJK SC Regular font and its license to the packaging script:

```powershell
python scripts/package.py --runtime 'C:\deps\ReShade64.dll' --font 'C:\deps\NotoSansCJKsc-Regular.otf' --font-license 'C:\deps\LICENSE-Noto.txt'
```

This creates a ZIP in `dist/` with bilingual manual-install/uninstall guides, checksums, configuration and notices, without installer scripts. It validates the external dependency hashes; it does not download dependencies. The package uses the source build output, not a binary branding patch. Verify a fresh package before distributing it. [Nexus publishing notes](docs/UPLOAD_GUIDE_zh-CN.md) and an [English page draft](docs/NEXUS_DESCRIPTION_EN.md) are included.

## Source layout

- `src/`: add-on, drawing model, camera observers, native ground readers, cursor artwork, and localization.
- `tests/`: model, grid/projection, localization tests, and a synthetic DX12 rendering host.
- `config/`: example ReShade configuration for packaged installs.
- `scripts/`: release packager.
- `docs/`: English/Chinese usage, validation scope, and publishing materials.
- `third_party/`: the header dependencies and original licenses.

The DX12 host is a rendering harness, not an automatic gameplay test. Local paths, drafts, saves, logs, test probes, build outputs and release archives are excluded from Git.

## License and credits

Mod-specific source code and documentation are available under the **[MIT License](LICENSE)**, copyright 2026 **ouyangzhixuan**. Included third-party components keep their own licenses; see [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md).

This is an unofficial community tool. Code, UI text and translations include generative-AI-produced content. No extracted game artwork or game files are distributed.

## Upstream camera integration — 3.4.6

New or missing camera settings default to speed 5, Y inversion on and Keep view on. Existing speed and axis preferences remain. T/Esc leaves controls and releases input while retaining the view when enabled; T resumes. Restore game view always restores the native view, with a button in the drawing toolbar while held. Retained views reset on restart or a native scene change. Drawing, native picking and voxel clipping continue using the effective camera position/direction. The full font-bearing manual package is retained.
