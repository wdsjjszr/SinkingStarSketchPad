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

### Debug 坐标点

按 E 打开菜单，在地面标识选项下勾选 **Debug：显示识别输入点**。该开关独立于手动标识模式，开启后可继续点选地面，也可关闭菜单观察场景。

- 彩色小点：当前进入网格识别的全部输入坐标，按高度分色；尚未通过网格评分的点也会显示。
- 白色圆环：最终候选网格的可选格心。缺失区域如果有彩色点而没有白圈，说明已采集到点，但未形成可选网格。
- 悬停彩色点可查看世界坐标 XYZ；菜单显示总输入点数、屏幕内输入点数和最终候选格心数。重复输入可能重叠，计数保留重复项。
- 输入与识别结果使用同一份约每秒更新的快照。Debug 不再抽样或限制显示点数；仍然只包含现有采集链路通过实例矩阵筛选并送入识别的点，不是场景的全部顶点。被墙体遮住的点也会显示。

`status.txt` 额外记录 `ground_input_points`、`ground_debug` 和 `ground_debug_visible`。点数较多时 Debug 绘制可能降低帧率，可随时取消勾选。

### 原生地面格 Debug 预览

按 E 打开菜单，勾选 **Debug：预览原生地面格**。建议先关闭旧的“显示识别输入点”，方便对照。无需启动 ootss-cmd。默认关闭；开启后约每秒读取一次当前场景，关闭后停止读取。

- 格距固定为游戏中的一格，即 1 世界单位；颜色按世界 Z 区分，可在 **预览高度** 中只显示一个高度。
- **实心点与格框**：当前 `Wall_Set.wall_data` 的实心体素顶面。位图按 X、Y、Z 顺序展开，加上 Wall_Set 的逻辑位置与体素 corner，再取上方一格作为候选站立高度。相邻体素间的内部顶面不显示；同一 XY 可以保留多个 Z。
- **空心圈**：Settings 给出的地表 / 地下基准底层，在 Settings 网格范围内显示，剔除占用格与相应水域。它是游戏逻辑参考平面，不表示该处画出了地形，也不是地形轮廓。天空基准不生成隐式地面。
- **十字**：同时具有 `blocking` 与 `is_floor` 的 Fence，在自身所在格形成的地板。
- 悬停格心显示 XYZ、来源和实体 ID；隐式底层没有来源实体，ID 为 0。菜单同时显示场景名、候选格总数、屏幕内格数、高度数和原始来源数量。
- 预览独立于现有的自动识别和手动锚定，暂不改变路径平面。点和格框会穿过遮挡物显示，墙顶也属于候选，不能据此判断角色可达性。升降台、莲叶和世界环绕的额外渲染副本暂不包含。

实现参考 ootss-cmd 的当前场景特征与 Jai 反射字段读取。只读取游戏内存，不修改游戏数据；读取失败或场景切换时丢弃旧候选并显示原因。当前零售版结构经过本机 EXE 与关卡文件静态核对，游戏更新改变场景代码特征时可能需要维护。

原生地面从当前场景的完整实体 ID 表枚举对象，跳过空槽和已删除槽。用于寻找角色的对象列表不包含全部静态对象，不能用它查找 Settings 或完整地形。

`status.txt` 新增 `native_ground_debug`、`native_ground_ready`、`native_ground_scene`、`native_ground_cells`、`native_ground_visible`、`native_ground_heights`、`native_ground_voxels` 和 `native_ground_error`。反馈缺失地面时，请同时提供场景名、预览高度、截图及该状态文件。

开发侧仅做静态检查与编译，没有运行测试程序或游戏。Fix1 交付后，用户人工确认已测试关卡中的网格均能正确显示。实现原理、Settings 修复与验证范围见 [原生地面格实验技术说明](NATIVE_GROUND_EXPERIMENT.md)。

## English

This experimental build retains grid candidates at different heights and lets you lock one for new paths on the current scratch page.

1. Exit the game and replace only `SinkingStarSketch.addon64` with this build. Keep the existing ReShade, font, settings and drafts.
2. Enter a level, press E and click **Mark ground manually** below the existing toolbar options. This also selects the path tool.
3. Hover a floor to preview its cell and nearby detected centers. The tooltip shows the height and the number of overlapping candidates.
4. Left-click to anchor the previewed floor. Keep the pointer still and use the wheel to switch overlapping layers, then click to confirm. You can anchor several floors in succession.
5. Press Esc or click **Finish ground marking**, then click to draw paths. **Use automatic ground** restores the highest-scoring grid.

Each scratch page has its own session-only lock. Switching floors finishes the current path; existing paths retain their positions and heights. Restarting restores automatic selection without changing saved paths. Free-camera movement and right-drag rotation remain available; its wheel zoom resumes after marking ends.

Selection uses observed grid cells, without scene-depth occlusion or entity classification. Walls can hide a selectable candidate. Each layer still needs at least six centers and the existing two-dimensional adjacency checks, so small or uncaptured platforms may be unavailable. Check alignment visually in the actual game. This build has only been checked statically and compiled; no runtime tests were executed.

Enable **Debug: show detection input points** below the ground options to display every input coordinate from the existing capture pipeline, including points rejected by grid scoring. Colors distinguish heights; white rings show selectable candidate cells. Hover a colored point for XYZ. The toolbar reports total inputs, on-screen inputs and candidate cells, counting repeated inputs separately. Capture and detection share the same snapshot, refreshed about once per second. Debug adds no sampling limit, but it only shows points already accepted by the existing instance-matrix capture filters. It ignores wall occlusion, remains visible when the toolbar is closed, and may reduce frame rate with large point sets.

### Native ground Debug preview

Press E and enable **Debug: preview native ground cells**. Disable the older input-point display for a clearer comparison. ootss-cmd does not need to run. Reading starts only when enabled and refreshes about once per second.

Cell spacing is one world unit. Colors distinguish world Z; **Preview height** can isolate a single height. Filled dots and cell outlines show exposed tops of solid `Wall_Set` voxels, including wall tops. Rings show the Settings ground / underground reference floor within the Settings grid, excluding occupied and water cells; they do not trace visible terrain. Crosses show blocking floor Fences. Hover a center for XYZ, source and entity ID (0 for an implicit floor). Multiple surfaces can share XY.

This separate preview leaves existing detection, manual marking and paths unchanged. It draws through occluders and does not determine reachability. Elevator platforms, lilies and extra world-wrap render copies are not included. Reading uses the current-scene code signature and reflected Jai fields, with old candidates cleared on failure. It never writes game memory. The retail executable and level data were inspected statically; future game changes may require updating the scene signature.

Entities are enumerated from the scene's complete entity-ID table, skipping empty and deleted slots. The object list used to find characters omits some static entities, including Settings.

For missing surfaces, report the scene, selected height, screenshot and `native_ground_*` entries in `SinkingStarSketch-data/status.txt`. Development validation was limited to static checks and compilation; no test programs or game sessions were run by the developer. After Fix1, the user confirmed correct grid display in the levels they checked. See the [experiment technical note (Chinese)](NATIVE_GROUND_EXPERIMENT.md) for implementation details and validation scope.
