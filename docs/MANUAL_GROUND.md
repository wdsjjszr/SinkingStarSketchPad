# 地面平面标记 / Ground plane marking

## 中文

按 **E** 打开绘图工具栏。“设为角色所在平面”和“手动标记”同时展示，无需切换模式，也没有持续跟随开关。

### 角色所在平面

- 进入新关卡时读取一次当前控制角色，三页采用同一个初始高度。直接使用角色逻辑坐标的 **Z 原值**，不找最近地面、不比较或取整高度；即使角色悬空也能设置。
- 点击 **设为角色所在平面** 会重新读取角色，只修改当前页。读取失败时提示并保留已有目标；不依赖原生地形读取成功。
- 多角色选择沿用 ootss-cmd：检查 `Status_Flags.ACTIVE`，在受控角色中按 X、Y、Z、实体 ID 升序选一个。
- 初始角色尚未准备好时，每次关卡检查会重试；成功后停止自动采样。等待期间已手动设好的页不会被迟到的初始结果覆盖。

设置后平面保持固定，角色移动或切换控制角色不会自动更改目标。重启插件、换关卡后重新初始化。

### 手动标记与中键

点击 **手动标记** 会选择路径工具，在后台刷新一次原生地形，然后显示鼠标下的地面格和高度预览。

| 操作 | 成功命中地面 | 墙体、空处或尚未读好 |
| --- | --- | --- |
| 标记模式左键 | 设置平面并退出标记 | 提示原因，继续等待标记 |
| 画布中键 | 快速标定一次并退出标记 | 提示原因，退出标记 |

中键始终标记指针下地面所在平面，保留当前工具，不添加路径点。它保存按下时的指针位置、相机、草稿页和关卡，后台刷新后仍按该次点击计算，不采用等待期间移动后的指针。读取期间暂停画布落笔，避免在旧平面上加点。

Esc、换页、切换工具、退出绘图或新的标记请求会作废旧请求；尚未开始的后台请求也会取消。点击取消按钮可退出手动标记。中键只在绘图编辑状态、游戏前台且指针位于画布时生效；文本输入、活动控件、工具面板和 ReShade 菜单会阻止触发。切回游戏后先松开鼠标按键。

点选使用当前有效相机，包括自由相机。射线根据相机世界位置和朝向定向，不固定假定深度 0 是近端、1 是远端。地面自身的体素顶面不算前方遮挡；先碰到实心墙侧面或底面时提示遮挡。绘图打开时，滚轮行为可在设置中配置：默认无修饰键调高、Shift+滚轮循环切色，Ctrl／Alt 默认无，多修饰键无动作。调高每格将 Z 加 / 减 1、保留小数，取消旧标定请求并保留活动路径和待定网格直线／箭头，不读取角色或地形。切色不依赖平面，也不改变已开始笔迹的颜色。高精度余量在切换组合或失去画布资格时清除。关闭绘图后滚轮用于自由相机缩放。

### 读取、页面和保存

后台空闲时只约每秒读取关卡身份，不再每 33 毫秒采样角色，也不再每秒重建地形。进入新关卡、点击手动标记或按中键时才读取一次完整原生地形；角色读取仅用于初始化及角色按钮。

三个草稿页分别保存目标平面。换关卡会清除旧目标并使用新的初始角色高度；已有路径保留各自的格子基准和高度。非滚轮重新标定会结束当前路径，已有世界笔迹继续使用各自基准。草稿 v4 保存网格和逐点 Z 偏移，兼容读取 v1/v2/v3，升级前保留独立备份；详见[网格绘画说明](GRID_DRAWING.md)。

关卡首次自动初始化不显示提示。提示的新默认值为灰色边框加黑白棋盘、透明度 45%、保持 0.75 秒后另加 30% 缓出渐隐；已有设置保留。四图层、显示策略与保持时长即时保存；同高度标定重新计时，关闭或到期不复活。四图层通过原生体素的屏幕投影裁剪被挡部分，世界单位 1e-4 接触容差保留共面表面且不修改 Z；无有效快照完整显示。范围仍最多 128×128 格，常驻网格预览不挖空，增加径向渐隐。详见[网格绘画说明](GRID_DRAWING.md)。

旧 GPU 地面实例采集与评分仍已移除。备用 `camera_bridge` 的重复采集和回退读取暂时禁用，保留源码；当前有效相机和自由相机必需的 GPU 读取／覆盖继续运行。

### Debug、诊断和限制

原生 Debug 显示最近一次读取的快照，不循环刷新。高度筛选只影响显示，不限制选面；彩色实心点为墙顶、空心圈为基准底层、十字为地板。悬停查看 XYZ、来源和实体 ID。Debug 仍会透视显示。

