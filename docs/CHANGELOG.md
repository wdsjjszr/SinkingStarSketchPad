# Changelog

## Unreleased — upstream integration and preview visibility (2026-10-25)

- T/Esc now restores the game camera; only the explicit Hold camera button fixes the pose. Remove the automatic Keep view option while retaining speed/sensitivity/axis preferences.
- Keep the compact E/G/B/T HUD hints; wheel up selects the previous ink color and wheel down selects the next. Drawing crosshair arms now use the selected ink.
- Reduce white grid-center dots to a 2.5px radius, retaining the 1.5px black outline thickness for a 4px outer radius.
- Merge upstream 3.4.6 camera retention, speed/Y defaults and scene cleanup while retaining the full font-bearing package and all native-ground/v4/input extensions.
- Put grid previews below saved/pending ink and snap markers above; expand persistent previews to 17×17 with an 8.5-cell radial fade, white cores and black outer outlines. New hint-opacity default is 15%, preserving saved values.
- Shift locks the start world X/Y for cross-height grid lines/arrows. Same-height constraints, wheel bindings and draft v4 remain; invalid projected endpoints keep waiting.
- Static checks/analysis and compilation/linking only. No game, test binary or package script execution; no SHA checks.

## Unreleased — preview readability and cross-height drawing (2026-10-25)

- Add hold-plus-30%-ease-out hint fading, radial persistent previews, thicker points/lines, black/white checker fill and snapped-point markers for grid shapes. New missing-preference defaults are 750ms/45%; saved choices remain.
- Replace center-cell hiding with projected voxel polygon subtraction, stable coplanar contact handling and cached clipped meshes.
- Preserve waiting grid lines/arrows and active orthogonal XYZ paths across wheel height changes. Draft v4 adds relative Z, reads v1–v3 and preserves `drafts.pre-v4.txt` before overwrite.
- Right press cancels a pending shape, finishes an active path, then temporarily erases; repeated XYZ endpoints finish paths. Camera right rotation requires drawing closed. Degenerate rectangles use a single line/point.
- Static assertions/checks, analysis and compilation/linking only; no game/test programs or SHA checks.

## Unreleased — preview settings and wheel bindings (2026-10-25)

- Hide hinted cells whose centers are behind native walls in the current camera view; keep own-top contact visible and cache unchanged view/plane/terrain masks. Initial level-plane setup stays silent.
- Render both grid previews in gray, replacing intersection dots with thicker perspective-aligned crosses.
- Add an automatically saved Settings window for hint layers, opacity, On/Off/Wheel-only and 0–5-second duration (default borders/checker, 25%, 1 second). Live edits never revive expired hints.
- Configure plain/Ctrl/Shift/Alt wheel actions independently: None, plane height or palette color. Defaults are height/plain and color/Shift; multiple modifiers do nothing.
- Static checks, analysis and compilation/linking only; no test programs, game sessions or SHA checks. Draft format remains v3.

## Unreleased — grid drawing and previews (2026-10-24)

- Add independent center/intersection/border/checker layers, opacity and persisted grid drawing preferences.
- Show a two-second plane checker hint after every successful mark; cut solid interiors only. Use the canvas wheel to change plane Z while drawing is open.
- Anchor pen/shapes/text to each mark’s captured grid; share projected outlines and arrowheads across display, erasing and SVG export.
- Support drag-release and click-start/click-end shapes with live previews, right-click cancellation and completed-only history/save/export.
- Read draft v1/v2/v3, write v3 and preserve a non-overwriting `drafts.pre-v3.txt` before the first legacy overwrite. Saving stops if that backup fails.
- The user confirmed ground marking now works. New features have static checks and compilation only, with no test programs or game sessions executed.

## Ground marking revision (2026-10-24)

- Show character-plane and manual-marking buttons together; initialize each level from logical character Z unchanged, and make middle-click a one-shot ground mark.
- Orient picking rays using the effective camera view, independent of depth order; retain voxel-top contact and wall-before-floor checks.
- Read characters/terrain on demand, invalidate cancelled or stale requests, and disable duplicate backup GPU camera collection while keeping free camera.
- Validation is static checks, compile-time assertions, static analysis and compilation/linking only; ground marking was subsequently confirmed by the user.

## 3.4.6 — Faster movement and optional view retention (Beta)

- Raise default movement speed from 3 to 5; preserve any explicitly saved speed.
- Add Keep view when leaving camera controls, enabled by default and saved with other camera preferences. T/Esc releases input while holding the view; T resumes from the retained pose.
- Keep an explicit Restore game view action in the camera panel and drawing toolbar. Disable the option for automatic restoration on exit.
- Clear retained poses on restart or detected scene/camera discontinuity. Keep world-path projection active while the view is held.

## 3.4.5 — Camera controls and axis preferences (Beta)

