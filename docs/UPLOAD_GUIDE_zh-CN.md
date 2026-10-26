# Nexus Mods 发布说明

发布文件：`SinkingStarSketchPad-3.4.6-fork.1.zip`。这是提供给玩家的手动安装完整包，不含 CMD、批处理、PowerShell 或 EXE 安装程序。作者材料目录无需一起上传，也不要上传本机安装后生成的 `installation.json`、存档、日志、诊断 DLL、游戏资源或开发用压缩包。

建议标题：**SinkingStarSketchPad - In-Game Drawing and Cell Paths**

简短英文介绍：**An in-game scratchpad with a themed cursor, drawing tools, saved pages and cell-snapped route marks. English and Simplified Chinese.**

版本：**3.4.6-fork.1**；发布状态建议选择 **Beta**。在对应游戏分类下发布，未找到游戏分类时先按 Nexus 的游戏收录流程处理，不要放到无关游戏。原作者为 ouyangzhixuan，fork 维护者为 wdsjjszr；项目源码和文档已选择 MIT 开源，请在 Nexus 页面保持与 MIT 一致的修改、转载和资源使用权限，第三方组件另按各自许可处理。

正文可使用同目录的 `NEXUS_DESCRIPTION_EN.md` / `.txt`，完整英文使用文档已在玩家包中。建议展示真实关卡里的路径与相机移动前后效果；附带的 UI 预览来自 DX12 测试程序，不是实际关卡截图，若上传请保留这一说明。

## AI 标记和说明

本项目的代码、界面和翻译主要由生成式 AI 生成，按当前 Nexus 规则应使用 **AI-Generated Content**；附带的 AI 生成封面和页面文字还对应 **AI Media**。不要仅用 AI Assisted 代替。英文介绍已包含如实说明。

依据：[Nexus File Submission Guidelines — Generative AI Tagging & Categorisation](https://help.nexusmods.com/article/28-file-submission-guidelines)。规则会更新，请在提交时再次查看。

## 已准备的组件和验证说明

- 主包包含本地编译的插件、ReShade 6.8.0 完整插件运行库、配置、Noto 字体及第三方许可。
- 玩家退出游戏后手动复制 game_files 内容；升级保留原配置和草稿，手动设置 InputProcessing=1。无自动检测、备份或冲突检查。
- 本版新增实验性自由相机，已验证合成 DX12 场景的几何变化、路径对齐和恢复原视角；实际关卡裁剪、阴影和反射仍需实测。早前安装器测试仅适用于历史发布包。
- 用户已确认 3.4.4 修订版在实际游戏中能移动镜头；3.4.6 的降速、俯仰限制和轴向选项已通过后台测试，实际手感仍待验证。完整关卡视觉对齐未全面实测。页面不要宣称兼容所有关卡、自动寻路、提升帧数或修复游戏性能。
- ZIP 不加密，不包含内嵌压缩包。[Nexus 扫描说明](https://help.nexusmods.com/article/117-why-has-my-mod-been-quarantined)指出内嵌压缩包会影响扫描。

旧安装器版已被 Nexus 隔离。新手动包尚未上传；移除安装脚本不保证通过扫描，包内仍包含必要的原生插件与 ReShade DLL。保留原隔离文件并向审核人员说明新旧包差别。GitHub 开源仓库另见下文。

## 开源仓库

本 fork 由 **wdsjjszr** 维护，基于 [ouyangzhixuan 的原项目](https://github.com/ouyangzhixuan/SinkingStarSketchPad)。保留原作者版权；fork 修改部分版权所有 2026 wdsjjszr，继续采用 MIT。

项目源码和文档采用 **MIT** 许可证：[https://github.com/wdsjjszr/SinkingStarSketchPad](https://github.com/wdsjjszr/SinkingStarSketchPad)。第三方组件仍按各自许可分发。

## 上游相机整合 — 3.4.6

尚未保存对应设置时，速度默认 5、反转 Y 开启；已有速度及轴向偏好保留。T/Esc 退出操作并恢复游戏视角，只有“固定相机”按钮会保留机位并释放输入；T 可继续调整，Esc 或“恢复游戏视角”可归位。机位不跨重启保留，切换原生关卡会清除。绘画、地面标记及体素裁剪继续使用有效相机的位置和朝向。仍采用附字体的完整手动包。
