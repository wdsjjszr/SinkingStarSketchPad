# SinkingStarSketchPad — In-Game Drawing and Cell Paths

**Version 3.4.6 Beta · English / Simplified Chinese · Windows, DirectX 12**

An in-game scratchpad for Order of the Sinking Star: sketch ideas, place text notes, and plan routes snapped to cell centers. A themed crystal-blade cursor, star crosshair, and rune-ring eraser help keep annotations readable.

## Features
- Native ReShade add-on rendered inside the game frame.
- Experimental free camera: pan, height, orbit/tilt, lens zoom and restore; T toggles it.
- Gentler camera defaults, tilt limits, adjustable mouse sensitivity and independent X/Y inversion; preferences persist across restarts.
- Pen, line, arrow, rectangle, ellipse, text and whole-mark eraser.
- Three automatically saved scratch pages, undo/redo and SVG export.
- Automatic grid inference with a cell-center preview; no manual calibration workflow.
- World-space path references reprojected using the game camera as it pans or zooms.
- English and Simplified Chinese interface with persistent language selection.

## Getting started
Completely exit the game, extract the archive, and copy the contents of game_files next to sinking_star.exe. Keep the font assets in their subfolder. Installation is manual; no CMD, batch, PowerShell or executable installer is included or required. The tested ReShade 6.8.0 full add-on runtime and an openly licensed Noto font are included. For an upgrade or an existing ReShade setup, back up the previous files, preserve drafts and settings, copy only the add-on/font assets, and set [INPUT] InputProcessing=1 in your existing ReShade.ini. See INSTALL_EN.txt for the full steps.

Press **E** for the toolbar, **G** to toggle the cursor and **B** to show/hide annotations. Use the **Language / 语言** menu to choose English or Chinese. Ordinary keyboard input stays available while drawing; Alt+letter selects tools. Alt+Tab preserves drawing mode. For paths, press **E → Alt+H**, inspect the cell-center preview, then click along rows or columns and add a point at each turn.

## Compatibility and limitations
This is a beta. Automated validation includes real game upload data, grid inference, both UI languages, draft compatibility, DX12 rendering and synthetic camera movement, zoom and resize. Full visual alignment across all actual levels has not been comprehensively verified. Check the preview in your level, especially with unusual terrain, floors, transitions or wrapping maps.

Free camera is a render-constant override; original game culling, shadows, reflections, visibility and mouse picking may not follow large changes. Actual-game scene consistency is not yet verified. While enabled, WASD/Q/Z control the camera; right drag rotates, wheel zooms, Esc restores the game view. E still toggles drawing and Ctrl+Z undoes marks.

Only path marks are world anchored. Ordinary drawing and text stay in screen space. Routes do not check obstacles, move the player, push boxes or solve puzzles. Pages are not automatically assigned to levels.

Back up your existing setup manually. Do not overwrite another mod's dxgi.dll or ReShade settings. This package does not perform automatic backups or conflict checks. Only the bundled 6.8.0 full add-on runtime has been validated. See README_EN.md for existing ReShade installations.

## Uninstall
Exit the game and move SinkingStarSketch.addon64 outside the game folder. Keep SinkingStarSketch-data to preserve drafts, exports and language settings. Remove bundled ReShade files only if they are not shared with another graphics mod.

## Credits and development
ReShade: Patrick Mours and contributors, BSD 3-Clause. Dear ImGui: Omar Cornut and contributors, MIT. Noto Sans CJK: SIL OFL 1.1. Runtime notices are included. No extracted game artwork or game files are supplied.

This is an unofficial community tool. Development used generative AI; the code, UI text and translations contain AI-generated content. This page text and the promotional cover were also made with generative AI. Mod-specific source code and documentation are MIT-licensed, copyright 2026 ouyangzhixuan. Third-party components retain their own licenses.

## Source code and license

The project is open source under the **MIT License**.

- Repository: [https://github.com/ouyangzhixuan/SinkingStarSketchPad](https://github.com/ouyangzhixuan/SinkingStarSketchPad)
- License: [MIT](https://github.com/ouyangzhixuan/SinkingStarSketchPad/blob/main/LICENSE)
- Bug reports: [GitHub Issues](https://github.com/ouyangzhixuan/SinkingStarSketchPad/issues)

Third-party components retain their original licenses.

## Upstream camera integration — 3.4.6

New or missing camera settings default to speed 5, Y inversion on and Keep view on. Existing speed and axis preferences remain. T/Esc leaves controls and releases input while retaining the view when enabled; T resumes. Restore game view always restores the native view, with a button in the drawing toolbar while held. Retained views reset on restart or a native scene change. Drawing, native picking and voxel clipping continue using the effective camera position/direction. The full font-bearing manual package is retained.
