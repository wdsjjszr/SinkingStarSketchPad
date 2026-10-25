# Validation — 3.4.6 Beta

## Preview visibility and upstream integration — 25 October 2026

- Follow-up checks cover reversed palette cycling, bilingual compact HUD hints, the 2.5px white/5.5px black center dot and explicit hold/resume/restore bridge fixtures. Integration fixtures are compiled only.
- Compile-time assertions cover explicit camera hold and restored exit states, upstream defaults, 17-cell/radial parameters, 15% missing-preference opacity and cross-height vertical snapping with same-plane compatibility.
- Compile-only fixtures cover exterior line/dot outlines, large preview batches, Shift release/Z preservation, pointer-ray failure with a valid vertical target, invalid camera/plane and behind-camera rejection, marker/projected-endpoint agreement and existing preference retention. Non-constexpr assertions are not run.
- Static source ordering checks verify preview < ink < snap marker. C++17 syntax, Clang analysis and add-on linking are used; the full packager is parsed without execution. Game/test binaries, runtime scripts and SHA checks are not run.
- Actual appearance, performance and held-camera interaction still require game confirmation. Existing unstaged changes and untracked depth prototypes are preserved; full fonts/manual packaging remain.


## Readability and cross-height revision — 25 October 2026

- Static assertions cover hold/fade timing, radial weights, checker parity, batching/index bounds, contact tolerance, three-axis path classification and right-button priority.
- Compile-only assertion fixtures cover partial/overlapping polygon subtraction and alpha interpolation, merged voxel faces, coplanar contact under camera jitter, reverse depth/cache invalidation, preview layers, degenerate rectangles, v1–v4 storage, relative-Z projection/erasing/SVG call paths, failed upgrade backup, preserved pending shapes, vertical paths and erase history. Non-constexpr assertions are not executed.
- Verification is C++17 syntax/static compilation, Clang static analysis and add-on compilation/linking. No game sessions or test binaries are run; no SHA checks are performed.
- Unverified on hardware: reported flicker elimination, clipped-edge appearance/performance, gesture feel and upgrade persistence/recovery. Native voxels alone determine hint occlusion; restore `drafts.pre-v4.txt` before downgrading.


## Preview settings revision — 25 October 2026

- The user reported the preceding drawing revision works well. This revision receives static compilation/assertions, Clang static analysis and production compilation/linking only. No test programs or game sessions are run, and no SHA checks are performed.
- Compile-time assertions cover silent initialization, On/Off/Wheel-only, 0/1/5-second timing, same-height restart, last-hint/page/scene isolation, live duration changes without revival, wheel defaults/modifiers/fraction resets and bidirectional palette wrapping.
- Compile-only assertion fixtures cover wall-before-center versus own-top contact, reverse-depth rays, mask edge ownership/cache invalidation, mixed world marks and INI round-tripping while preserving language. Non-constexpr assertions are not executed.
- Runtime gaps: native-wall-only blocky occlusion, actual camera alignment/cross appearance, UI/keyboard capture, persistence and frame cost while moving the camera. Hints remain capped at 128×128 cells.


## Grid drawing extension — 24 October 2026

- The user confirmed ground marking now works; the additional preview/drawing behaviors still need in-game confirmation.
- Static assertions cover preview defaults/opacity, clipping, solid interior versus top contact, last-hint timing/page/scene isolation, fractional wheel accumulation, center/intersection snapping, negative coordinates, Shift constraints and shape gesture transitions.
- Compile-only fixtures cover mixed screen/world v3 serialization, v1/v2 reading including old paths, perspective outlines/text anchors, projected erasing/arrowheads, SVG call sites, captured planes, gesture completion/history/cancellation and the legacy-backup failure path. Their non-constexpr assertions are not executed.
- Validation is C++17 syntax/static compilation, Clang static analysis and production add-on compilation/linking. No test executables or game sessions are run; no SHA checks are performed.
- Runtime gaps: actual camera alignment, drawing feel, INI persistence and filesystem upgrade/recovery behavior. Far-plane hints are limited to 128×128 cells; restore `drafts.pre-v3.txt` before using an older add-on.

## Ground marking revision

- **Ground marking revision, 24 October 2026:** viewing rays now use the effective camera eye/forward direction, with ordinary/reverse-depth and voxel-top fixtures. Character Z initialization, one-shot marking, page/request invalidation and on-demand acquisition replace the previous follow modes. The duplicate backup GPU camera collector is disabled; primary effective-camera/free-camera handling remains active.
- This revision received C++17 static compilation, compile-time assertions, Clang static analysis and add-on compilation/linking only. No test programs or game sessions were run. The user subsequently confirmed ground marking works. Historical runtime results below apply to the earlier builds they describe.

- The 3.4.5 control changes pass core tests for independent X/Y inversion, gentler defaults, absolute tilt bounds, pitch-independent ground movement and preference retention. Actual-game feel of these new controls still needs user verification.
- The hidden production-input fixture checks saved speed/sensitivity/X/Y settings reload correctly without changing the interface language.

