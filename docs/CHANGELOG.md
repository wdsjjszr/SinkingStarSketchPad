# Changelog

## Unreleased — ground marking revision (2026-10-24)

- Show character-plane and manual-marking buttons together; initialize each level from logical character Z unchanged, and make middle-click a one-shot ground mark.
- Orient picking rays using the effective camera view, independent of depth order; retain voxel-top contact and wall-before-floor checks.
- Read characters/terrain on demand, invalidate cancelled or stale requests, and disable duplicate backup GPU camera collection while keeping free camera.
- Validation is static checks, compile-time assertions, static analysis and compilation/linking only; actual-game picking remains unverified.

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
