# SinkingStarSketchPad 3.4.6

In-game drawing, scratch pages, a themed cursor, and cell-snapped route planning for **Order of the Sinking Star**. This is a native 64-bit ReShade add-on rendered into the game's DirectX 12 frame. It is not a separate desktop overlay.

**Release status: beta.** The user confirmed native ground previews and marking in the levels they checked. The user reported the preceding grid drawing revision works well. This settings/preview revision has static checks and compilation only; full alignment still needs in-game confirmation.

## Requirements

- A 64-bit Windows installation and the DirectX 12 version of the game (`sinking_star.exe`).
- The package includes the tested **ReShade 6.8.0 full add-on support** runtime, the add-on, a configuration file, and an openly licensed Noto CJK font. No separate ReShade download is needed for a fresh install.
- Manual file-copy installation. No CMD, batch, PowerShell or executable installer is included or required.

## Install or upgrade

See **INSTALL_EN.txt** for the quick-start instructions. Completely exit the game first.

For a fresh installation with no existing ReShade/graphics proxy, extract the archive and copy the **contents** of `game_files` into the folder containing `sinking_star.exe`. Keep this layout:

```text
sinking_star.exe
dxgi.dll
SinkingStarSketch.addon64
ReShade.ini
SinkingStarSketch-assets/
  NotoSansCJKsc-Regular.otf
```

Do not place the `game_files` folder itself next to the executable. Start the game and press **E** to open the toolbar.

For an upgrade, back up the existing add-on, font assets, `ReShade.ini`, and `SinkingStarSketch-data` outside the game directory. Copy the new add-on and font assets, preserve your drafts/preferences, and keep the existing configuration. In `ReShade.ini`, set the key below in its existing section, replacing the old value without duplicates:

```ini
[INPUT]
InputProcessing=1
```

Add this section only if it is absent. This value is required for separate mouse/keyboard capture; mode 2 blocks the keyboard and mode 0 allows drawing clicks to reach the game. This is a manual change; no installer backs up or edits settings for you.

Keep an existing ReShade runtime instead of overwriting it blindly. Only the bundled ReShade 6.8.0 full add-on runtime has been tested. Read **Existing ReShade installations** below before combining graphics mods. No automatic conflict detection is performed in this manual package.

## Language

The toolbar's **Language / 语言** menu offers **System**, **English**, and **Chinese (Simplified)**. System uses Chinese for a Chinese Windows UI language and English otherwise. Changes take effect immediately and persist in:

`SinkingStarSketch-data/settings.ini`

You can also close the game and set this file manually:

```ini
[Interface]
Language=en
```

Other accepted values are `zh-CN` and `auto`. The setting changes the mod interface and notifications. Your annotations and the base game language are not translated.

## Controls

| Action | Key / input |
| --- | --- |
| Enter or leave drawing mode | E |
| Show or hide the mod cursor | G |
| Show or hide all marks | B |
| Cancel a stroke/text entry; otherwise return to the game | Esc |
| Pen / line / arrow | Alt+P / Alt+L / Alt+A |
| Rectangle / ellipse / text | Alt+R / Alt+O / Alt+T |
| Whole-mark eraser / cell path | Alt+K / Alt+H |
| Draw a stroke or shape | Hold and drag the left mouse button |
| Line / arrow / rectangle / ellipse | Also supports click start, then click end; right-click cancels |
| Raise / lower target plane one cell | Default: wheel on drawing canvas; configurable in Settings |
| Cycle ink color | Default: Shift+wheel on drawing canvas |
| Choose one of five ink colors | Alt+1-5 |
| Snap a line/arrow to 45 degrees; constrain rectangle/ellipse | Hold Shift |
| Undo | Ctrl+Z |
| Temporary whole-mark eraser | Hold right button; pending shape cancellation / active path finish takes priority |
| Redo | Ctrl+Y, or Ctrl+Shift+Z |
| Switch among three scratch pages | Ctrl+1 / Ctrl+2 / Ctrl+3 |
| Save | Ctrl+S |
| Clear this page, with undo | Ctrl+Shift+Delete |
| Finish text entry | Ctrl+Enter |
| Mark the ground under the pointer once | Middle-click on canvas while drawing |