- Add independent mouse X/Y inversion and an adjustable mouse sensitivity slider, in English and Chinese.
- Save speed, sensitivity and axis settings across camera resets and game restarts without changing language preferences or drafts.
- Reduce default speed from 12 to 3 units/second, rotation from 0.30 to 0.08 degrees/pixel, Shift acceleration from 4× to 2×, and wheel zoom from 12% to 6% per notch.
- Limit total tilt to avoid crossing the overhead pole or horizon; ground movement follows yaw independently of tilt.
- Preserve the GPU camera upload fix confirmed in 3.4.4 and the single T toggle.

## 3.4.4 — GPU camera upload fix (Beta)

- Fix camera controls changing internal values without moving the actual scene: follow recorded CPU-to-GPU constant-buffer uploads and patch validated camera data before submission.
- Keep the effective camera projection shared with world paths; support padded CBVs, late-filled descriptors and bulk descriptor copies.
- Single T remains the camera toggle. WASD pans, Q/Z changes height, right drag rotates, wheel zooms and Esc restores.
- Actual-game camera movement confirmed by the user on 23 October 2026. Broader culling, shadow, reflection and picking compatibility remains experimental.
- Remove temporary raw constant-buffer samples, diagnostic files and per-draw diagnostic callbacks from the release build. Manual bilingual package; no installer scripts.

## 3.4.2 — Camera compatibility investigation (Beta)

- Resolve bound descriptor tables at submission, including descriptors filled after command recording.
- Support CPU-visible GPU Upload heaps in camera reading and overrides.
- Record bounded camera-path diagnostics (heap/view type, dimensions, validity, command-list draw references and patches). No desktop interaction or automatic camera movement.
- **Not confirmed fixed in the actual game.** The user reported that 3.4.1 still did not move the view despite changed rotation parameters. A real-level diagnostic run is required.

## 3.4.1 — Camera input and cached-buffer fixes (Beta)

- Single **T** toggles the camera. Plain C remains the game's character switch.
- Hovering an inactive panel no longer blocks WASD/Q/Z. Active widgets and text fields retain keyboard ownership; right drag and wheel operate on the canvas.
- Reused camera buffers are recalculated from their original game values instead of being skipped. Disabling restores cached originals on submission.
- Added hidden native input regression coverage and cached-buffer GPU geometry/path checks. Actual-game confirmation is still pending.

## 3.4 — Experimental free camera (Beta)
- Add pan, height, orbit/tilt, lens zoom, camera settings and one-action restore in the existing add-on.
- Ctrl+Alt+C toggles the camera; WASD/Q/Z move, right drag rotates, wheel zooms, Esc restores.
- Share effective camera projection with world paths and preserve drawing controls.
- Validate only main-view b2 constants referenced by submitted command lists; camera starts disabled.
- Camera override is experimental: CPU culling, reflections, shadows and mouse picking may still use the original view.
- Keep manual installation; no CMD/BAT/PowerShell installer scripts in the player package.

## 3.3 — Manual-install packaging revision
- Replace installer/uninstaller launchers with manual file-copy instructions in English and Chinese.
- Omit all CMD, batch and PowerShell installation scripts from the player ZIP.
- Keep the existing 3.3 add-on and third-party runtime/font binaries unchanged.
- Manual upgrades must back up existing files and set ReShade InputProcessing=1.
- Automatic backup/conflict detection statements below describe historical installer releases only.

## 3.3 — Keyboard coexistence and focus persistence (Beta)
- Keep ordinary keyboard movement and camera input available while drawing; reserve the mouse for annotations.
- Use Alt+letter tool shortcuts and Alt+1-5 colors so game movement letters do not switch tools.
- Capture the keyboard only for text entry and Ctrl/Alt shortcut chords.
- Keep drawing mode and unfinished text across Alt+Tab; commit/save interrupted strokes and require mouse release before resuming.
- Preserve drawing mode and unfinished text when resize/fullscreen transitions recreate the rendering runtime.
- Set ReShade InputProcessing=1; known-version upgrades back up the old configuration and change only this routing setting.
- Add deterministic focus/input regression tests and a native DX12 overlay test fixture.

## 3.2 — English and Simplified Chinese (Beta)
- Display name unified as SinkingStarSketchPad; compatible file and data paths retained.
- Translate the toolbar, text entry dialog, camera/grid status, save/export messages, and HUD.
- Add System / English / Simplified Chinese selection with persistent preferences.
- Bundle an unmodified, openly licensed Noto CJK font for both languages.
- Add English and Chinese guides and installer/uninstaller entry points.
- Detect Steam libraries instead of embedding a particular user's installation path.
- Preserve earlier drafts, user configuration and language settings during known-version upgrades.

## 3.1
- Add crystal-blade pointer, star crosshair, and rune-ring eraser artwork.

## 3.0
- Add automatic grid detection and world-space path marks reprojected with the game camera.

## 2.1
- Use E/G/B for drawing, cursor and mark visibility; K for erasing.
