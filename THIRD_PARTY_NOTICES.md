# Third-party components

The root MIT license applies to mod-specific source code and documentation. It does not replace the licenses of third-party components.

| Component | Use | License / source |
| --- | --- | --- |
| ReShade 6.8.0 headers | Vendored build dependency | BSD 3-Clause, [notice](third_party/reshade-sdk/LICENSE-ReShade.md), [source](https://github.com/crosire/reshade/tree/v6.8.0) |
| Dear ImGui 1.92.5 docking headers | Vendored UI build dependency | MIT, [notice](third_party/reshade-sdk/LICENSE-ImGui.txt), [source](https://github.com/ocornut/imgui/tree/v1.92.5-docking) |
| ReShade 6.8.0 full add-on runtime | External packaging/runtime dependency | BSD 3-Clause, [official download](https://reshade.me/) |
| Noto Sans CJK SC Regular | External font for bilingual packages | SIL Open Font License 1.1, [source and license](https://github.com/notofonts/noto-cjk/tree/main/Sans) |
| LLVM-MinGW | External build toolchain | Its compiler/runtime notices are copied from the validated release into `third_party/runtime-notices`; [official project](https://github.com/mstorsjo/llvm-mingw) |
| ootss-cmd | Reference for the current-scene code signature, entity list and Jai reflection layout used by the native ground preview | [Project](https://github.com/qiekn/ootss-cmd), [permissive license](https://github.com/qiekn/ootss-cmd/blob/main/LICENSE) |

The native ground preview also uses the reverse engineering notes in [joric/ootss Progression Rules](https://github.com/joric/ootss/wiki/Progression-Rules). Its field layout was checked by statically reading the installed game executable; game code and resources are not bundled.

Runtime DLLs, font binaries, compiler binaries, game resources and Windows fonts are not stored in this source repository. A redistributable package must include the relevant notices for any components it bundles. The release packager copies the original notices and requires the font license as an input.