E/G/B only trigger without Ctrl, Shift, or Alt, and pause while the text entry dialog is open. They were checked against the game's default keymaps; customized keymaps may conflict. While drawing, ordinary keyboard input continues to the game, including movement and camera controls. Use Alt+letter for tools and Alt+1-5 for colors to avoid triggering tools with game movement keys. The mod captures the keyboard while text entry is open or Ctrl/Alt is held for mod shortcuts. Custom game bindings may still overlap those chords. With the free camera disabled, mouse input is reserved for drawing; press E to use mouse controls in the game. Alt+Tab preserves drawing mode and text entry, finishes actual drag strokes, cancels unfinished click shapes and saves completed marks, and pauses mouse interaction until the game is active again. Release held mouse buttons before drawing again.

## Experimental free camera

Press the single key **T**, or the toolbar's **Free camera** button, after entering a level. The feature starts disabled on each game launch and requires a validated main-view camera. WASD works while hovering an inactive panel; use right drag and wheel outside panels. Camera mode displays the cursor. Active widgets and text entry pause camera keyboard controls. It is integrated with the sketchpad; no second mod or project is needed.

| Action | Input |
| --- | --- |
| Pan on the world plane | W/A/S/D |
| Move up / down | Q / Z |
| Move faster | Hold Shift |
| Orbit the view around its captured focus | Close drawing, then hold right mouse button and drag |
| Zoom the lens | Mouse wheel on canvas with drawing closed |
| Show / hide drawing toolbar | E |
| Undo an annotation while the camera is active | Ctrl+Z |
| Leave camera controls and restore game view | Esc or T |
| Restore the native game view | Restore game view button |

The camera panel offers separate **Invert mouse X** and **Invert mouse Y** checkboxes. X reverses horizontal right-drag rotation; Y reverses vertical right-drag tilt. Neither changes WASD, height keys or the wheel. X defaults off and Y on; saved axis settings remain. Movement speed and mouse sensitivity are adjustable and are saved together with these checkboxes in `SinkingStarSketch-data/settings.ini`; reopening the camera or restarting the game retains them. Resetting the view keeps these preferences.

Default movement speed is now 5 world units/second (previously 3), mouse sensitivity is 0.08 degrees/pixel (previously 0.30), Shift gives 2× speed, and wheel zoom changes by 6% per notch. Tilt stops at the overhead position and five degrees before the horizon, keeping the original side of the view. WASD directions follow yaw independently of tilt, preventing reversal when looking down.

The camera panel also offers X/Y/height offsets, yaw, tilt, zoom and reset-to-current-game-camera controls. While enabled, keyboard and mouse input are reserved for the camera and drawing, so WASD does not move the character. Text entry pauses camera movement. A scene/camera discontinuity may disable the override automatically. The normal 3.3-style keyboard/game coexistence returns when the free camera is disabled. Existing world paths use the same modified camera projection and remain anchored to cells; ordinary pen/text annotations remain in screen space unless created with Grid drawing enabled.

**Experimental render override:** this changes validated DirectX 12 camera constants referenced by the submitted draw commands, not the game's CPU scene camera. Game-side culling, shadows, reflected views, visibility/streaming and mouse picking may still use the original view. Large translations, low angles or looking outside the originally visible area can reveal missing geometry or visual artifacts. It does not unlock unloaded levels or pause game simulation. Restore the game view if needed; full scene consistency is not yet verified in the actual game.

## Grid drawing and previews

Toggle **Grid drawing** to create new marks on the target plane. Pen/ellipse use continuous plane coordinates; line/arrow/rectangle endpoints snap to the nearest center or intersection, with Shift constraints calculated on that plane. For cross-height grid lines/arrows, Shift instead locks the start world X/Y to create a vertical line; same-plane constraints and wheel bindings remain unchanged. Text anchors follow the world while letters stay upright and fixed in screen size. Erasing uses the current projection; paths retain their existing rules. Each mark keeps its own plane, ink and width.

