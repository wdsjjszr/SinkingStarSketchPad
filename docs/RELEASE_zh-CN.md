# GitHub 源码与 Release 发布

本 fork 的仓库是 https://github.com/wdsjjszr/SinkingStarSketchPad，基于 ouyangzhixuan 的同名项目。首次发布包版本为 `3.4.6-fork.1`，保持 Beta 标记；原项目 MIT 声明保留，并追加 fork 修改署名。

## 一次性准备外部依赖

在项目根目录创建 `.tools/release-deps/`，放入：

```text
.tools/release-deps/
  ReShade64.dll
  NotoSansCJKsc-Regular.otf
  NotoSansCJKsc-Bold.otf
```

- DLL 使用 [ReShade 官方](https://reshade.me/)的 **6.8.0、64 位、完整插件支持版**。已有同版本安装可复制其 `dxgi.dll`，在依赖目录中命名为 `ReShade64.dll`。不要带入其他游戏 DLL。
- 两款字体使用 [Noto 官方原文件](https://github.com/notofonts/noto-cjk/tree/main/Sans/OTF/SimplifiedChinese)，保留字体元数据中的版权信息。不要替换成 Windows 系统字体。
- 字体 OFL、ReShade/ImGui、ReShade 内含库和编译运行库的原始许可文本已放在 `third_party/`，打包时自动附带。
- `.tools/`、`build/`、`dist/` 和 ZIP 均被 Git 忽略。不要使用 `git add -f` 将它们加入源码。

## 构建完整 ZIP

需要 PowerShell、Python 3 和 README 所述 LLVM-MinGW。项目根目录运行：

```powershell
.\test.ps1
.\package.ps1
```

`test.ps1` 只做静态编译检查；`package.ps1` 调用 `build.ps1` 重新编译，再运行 `scripts/package.py` 打包，不运行游戏或测试程序，也不做 SHA 校验。可通过 `-Compiler 'C:\path\clang++.exe'` 或 `-Python 'C:\path\python.exe'` 指定程序。

生成文件：`dist/SinkingStarSketchPad-3.4.6-fork.1-Manual.zip`。重复构建会在成功写完临时 ZIP 后替换同名产物。

包内主要内容：

```text
game_files/
  SinkingStarSketch.addon64
  dxgi.dll
  ReShade.ini
  Sketch-ReShadePreset.ini
  SinkingStarSketch-assets/
    NotoSansCJKsc-Regular.otf
    NotoSansCJKsc-Bold.otf
LICENSE
THIRD_PARTY_NOTICES.md
licenses/
INSTALL_EN.txt
安装说明.txt
README_EN.md
README_zh-CN.md
CHANGELOG.md
GRID_DRAWING.md
MANUAL_GROUND.md
NATIVE_GROUND_EXPERIMENT.md
VALIDATION.md
package.json
```

包内使用仓库的初始配置，不复制个人 ReShade 配置、草稿、偏好、日志、游戏文件、测试程序或编译器。草稿目录和兼容插件文件名保持原样。`config/SketchDepth.fx` 仍是未接入正式插件的实验素材，不随玩家包发布。

也可以先运行 `build.ps1`，再用 `python scripts/package.py --runtime 'C:\deps\ReShade64.dll' --font 'C:\deps\NotoSansCJKsc-Regular.otf' --bold-font 'C:\deps\NotoSansCJKsc-Bold.otf'` 从其他位置提供依赖。

## 推送源码与创建 Release

先按功能提交源码，再运行 `.\Publish-To-GitHub.ps1`。它只将已经提交的 `main` 推送到本 fork，不自动暂存、提交、改分支、打包或创建 Release。

在自己的 GitHub 仓库中创建 Release，标签建议 `v3.4.6-fork.1`，选择已推送的 `main`，并附上上述完整 ZIP。发布说明应注明基于上游 3.4.6、fork 新增功能和当前验证范围。GitHub 自动生成的 Source code ZIP 仅包含源码，不能作为玩家安装包。

公开接收问题前，在仓库 Settings → General → Features 中启用 Issues；fork 默认可能关闭该功能。现有 `build/` 下的历史调试包和 UI 更新包不作为本次完整 Release 的附件。