- 遮挡只检查原生实心墙体，不检查装饰模型和角色；升降台、莲叶、额外支撑与世界环绕渲染副本仍未纳入。
- 基准底层是逻辑参考面，可能没有可见地形；墙顶可选不表示可达。本工具不复刻移动、技能或寻路规则。
- 用户已反馈地面标记可正常使用；此前网格绘画已反馈整体可用；本次设置与预览调整仅做静态检查与编译，未执行测试程序或游戏。

`SinkingStarSketch-data/status.txt` 保留 `ground_marking`、`ground_ready`、`ground_z`、`ground_layers`、`ground_request_pending`、`ground_scene_ready`、`ground_generation`、`ground_character_*`、`ground_controlled_count` 和 `native_ground_*`。角色坐标和错误代表最近一次读取结果，不是实时监视值；模式和跟随字段已移除。`grid_drawing`、`grid_preview_*`、`plane_hint`、`shape_gesture`、`draft_format` 和 `world_strokes` 描述绘图与预览状态；`settings_open`、`hint_*`、`wheel_binding_0..3` 、`ink_color`、`path_active` 和 `right_erasing` 描述设置与交互状态。提示策略按开／关／仅微调编码为 0／1／2，来源按初始化／标记／滚轮编码为 0／1／2；滚轮动作按无／改高／切色编码为 0／1／2。历史实验见 [NATIVE_GROUND_EXPERIMENT.md](NATIVE_GROUND_EXPERIMENT.md)。

## English

Press **E** to show **Set plane to character** and **Manual marking** together. There is no mode switch or continuous-follow option.

On level entry, all three pages initialize once from one controlled-character sample. The plane uses the logical **Z unchanged**, without rounding or looking for ground; hovering characters work too. The character button requests a fresh sample for the current page and preserves the target on failure, independently of terrain availability. Selection follows ootss-cmd: ACTIVE characters ordered by X, Y, Z, then entity ID. Initialization retries until a valid character exists, then stops. A manual choice made while loading is not overwritten by late initialization.

**Manual marking** selects the path tool, refreshes terrain in the background and previews a ground cell. Left-click sets the plane and exits on success; a wall, empty space or pending read shows a message and keeps marking active.

**Middle-click on the canvas** always requests a one-shot ground mark and exits marking on success or failure. It preserves the tool and adds no path point. Pointer position, effective camera, page and level are captured at the click. While waiting, drawing pauses. Esc, switching page/tool, leaving drawing or a newer request invalidates the old result; queued work is cancelled when possible. Text, active widgets, panels, loss of focus and the ReShade menu suppress the shortcut. Release held mouse buttons after returning to the game.

Rays use the effective camera position and forward direction, including free camera, instead of assuming depth 0-to-1 is forward. A floor's own voxel top is not an occluder; a solid wall before the floor blocks selection. Four configurable wheel bindings apply while drawing is open. Defaults are plain wheel for whole-unit height changes, Shift for palette cycling, Ctrl/Alt for None; multiple modifiers do nothing. Height actions cancel old marking requests but preserve active paths and waiting grid lines/arrows without reading terrain or characters and preserve fractional Z. Color changes affect future marks only and need no plane. Fractional wheel input resets when binding/ownership changes. Close drawing for camera zoom.

Idle work only probes level identity about once per second. Terrain is read on level entry and manual/middle-click requests; characters are sampled for initialization and the character button. Targets then remain fixed. Each page stays independent; new levels reset targets but existing paths retain their own lattice and height. Draft v4 stores grids/relative point Z and reads v1/v2/v3, preserving a separate non-overwriting legacy backup before upgrade. See [grid drawing](GRID_DRAWING.md). Automatic initialization stays silent. Settings controls hint layers, opacity, policy and hold duration. New defaults are gray borders/black-white checker, 45% and a 0.75-second hold plus a 30% ease-out fade; saved preferences remain. Hints clip all primitives against projected native voxel faces, preserving coplanar contact with a 1e-4 tolerance without changing Z. The 128×128 cap remains. New intents/pages/levels clear hints, and same-height marks restart the timer. Persistent previews use radial fading without wall cutting.

GPU ground scoring remains removed. The duplicate backup camera collector is disabled with its source retained; effective-camera and free-camera GPU handling remains active. Native Debug displays the last snapshot and still draws through walls. Height filtering affects display only.

Occlusion covers native solid voxels only. Decorative models, characters, platforms, lilies, extra supports and world-wrap render copies are not included. Logical bases need not have visible terrain; a selectable wall top is not a walkability result.

The user confirmed ground marking now works. The preceding grid drawing revision was reported usable. This settings/preview revision received static checks, compile-time assertions, static analysis and compilation/linking only; no test programs or game sessions were run. The `ground_*` / `native_ground_*` diagnostics describe the latest requested samples, not live character tracking.