**Grid preview** covers 17×17 cells with an 8.5-cell radial fade, white points/lines/crosses with black outer outlines, and black/white checker fill. Previews sit below saved/pending ink; snap markers remain above it. Grid line/arrow/rectangle tools show the actual snapped endpoint. **Settings** configures hints: new defaults are 15% opacity and a 0.75-second hold followed by a 30% ease-out fade; saved preferences remain. Hints clip against projected native voxel faces at sub-cell boundaries, retaining self-contact surfaces without shifting Z. Initialization stays silent, and the 128×128 cap remains. See [the full behavior and draft-upgrade guide](GRID_DRAWING.md).

## Cell paths that follow the camera

1. Enter a level, press **E**, then **Alt+H** or click **Path**.
2. The level initializes all three pages once from the controlled character logical Z, without requiring ground below. **Set plane to character** reads the character again for the current page. Targets stay fixed until another mark; wait for a valid game camera to draw paths.
3. Enable **Grid preview → Cell centers** to inspect the snap points near your mouse.
4. Click a cell center to start a path. Click along one of X, Y or Z to connect intermediate centers; use wheel height changes for vertical runs.
5. Place a point at each corner. A diagonal target is rejected until you add the corner.
6. Click the same XYZ endpoint again, right-click, or use **Finish / New path** to finish before starting another route.
7. Press **E** to return to the game. The path is stored using a world-space grid reference and is reprojected with the camera as it moves or zooms.

Each page keeps an independent target for this session. **Manual marking** requests fresh native terrain, then previews ground cells. Left-click sets the plane and exits on success; a wall, empty space or pending read keeps marking active. Middle-click requests fresh terrain and marks once using the pointer and camera captured at the click, then exits even on failure. It preserves the selected tool and adds no path point. Esc, switching pages/tools and a newer marking request invalidate a pending result. With drawing open, the wheel follows four configurable bindings. Defaults are height adjustment for plain wheel and color cycling for Shift+wheel; Ctrl/Alt+wheel do nothing, as do multiple modifiers. Height changes preserve fractional Z; close drawing for free-camera zoom. Switching levels initializes all pages from one character sample and retains existing paths. See [ground marking](MANUAL_GROUND.md).

The background reader only probes level identity about once per second while idle. Terrain refreshes on level entry and manual/middle-click requests; characters are sampled for initialization and the character-plane button. Initialization retries until a valid character is available, then stops. The duplicate backup GPU camera collector is disabled; effective-camera and free-camera processing remain active.

Ctrl+Z undoes a path addition and ends the active path. Right-click finishes an active path without erasing; otherwise it temporarily erases, after pending-shape cancellation. Alt+K selects the whole-path/whole-mark eraser. A path retains its own grid reference, so a later change in automatic detection does not move existing marks.

This tool marks routes only: it does not move the player, push boxes, check obstacles, or solve puzzles. Ordinary drawing stays in screen space; enable Grid drawing for new pen/shapes and text anchors to follow the world. Use a different scratch page for another level; pages are not automatically assigned to level names.

## Cursor artwork

Normal mode uses a pale gold crystal-blade pointer with a dark edge and a small engraved star. Drawing uses an open star-shaped crosshair whose arms, center and outer dots show the selected ink color. The eraser uses an open gold rune ring. The pointer tip or crosshair center is the mouse hotspot. Vector artwork scales modestly at higher resolutions.

## Drafts, export, and limits

- Draft: `SinkingStarSketch-data/drafts.txt` in the game folder.
- Previous save: `drafts.txt.bak`.
- Language, camera, grid drawing and preview preferences: `settings.ini` in the same folder.
- **Export SVG** writes a transparent vector image to `SinkingStarSketch-data/exports`. World marks use the same current-camera projection as display and erasing; without a valid camera they are omitted. Unfinished shape previews are never exported.
- Up to 2,000 marks per page, 120,000 total points, and 60 undo steps. Undo history does not survive a restart.
- Draft format **v4** reads v1/v2/v3, stores each mark’s grid and relative point Z. Before upgrading an old draft, `drafts.pre-v4.txt` is preserved once without overwriting it; backup failure blocks saving. Restore that file before using an older add-on. The regular `.bak` continues to track the previous save.

## Uninstall

Completely exit the game and move `SinkingStarSketch.addon64` outside the game folder. This disables the sketchpad. Preserve `SinkingStarSketch-data` to keep drafts, language preferences and exports.

