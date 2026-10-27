<div align="center">

# SinkingStarSketchPad

**An in-game sketchpad for Order of the Sinking Star — draw notes, arrows, and routes directly into the game world**

Move the camera, and your marks move with the scene as if they were drawn on the ground.

[简体中文](../README.md) · **English**

![version](https://img.shields.io/badge/version-1.0-blue)
![platform](https://img.shields.io/badge/platform-Windows%2064--bit-lightgrey)
![status](https://img.shields.io/badge/status-Beta-orange)
![license](https://img.shields.io/badge/license-MIT-green)

[Key features](#key-features) · [More features](#more-features) · [Installation](#installation) · [Shortcuts](#shortcuts) · [FAQ](#faq) · [Known limitations](#known-limitations)

</div>

<p align="center">
<img src="images/hero.gif" alt="In-game drawing and world-space marks" width="720"><br>
<em>In-game drawing and world-space marks</em>
</p>

---

## Introduction

SinkingStarSketchPad is an in-game sketchpad for **Order of the Sinking Star**. Draw, write notes, and mark routes while playing to keep track of puzzle ideas and plan your next moves.

It runs as a ReShade add-on without modifying the game's files. Once installed, press **E** in the game to start drawing.

> [!NOTE]
> **Beta release.** Grid drawing and ground marking have been confirmed in some levels. The latest settings, preview, and mouse-wheel changes have only received compilation checks and have not been fully verified in the game. Free camera is experimental; see [Known limitations](#known-limitations).

---

## Key features

### Marks stay in the world, not on the screen

A conventional sketchpad keeps marks on the screen. Move the camera, and those marks no longer line up with the scene.

**Grid drawing** places your marks on a plane in the game world, such as the floor beneath your character. As the camera moves, your strokes, arrows, and text anchors stay in their original positions, as if drawn on the ground.

<table align="center" width="680">
<tr>
<td width="50%" align="center"><b>Regular drawing: fixed on the screen</b></td>
<td width="50%" align="center"><b>Grid drawing: fixed in the world</b></td>
</tr>
<tr>
<td>
<p align="center">
<img src="images/compare-screen.gif" alt="Regular drawing stays fixed on the screen" width="320"><br>
<em>Regular drawing stays fixed on the screen</em>
</p>
</td>
<td>
<p align="center">
<img src="images/compare-world.gif" alt="Grid drawing stays fixed in the world" width="320"><br>
<em>Grid drawing stays fixed in the world</em>
</p>
</td>
</tr>
</table>

### Get started in three steps

1. **Enter drawing mode:** press `E`, then click **Grid drawing** on the toolbar.
2. **Choose a plane:** decide which floor or height you want to draw on.
   - Click **Set to character plane** to use the height beneath your character.
   - Or click **Manual marking**, then click the ground you want to draw on. Middle-click performs a quick one-shot mark.
   - To move up or down a level, **scroll the mouse wheel** over the canvas. Each notch changes the height by one cell.
3. **Start drawing.** Move the camera afterward to see your marks stay with the scene.

### What you can draw on a plane

| Tool | Behavior on the plane |
|---|---|
| Pen, ellipse | Draw freely along the plane |
| Line, arrow, rectangle | Endpoints snap to cell centers or grid intersections for neat shapes |
| Lines / arrows across heights | When the endpoints are on different levels, the result becomes a vertical line, useful for marking stairs or floor changes |
| Text | The anchor stays in the world, while the text remains upright and keeps a constant screen size |
| Eraser | Removes an entire mark at once |

Each mark remembers its own plane, color, and stroke width independently.

Enable **Grid preview** to see nearby cells and where points will snap. Changing the plane height also briefly shows the new plane's position.

<p>
<img src="images/grid-preview.png" alt="Grid preview and snap points" width="720"><br>
<em>Grid preview and snap points</em>
</p>

<p>
<img src="images/vertical-line.gif" alt="A vertical line between levels with a landing guide" width="720"><br>
<em>A vertical line between levels with a landing guide</em>
</p>

For full rules, see [Grid drawing](GRID_DRAWING.md) and [Ground marking](MANUAL_GROUND.md) (in Chinese).

---

## More features

### Cell paths

Click through cell centers to mark a route.

- Click a cell center to start, then another point along the X, Y, or Z axis. **Intermediate cells are filled in automatically.**
- Add a point at each turn; you cannot jump directly to a diagonal cell.
- Change the height with the wheel to connect vertical segments between floors.
- Enable **Path numbering** to number every path point automatically. Each new path starts at 1.
- Paths stay fixed to their original cells as the camera moves. Undo is supported.

<p>
<img src="images/path.png" alt="A cell path with numbered points" width="720"><br>
<em>A cell path with numbered points</em>
</p>

> This tool **only marks routes**. It does not check obstacles, find paths automatically, or move your character.

### Free camera (experimental)

Press `T` to leave the default game view and pan, rotate, raise, or lower the camera. Combined with grid drawing, it is useful for planning routes from above.

| Action | Control |
|---|---|
| Pan along the ground | `W` `A` `S` `D` |
| Move up / down | `Q` / `Z` |
| Move faster | Hold `Shift` |
| Rotate and tilt | Hold the right mouse button and drag |
| Zoom | Scroll with drawing mode closed |
| Exit and restore the game view | `T` or `Esc` |

<details>
<summary>More about free camera</summary>

- It starts disabled each time you launch the game. Enter a level and wait for the add-on to identify the main game camera before using it.
- The camera panel provides X / Y / height offsets, rotation, tilt, zoom, and a reset to the current game camera.
- **Invert mouse X / Y** are independent switches: X changes horizontal right-drag direction; Y changes vertical right-drag direction. They do not affect WASD or the wheel. X defaults off and Y defaults on.
- Movement speed (default 5), mouse sensitivity, and both inversion switches are saved across restarts.
- **Hold camera** keeps the current pose and returns keyboard/mouse input to the game. Press `T` to resume adjustments, or `Esc` / **Restore game view** to restore the original view. Held poses do not survive restarts and are cleared on level changes.
- WASD does not move your character while camera controls are active. Camera shortcuts pause while entering text.
- A detected scene change or large jump in the original camera may disable free camera automatically.

</details>

### Drawing tools

Pen, line, arrow, rectangle, ellipse, text, and whole-mark eraser, with five ink colors. Hold `Shift` to constrain lines to 45° angles or draw squares and circles. Undo / redo supports 60 steps per page.

<p>
<img src="images/toolbar.png" alt="Drawing toolbar" width="720"><br>
<em>Drawing toolbar</em>
</p>

### Three scratch pages and export

- Three independent, automatically saved pages; switch with `Ctrl+1 / 2 / 3`.
- Each page can hold its own plane. Use **different pages for different levels**.
- Export **SVG** using **Export SVG** on the toolbar.

### Themed cursors

- Normal mode: a pale-gold crystal-blade pointer with star engravings.
- Drawing mode: a hollow star crosshair in the current ink color.
- Eraser: a hollow rune ring.

<p>
<img src="images/cursors.png" alt="Crystal-blade pointer, star crosshair, and rune-ring eraser" width="720"><br>
<em>Crystal-blade pointer, star crosshair, and rune-ring eraser</em>
</p>

### English / Chinese interface and UI scaling

- In **Settings → Language**, choose System, English, or Simplified Chinese. Changes take effect immediately and persist across restarts.
- **Settings → UI scale** adjusts windows and tooltips from 75% to 200%. Click **Apply scale** to apply the change.
- Strokes, text annotations, and the grid keep their existing size.

---

## Installation

**Requirements:** 64-bit Windows and the DirectX 12 version of **Order of the Sinking Star**. Download a release package from [Releases](https://github.com/wdsjjszr/SinkingStarSketchPad/releases).

Installation is **manual file copying**. No installer or script is required.

**First installation (no existing ReShade or graphics proxy):**

1. Save your progress and **completely exit the game**.
2. Extract the release package and copy the **contents of** `game_files` into the folder containing `sinking_star.exe`.
   Copy the contents, not the `game_files` folder itself.
3. Start the game and press **E** to open drawing mode.

<details>
<summary>Expected game directory after copying</summary>

```text
sinking_star.exe
dxgi.dll
SinkingStarSketch.addon64
ReShade.ini
Sketch-ReShadePreset.ini
SinkingStarSketch-assets/
  NotoSansCJKsc-Regular.otf
  NotoSansCJKsc-Bold.otf
```

</details>

<details>
<summary>Upgrading from an older version</summary>

1. Back up the old add-on, fonts, `ReShade.ini`, and `SinkingStarSketch-data` **outside the game directory**.
2. Copy the new add-on and fonts, keeping your existing drafts, preferences, and configuration.
3. In `ReShade.ini`, change `InputProcessing` to `1` in the existing `[INPUT]` section without adding a duplicate key. If the section is missing, add:

```ini
[INPUT]
InputProcessing=1
```

This value separates keyboard and mouse handling: `2` continues to block keyboard input while drawing, while `0` lets drawing mouse actions reach the game. The package does not automatically back up files, edit settings, or detect conflicts; complete these steps yourself.

If Chinese characters are missing after upgrading, see the font configuration below.

</details>

<a id="existing-reshade-installations"></a>
<details>
<summary>Existing ReShade installations</summary>

Do not overwrite your existing runtime. Back up your setup and check that it supports **full add-ons**. Copy only the add-on and fonts, keep your existing runtime and configuration, manually set `[INPUT] InputProcessing=1`, and ensure ReShade loads add-ons from the game directory.

Only the bundled **ReShade 6.8.0 full add-on support** runtime has been validated. Other ReShade versions and graphics proxy combinations have not been verified.

</details>

<details>
<summary>Font configuration for existing installations (missing Chinese characters)</summary>

Set the following in the `[STYLE]` section of `ReShade.ini`:

```ini
[STYLE]
Font=.\SinkingStarSketch-assets\NotoSansCJKsc-Regular.otf
LatinFont=.\SinkingStarSketch-assets\NotoSansCJKsc-Regular.otf
FontSize=18
EditorFont=.\SinkingStarSketch-assets\NotoSansCJKsc-Bold.otf
EditorFontSize=18
```

Restart the game to apply. Replacing only the add-on does not change your existing ReShade font settings. If the bold font or its configuration is missing, regular text is used instead.

</details>

---

## Shortcuts

Hover over toolbar icons to see names and shortcuts. Click **Shortcut help** on the toolbar for a draggable window with the full list.

| Action | Control |
|---|---|
| Enter / exit drawing mode | `E` |
| Pen / line / arrow | `Alt+P` / `Alt+L` / `Alt+A` |
| Rectangle / ellipse / text | `Alt+R` / `Alt+O` / `Alt+T` |
| Whole-mark eraser / cell path | `Alt+K` / `Alt+H` |
| Five ink colors | `Alt+1` – `Alt+5` |
| **Raise / lower the drawing plane** | Scroll over the canvas (configurable in Settings) |
| Cycle colors / tools | `Shift+wheel` / `Alt+wheel` |
| Temporary eraser | Hold the right mouse button |
| Constrain to 45° lines / squares / circles | Hold `Shift` |
| Undo / redo | `Ctrl+Z` / `Ctrl+Y` |
| Switch scratch pages | `Ctrl+1` / `Ctrl+2` / `Ctrl+3` |
| Save / clear the current page (undoable) | `Ctrl+S` / `Ctrl+Shift+Delete` |
| Finish text entry | `Ctrl+Enter` |
| Cancel the current stroke or text; otherwise return to the game | `Esc` |
| Free camera | `T` |
| Toggle pointer / draft visibility | `G` / `B` |

<details>
<summary>Usage tips</summary>

- Lines, arrows, rectangles, and ellipses support both dragging and clicking a start point followed by an endpoint. Right-click cancels an unfinished shape.
- While drawing, regular keyboard input still reaches the game, so you can move your character and camera. **The sketchpad takes over the mouse**; press `E` when you need the game's mouse controls.
- Opening text entry or holding `Ctrl` / `Alt` for mod shortcuts temporarily gives the mod keyboard control.
- `E` / `G` / `B` work only without `Ctrl` / `Shift` / `Alt` held and with no text entry field open.
- Switching away from the game preserves drawing mode and unfinished text and saves completed marks. Release the mouse buttons before continuing when you return, so the click used to switch windows does not draw on the page.
- Shortcuts have been checked against the game's default bindings. Custom game bindings may conflict.

</details>

---

## FAQ

**No toolbar after starting the game?**
Check that the files are beside the actual running `sinking_star.exe`, completely restart the game, and press `E`.

**Grid or camera keeps showing “waiting”?**
Enter a regular level and wait a moment. Do not assume that any grid visible on screen represents the actual ground.

**Chinese characters missing after an upgrade?**
Check your preserved `ReShade.ini`. Select `SinkingStarSketch-assets/NotoSansCJKsc-Regular.otf` in ReShade's font settings, or use the font configuration above.

**What should I include in a bug report?**
Logs are in `ReShade.log` and `SinkingStarSketch-data/status.txt` in the game directory. Check for personal paths before sharing them publicly.

---

## Known limitations

- **Free camera changes the rendered view, not the game's own camera.** Game-side culling, shadows, reflections, object loading, and mouse picking may still use the original view. Large camera moves, low angles, or views into previously unseen areas can cause missing objects or visual issues. It does not unlock unloaded areas or pause the game.
- **Cell paths only mark routes.** They do not check obstacles or find paths automatically.
- **Planes do not automatically follow levels.** All three pages reinitialize their target planes on a level change; existing paths retain their own cells. Use separate scratch pages for different levels.
- Occlusion checks cover only the game's native solid walls, not decorative models, lifts, lily pads, or similar objects.
- Special slopes, multiple floors, camera transitions, wrapping maps, and behavior after game updates require in-game verification.

---

## Data, export, and removal

<details>
<summary>Where drafts and settings are stored</summary>

- Drafts: `SinkingStarSketch-data/drafts.txt`, with the previous version backed up as `.bak`.
- Settings (language, camera, grid drawing, previews, and other preferences): `settings.ini` in the same directory.
- Exported SVG files: `SinkingStarSketch-data/exports/`. World-space marks are omitted if there is no valid camera; unfinished shapes are not exported.
- Each page supports up to 2,000 marks and 120,000 points in total. Undo history does not survive restarts.

**The draft format is now v5**, with support for reading v1–v4. Upgrades create a non-overwriting `drafts.pre-v5.txt` backup; saving stops if that backup fails. Older add-ons cannot read v5. Restore this backup before downgrading.

Example language setting (`Language` accepts `zh-CN`, `en`, or `auto`):

```ini
[Interface]
Language=en
```

</details>

<details>
<summary>Uninstalling</summary>

Exit the game and move `SinkingStarSketch.addon64` outside the game directory to disable it. Keep `SinkingStarSketch-data` to retain drafts, preferences, and exports.

To also remove ReShade bundled with a fresh installation, first confirm that no other mod shares it, then move `dxgi.dll`, `ReShade.ini`, and the font assets out of the game directory. Restore your original graphics settings from a backup if needed. No uninstall script is required.

</details>

<details>
<summary>Building from source</summary>

Requires 64-bit Windows.

1. Download `llvm-mingw-20261006-ucrt-x86_64.zip` from the [LLVM-MinGW release page](https://github.com/mstorsjo/llvm-mingw/releases/tag/20261006). Extract it into `.tools/` at the repository root, keeping the top-level directory. The compiler should be at `.tools/llvm-mingw-20261006-ucrt-x86_64/bin/clang++.exe`.
2. Run these commands in PowerShell from the repository root:

```powershell
.\build.ps1
.\test.ps1
```

The output is `build/SinkingStarSketch.addon64`. `test.ps1` performs static compilation checks only and does not run programs. Use `-Compiler 'another\path\clang++.exe'` to select another compiler. `.tools/` and `build/` are ignored by Git.

The ReShade headers are included in the repository. Visual Studio and system PATH changes are not required. In-game verification additionally requires a compatible ReShade runtime, fonts, and the game environment.

</details>

---

## Documentation

- [中文 README](../README.md)
- [Grid drawing (Chinese)](GRID_DRAWING.md)
- [Ground marking (Chinese)](MANUAL_GROUND.md)
- [Third-party components](../THIRD_PARTY_NOTICES.md)

---

## About this project

This project is a fork of [ouyangzhixuan/SinkingStarSketchPad](https://github.com/ouyangzhixuan/SinkingStarSketchPad), maintained by **wdsjjszr**.

- The add-on filename `SinkingStarSketch.addon64` and the draft/font directory names are unchanged so existing data and preferences remain usable after upgrading.
- This is an unofficial community tool. No extracted game files or artwork are distributed.
- Code, UI text, and translations include generative-AI-produced content.

### License

Source code and documentation use the **MIT** license. The original author's copyright is retained; fork modifications are copyright 2026 wdsjjszr and also use MIT.

Bundled third-party components keep their own licenses: ReShade (BSD 3-Clause), Dear ImGui (MIT), Noto Sans CJK (SIL OFL 1.1), and LLVM-MinGW runtimes. Full license texts are in the release package's `licenses` directory.
