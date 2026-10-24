# 手动标识地面 / Manual ground marking (experimental)

## 中文

这是供真实关卡人工验证的候选地面选层功能。自动识别会保留不同高度的候选网格，手动模式可选择其中一层作为当前草稿页的新路径平面。

1. 正常退出游戏，用本次构建的 `SinkingStarSketch.addon64` 替换原有同名插件，保留现有 ReShade、字体、设置和草稿。
2. 进入关卡，按 E 打开菜单，点击现有选项下方的 **手动标识地面**。该按钮同时选择路径工具。
3. 移动鼠标查看青色格子和附近格心；悬停提示会显示高度和鼠标下的候选层数量。
4. 左键锚定当前预览平面。上下层重叠时，保持鼠标位置并滚动滚轮选择另一层，再左键确认。可以连续点击不同高度的地面。
5. 按 Esc 或点击 **结束地面标识**，随后点击地面绘制路径。点击 **恢复自动地面** 可重新使用评分最高的网格。

每个草稿页独立锁定平面。切换平面会结束当前路径，已有路径保留各自的位置与高度。锁定状态仅在本次游戏会话内有效；重启后恢复自动识别，已保存路径的高度不变。

自由相机可继续移动和右键旋转。标识模式中，滚轮用于选层；结束标识后恢复相机缩放。

当前限制：

- 根据实例推断的格心和高度选层，尚未接入场景深度，不判断墙体遮挡，也不识别实体种类。请以网格预览是否贴合目标地面为准。
- 只在候选网格实际观测到的格子范围内提供点选；没有候选格子的空处不会命中无限延伸的平面。
- 每层仍需满足现有二维网格识别条件（至少六个格心及横纵相邻关系）。很小的平台、零散实例或未捕获的地面可能没有候选层。
- 仅完成静态检查和编译，实际游戏中的地面高度、贴合程度、遮挡和操作手感需人工确认。

`SinkingStarSketch-data/status.txt` 中的 `ground_marking`、`ground_locked`、`ground_z` 和 `ground_layers` 可用于反馈当前状态。

## English

This experimental build retains grid candidates at different heights and lets you lock one for new paths on the current scratch page.

1. Exit the game and replace only `SinkingStarSketch.addon64` with this build. Keep the existing ReShade, font, settings and drafts.
2. Enter a level, press E and click **Mark ground manually** below the existing toolbar options. This also selects the path tool.
3. Hover a floor to preview its cell and nearby detected centers. The tooltip shows the height and the number of overlapping candidates.
4. Left-click to anchor the previewed floor. Keep the pointer still and use the wheel to switch overlapping layers, then click to confirm. You can anchor several floors in succession.
5. Press Esc or click **Finish ground marking**, then click to draw paths. **Use automatic ground** restores the highest-scoring grid.

Each scratch page has its own session-only lock. Switching floors finishes the current path; existing paths retain their positions and heights. Restarting restores automatic selection without changing saved paths. Free-camera movement and right-drag rotation remain available; its wheel zoom resumes after marking ends.

Selection uses observed grid cells, without scene-depth occlusion or entity classification. Walls can hide a selectable candidate. Each layer still needs at least six centers and the existing two-dimensional adjacency checks, so small or uncaptured platforms may be unavailable. Check alignment visually in the actual game. This build has only been checked statically and compiled; no runtime tests were executed.