To remove the bundled ReShade from a fresh installation as well, move its `dxgi.dll`, `ReShade.ini`, and font assets outside the game folder only after confirming they are not used by another graphics mod. Restore a previous graphics setup from your backup if applicable. No uninstall script is required.

## Existing ReShade installations

If you already use ReShade, back up that setup and verify its version and **full add-on support** before proceeding. You may copy only `SinkingStarSketch.addon64` and the font assets into your existing installation, enable loading add-ons from the game folder, and keep your own `dxgi.dll` and other ReShade settings. Set `[INPUT] InputProcessing=1` in `ReShade.ini` before starting the game; mode 2 still blocks the keyboard during drawing, and mode 0 does not protect game mouse input. Only the bundled 6.8.0 runtime has been validated here. Other ReShade versions and graphics proxy combinations are not verified. Select the bundled Noto font in ReShade's settings if Chinese glyphs are missing.

To disable a manual install, exit the game and move this add-on out of the game directory. Remove the bundled `dxgi.dll` only if it belongs to this mod's installation and is not shared by another setup.

## Troubleshooting and scope

- No toolbar: confirm installation beside the actual executable, completely restart the game, then press E. **Home** opens ReShade's own settings, whose language is separate from this mod.
- Grid/camera still waiting: enter a regular level, wait, and inspect the preview. Ground cells come from native scene data. Character selection, special supports, camera/level transitions, wrapping-map copies and game updates need verification. Manual wall occlusion only covers solid native voxels, not decorative models. Do not assume an arbitrary grid if detection is unavailable.
- Incorrect alignment: stop using that route and keep a screenshot plus a description of the level/camera view for a bug report. Do not treat the preview as an obstacle or walkability map.
- Missing Chinese glyphs after an upgrade: keep your existing `ReShade.ini` and inspect its font setting. In ReShade settings, choose `SinkingStarSketch-assets/NotoSansCJKsc-Regular.otf` as the font and restart if needed.
- Diagnostics: `ReShade.log` and `SinkingStarSketch-data/status.txt`. Review logs and remove personal paths before posting them publicly.
- Native preview and ground marking were confirmed by the user in tested levels. The preceding grid drawing revision was reported usable. This settings/preview revision received static checks and compilation only; no test programs or game sessions were run.

## Credits and third-party notices

This is an unofficial community tool, not an official game feature. The cursor artwork is drawn by the add-on; no extracted game artwork or game files are distributed.

- ReShade 6.8.0 by Patrick Mours and contributors: [official site](https://reshade.me/) — BSD 3-Clause notice in `licenses`.
- Dear ImGui by Omar Cornut and contributors: [repository](https://github.com/ocornut/imgui) — MIT notice in `licenses`.
- Noto Sans CJK: [official repository](https://github.com/notofonts/noto-cjk) — unmodified font and SIL Open Font License 1.1 notice in `licenses`.
- LLVM-MinGW runtime components: bundled notices in `licenses`.

Development and translations include generative-AI-produced code and text. Mod-specific source code and documentation are licensed under MIT, copyright 2026 ouyangzhixuan. Third-party components retain their original licenses.

The display name is **SinkingStarSketchPad**. The compatible add-on filename (`SinkingStarSketch.addon64`) and existing data/font folders retain their original names, so upgrades continue using your saved pages and preferences.

## Source code and license

The project is open source under the **MIT License**.

- Repository: [https://github.com/ouyangzhixuan/SinkingStarSketchPad](https://github.com/ouyangzhixuan/SinkingStarSketchPad)
- License: [MIT](https://github.com/ouyangzhixuan/SinkingStarSketchPad/blob/main/LICENSE)
- Bug reports: [GitHub Issues](https://github.com/ouyangzhixuan/SinkingStarSketchPad/issues)

Third-party components retain their original licenses.

## Upstream camera integration — 3.4.6

New or missing camera settings default to speed 5 and Y inversion on; existing speed and axis preferences remain. T/Esc exits controls and restores the game view. Only the Hold camera button leaves the pose fixed while releasing input; T resumes adjustment, and Esc or Restore game view restores the native camera. Fixed poses reset on restart or a native scene change. Drawing, native picking and voxel clipping continue using the effective camera position/direction. The full font-bearing manual package is retained.
