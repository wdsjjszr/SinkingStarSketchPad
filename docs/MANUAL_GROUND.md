# 原生地面自动／手动标识 / Native ground marking

## 中文

地面格来自当前关卡的原生逻辑数据：实心墙体的暴露顶面、Settings 基准底层、blocking 地板 Fence。格距固定为 1 世界单位，格心 X/Y 为整数；不再从 GPU 实例推断或打分。

### 自动标识

1. 进入关卡，按 **E** 打开绘图工具栏。每页默认使用 **自动标识**，**随角色移动** 默认不勾选。
2. 首次取得数据时，以当前控制角色的位置确定目标平面。优先找角色同一 XY 格内高度最近的地面；同列没有地面时，找全场三维距离最近的地面格。距离相同优先较低处。
3. 多角色同时受控时，采用 ootss-cmd 的方法：只考虑带 `Status_Flags.ACTIVE` 的角色，按 X、Y、Z、实体 ID 升序选择一个；每次采样重新检查控制状态。
4. 勾选 **随角色移动** 后立即更新，并随角色或当前控制角色的变化更新平面。取消勾选后固定在当前高度。在绘图状态下，画布空白处 **中键** 可切换该开关。

角色位置约每 33 毫秒采样，地形约每秒刷新；地形重建期间角色采样会等待。未读取到角色或地面时暂停自动更新，已确定的同场景平面保留；读取恢复后继续。

### 手动标识

切换到 **手动标识** 时保留当前目标平面，不会立即进入标记。点击 **标记平面** 会选择路径工具并进入标记模式；悬停可预览地面格和高度。

| 操作 | 选中地面 | 墙面遮挡或空处 |
| --- | --- | --- |
| 标记模式中左键 | 设置目标平面，自动结束标记 | 提示原因，继续等待标记 |
| 手动模式中键 | 快速标定一次，结束标记 | 提示原因，结束标记 |

中键快速标定保留当前绘图工具，本次点击不会添加路径点。若此前正在标记，中键尝试后也会结束标记。Esc、切页、切换工具或再次点击取消按钮可以取消标记。切回自动模式会重新按角色位置确定目标，保留该页的跟随开关。

只选相机射线首先可见的原生地面；先碰到实心墙的侧面或底面会提示遮挡，不会选择墙后的地面。墙顶本身可以选中。滚轮不再切换重叠高度，保持自由相机缩放功能；点选使用自由相机当前有效投影。

### 页面、焦点和保存

三个草稿页的模式、跟随开关和目标平面彼此独立，仅保留在本次会话中。重启后各页恢复自动、不跟随，并重新初始化。换关卡会保留各页模式和跟随开关，但清除目标：自动页重新初始化，手动页等待重新标记。

真正更换平面时结束当前路径；旧路径保留各自的高度和格子基准，不随目标移动。草稿文件格式不变。

中键仅在绘图编辑状态、游戏前台、指针位于画布时生效。指针在工具栏或快捷键说明窗口内、编辑文字、操作控件或打开 ReShade 菜单时不触发。切回游戏后先松开所有鼠标按键。

### 原生 Debug 与限制

**Debug：预览原生地面格** 只控制显示；地面读取始终为正式标识服务。可按高度筛选，悬停查看来源、实体 ID 和 XYZ。筛选不会限制正式选面。彩色实心点为墙顶、空心圈为基准底层、十字为地板；Debug 覆盖层仍会透视墙体显示。

- 遮挡仅检查实验已读取的原生实心墙体，不检查装饰模型、角色等其他可见物体。
- 基准底层是逻辑参考面，可能没有可见地形；墙顶可选不代表角色可达。
- 尚未包含升降台、莲叶、其他额外支撑及世界环绕的渲染副本。不复刻完整移动、技能和可达性规则。
- 用户已确认 Fix1 实验在所测关卡中正确显示网格；本次角色跟随和墙体点选只做静态检查、编译链接，未执行测试程序或游戏。

诊断文件 `SinkingStarSketch-data/status.txt` 包含 `ground_mode`、`ground_follow`、`ground_marking`、`ground_ready`、`ground_z`、`ground_layers`、`ground_character_ready`、`ground_character_id`、`ground_controlled_count`、`ground_character_position`、`ground_character_error` 及 `native_ground_*`。旧 GPU 输入点 Debug 已移除。历史实验记录见 [NATIVE_GROUND_EXPERIMENT.md](NATIVE_GROUND_EXPERIMENT.md)。

## English

Ground comes from native scene data: exposed solid-wall tops, Settings layer bases and blocking floor Fences. Spacing is one world unit, with integer X/Y cell centers. GPU instance scoring is no longer used.

### Automatic marking

Each page starts in **Automatic marking**, with **Follow character** off. The first valid sample sets the target from the controlled character. Ground in the same XY cell takes priority, choosing the nearest height; if that column has no ground, choose the nearest cell in 3D. Equal distances prefer lower ground.

Character selection follows ootss-cmd: recheck `Status_Flags.ACTIVE` each sample and, among controlled characters, choose the first by X, Y, Z, then entity ID. Positions refresh about every 33 ms; terrain refreshes about once per second. Terrain rebuilding can delay character samples. Reading never writes character memory.

Enable **Follow character** to update immediately and keep following the selected character's ground. Disable it to freeze the current plane. **Middle-click on the canvas while drawing** toggles following. Temporary read failures pause updates and preserve an established target in the same scene.

### Manual marking

Switching to **Manual marking** retains the target. Click **Mark plane** to select the path tool and preview a ground cell. Left-click sets the plane and exits on success; a wall or empty location shows a message and keeps marking active. Esc, switching page/tool or the cancel button cancels marking.

In manual mode, **middle-click** makes one attempt and exits marking on either success or failure, preserving the selected tool and adding no path point. Switching back to automatic immediately requests a new character-based target and retains that page's following preference.

Picking uses the current effective camera, including free camera. A solid wall's side or underside blocks ground behind it; an exposed wall top can be selected. Overlapping floors cannot be cycled through with the wheel. The wheel remains available for free-camera zoom.

### Pages, focus and limits

Modes, following preferences and targets are independent per page and last for this session only. Restarting restores automatic mode with following off. Changing levels clears all targets while retaining modes, preferences and existing paths: automatic pages reinitialize, manual pages wait for another mark. Changing the target ends the active path; existing paths retain their own lattice and height. The draft format is unchanged.

Middle-click only acts in drawing mode, with the game foreground and the pointer on the canvas. Panels, text entry, active widgets and the ReShade menu suppress it. Release held mouse buttons after returning to the game.

**Debug: preview native ground cells** controls display only. Height filtering does not restrict picking. Dots are voxel tops, rings are logical layer bases and crosses are floor Fences. Hover for source, entity ID and XYZ. Debug still draws through walls.

Occlusion covers native solid voxels only. Decorative models, characters, elevator platforms, lilies, extra supports and world-wrap render copies are not included. Logical bases need not have visible terrain; selecting a wall top does not establish walkability. No movement, skill or reachability solver is included.

The user confirmed the earlier Fix1 preview in the levels they checked. This integration received static checks and compilation/linking only; no test programs or game sessions were run. Use the `ground_*` and `native_ground_*` fields in `SinkingStarSketch-data/status.txt` for diagnostics. The old GPU input-point Debug was removed.