- **Actual-game confirmation, 23 October 2026:** the user confirmed that the camera can move after testing the 3.4.4 diagnostic build in a level. Earlier 3.4.1–3.4.3 attempts did not move the scene despite changed controller values. The working change follows CPU uploads into GPU-only constant buffers.
- Release cleanup removes temporary diagnostic counters, raw constant-buffer samples, diagnostic file writes and per-draw observation callbacks. It preserves the camera upload and projection behavior of the confirmed build.
- Hidden native DX12 tests cover a 4096-byte DEFAULT-heap camera, alternating upload buffers, late descriptor filling, 5000-entry descriptor copies, projected world paths, view restoration and resize. Separate coverage exercises CPU-visible GPU Upload heaps and cached camera constants.
- Hidden input tests cover single T enable/disable, WASD while hovering an inactive panel, right drag, wheel, background-input isolation, text entry, resize and Esc. Test messages target only the test window; no desktop focus changes or system-wide input are generated.
- Core model, grid/projection/detection, language and camera math suites cover valid inverses, height/orbit/zoom, reflected matrix relationships and 49 grid snap points.
- The user's confirmation establishes camera movement, not exhaustive validation of every level, height/zoom interaction, path alignment, streaming, culling, shadows, reflections or game-side mouse picking. These broader actual-game behaviors remain unverified; keep the experimental/Beta label.
- Player ZIP contains manual installation/uninstallation guides in English and Chinese; no CMD/BAT/PS1/EXE installers. Required input configuration: InputProcessing=1. Private test fixtures and diagnostic data are excluded.

## Earlier validation history

### 3.3 Beta

The manual-install packaging revision omits installer scripts. It retains the previously validated 3.3 add-on binary; prior installer checks below apply only to the historical scripted package, not to this manual distribution.

The manual ZIP is checked for archive integrity, payload hashes, unchanged add-on/runtime/font binaries, required InputProcessing=1 configuration, bilingual quick-start guides, and the absence of installation scripts, standalone executables, nested archives and local installation/draft data. Manual upgrades do not perform automatic backups or conflict checks.

The 3.3 input update is validated separately from the earlier rendering/grid work below. Full keyboard/mouse coexistence in an actual game session still needs user verification.

- Core suites passed: model, projection/grid detection, localization, and focus/input policy.
- Native ReShade 6.8.0 / DX12 fixture: 240 frames, resize, device-removed status 0. The production overlay was exercised with deterministic focus changes; drawing intent, interrupted-stroke commit/save, unfinished text preservation (including runtime recreation), and separate capture flags passed. The fixture simulates focus rather than moving the desktop's actual focus.
- Messages sent only to the hidden test window: W/A reached the host while drawing, and W reached it again after runtime recreation; left-click messages did not. A did not select the arrow tool. No desktop input was generated.
- Production add-on smoke test: 240 frames with the grid fixture and resize, device-removed status 0, existing annotations rendered into the native back buffer.
- Installer: known 3.2 upgrade with InputProcessing=2, 0, absent key, or absent section; original config/add-on/draft backup, custom font retention, idempotent reinstall, uninstall, draft preservation, and fresh installation passed. When an existing routing key was present, config bytes changed only for its value.
- Required routing: `[INPUT] InputProcessing=1`. See the [ReShade 6.8.0 input-routing implementation](https://github.com/crosire/reshade/blob/v6.8.0/source/runtime_gui.cpp#L1405). The historical installer migrated this setting; the current manual package requires the user to set it during upgrades.

## Earlier 3.2 validation

- Translation catalog: complete English/Chinese entries, matching printf argument types, stable window IDs, system-language defaults and explicit overrides.
- Both UI languages rendered with the bundled Noto font in the DX12 test host: 240 frames each, resize, native back-buffer readback and device-removed status 0.
- Live English-to-Chinese and Chinese-to-English changes written through the real settings function; each next startup read the persisted choice.
- Grid fixture: 256 centers, spacing 1, matching camera frame. Existing world projection/draft validation remains applicable; the language update does not change the draft format.
- Windows PowerShell installer: fresh install, reinstall, versions 2.1/3.0/3.1 upgrades, old binary/draft backup, configuration and language preference preservation, uninstall, repeated uninstall, conflicting proxy rejection, tampered font rejection, English and Chinese output.
- Automatic discovery: secondary Steam library with Unicode and spaces, without an explicit game path.
- Release: payload hashes and ZIP integrity checked; no personal installation record, drafts, logs, probes, game assets, or nested archives.

UI preview images are from the synthetic DirectX 12 test host, not actual game levels. The preview build forces the toolbar open to inspect layout; the distributed add-on is the normal production build. Full visual alignment across actual game levels remains unverified. Keep the Beta label and check the cell-center preview in your level.

Naming update: display strings now use SinkingStarSketchPad. Executable code, file size, PE headers, persistent paths and window IDs are unchanged. The production add-on and both localized UI previews passed native DX12 loading/rendering checks; upgrading from the preceding 3.2 package preserves existing data and preferences.

## Upstream integration

3.4.6 explicit camera holding/defaults are integrated with native-scene resets, effective eye/forward projection and local drawing input. Full fonts/manual packaging are retained. Static compilation/analysis/linking only; no runtime test or packager is executed.
