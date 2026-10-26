# Third-party components

The root MIT license applies to mod-specific source code and documentation. It does not replace the licenses of third-party components.

This fork is maintained by wdsjjszr and is based on [ouyangzhixuan/SinkingStarSketchPad](https://github.com/ouyangzhixuan/SinkingStarSketchPad). Both copyright notices are retained in [LICENSE](LICENSE).

| Component | Use | License / source |
| --- | --- | --- |
| ReShade 6.8.0 headers | Vendored build dependency | BSD 3-Clause, [notice](third_party/reshade-sdk/LICENSE-ReShade.md), [source](https://github.com/crosire/reshade/tree/v6.8.0) |
| Dear ImGui 1.92.5 docking headers | Vendored UI build dependency | MIT, [notice](third_party/reshade-sdk/LICENSE-ImGui.txt), [source](https://github.com/ocornut/imgui/tree/v1.92.5-docking) |
| ReShade 6.8.0 full add-on runtime | External packaging/runtime dependency | BSD 3-Clause; bundled library notices in [runtime notices](third_party/reshade-runtime/SOURCES.md), [official download](https://reshade.me/) |
| Noto Sans CJK SC Regular / Bold | External font for bilingual packages | SIL Open Font License 1.1, [local license](third_party/noto-cjk/LICENSE.txt), [source](https://github.com/notofonts/noto-cjk/tree/main/Sans) |
| LLVM-MinGW | External build toolchain | Its compiler/runtime notices are copied from the validated release into `third_party/runtime-notices`; [official project](https://github.com/mstorsjo/llvm-mingw) |
| ootss-cmd | Reference for scene location, Jai reflection, logical character positions, named ACTIVE selection and X/Y/Z/entity-ID ordering used by native ground marking | [Project](https://github.com/qiekn/ootss-cmd), [custom permissive license](third_party/reference-notices/ootss-cmd-LICENSE.txt), [selection reader](https://github.com/qiekn/ootss-cmd/blob/main/src/game/position_reader.jai) |
| joric/ootss | Reverse engineering and static inspection references | [Project](https://github.com/joric/ootss), [Unlicense](third_party/reference-notices/joric-ootss-UNLICENSE.txt) |

Native ground marking and its Debug preview also use the reverse engineering notes in [joric/ootss Progression Rules](https://github.com/joric/ootss/wiki/Progression-Rules). The terrain field layout was checked by statically reading the installed game executable; game code and resources are not bundled. Character selection follows ootss-cmd's read-only approach; no teleportation or character writes are included.

Runtime DLLs, font binaries, compiler binaries, game resources and Windows fonts are not stored in this source repository. A redistributable package must include the relevant notices for any components it bundles. The release packager includes the root MIT license, ReShade and ImGui licenses, Noto OFL, runtime library notices and reference licenses. ReShade runtime notices also cover MinHook, glad, UTF8-CPP, stb, SPIR-V headers, Vulkan Memory Allocator, OpenVR/OpenXR, DirectX headers, the simple lossless encoder, ImGuiColorTextEdit, Solarized and Fork Awesome.

The bilingual UI uses the same unmodified Noto Sans CJK SC Regular file for ReShade's `Font` and `LatinFont`. It includes Latin glyphs as well as CJK glyphs. Distribute the original font with its copyright notice and [SIL OFL 1.1 license](https://github.com/notofonts/noto-cjk/blob/main/Sans/LICENSE); the font remains under OFL and the mod-specific code remains MIT. The OFL explicitly permits bundling fonts with open-source software; see the [official OFL FAQ](https://openfontlicense.org/ofl-faq/). Do not substitute a Windows system font in a redistributable package.

Noto Sans CJK SC Bold is loaded through ReShade's separate EditorFont slot for true font-weight emphasis; both original font files remain under the same SIL OFL 1.1 license. Packages include the original license and retain Adobe's copyright information in both unmodified font files' metadata.
