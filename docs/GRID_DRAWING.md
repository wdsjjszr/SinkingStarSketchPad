# 网格绘画、预览与草稿升级 / Grid drawing

## 中文

按 **E** 打开绘图菜单。默认仍是普通屏幕绘画，点击 **网格绘画** 后，新笔迹绘制在当前标定的平面上，随游戏或自由相机重新投影。切换模式和重新标定都不会移动已有笔迹；每条笔迹保存创建时的平面、工具、颜色和笔宽。缺少有效相机或平面时，拒绝新建网格笔迹并提示原因。

### 预览与设置

**网格预览** 的中心点、网格交点、格子边框、棋盘填充可以独立叠加，统一透明度为 0–100%，默认 45%，初始只显示中心点。范围为指针附近 **11×11 格**；普通绘画时仅路径工具显示，网格绘画时所有工具都可显示。常驻预览不检查墙体，不挖空。

两种网格预览统一采用浅灰色 RGB(190,190,190)。交点位于半格偏移处，改成沿平面网格轴的小十字，每条臂长 0.12 格，随透视变化；边框为 1 像素，十字为 1.5 像素。

点击工具栏 **设置** 打开可移动、可调整大小的窗口，再次点击或使用关闭按钮可关闭。E 随绘图菜单隐藏 / 恢复窗口，保留本次会话的打开状态。修改即时生效并自动保存到 `settings.ini` 的 `[Drawing]`，无需点击保存，保留语言、相机及常驻预览偏好。

**平面变动预览** 的默认图层为边框和棋盘开启、中心点和交点关闭，透明度默认 25%。显示策略可选：

- **开**（默认）：角色按钮、手动标记、中键标记或滚轮改高成功后显示。
- **关**：不显示。
- **仅微调**：仅响应配置为调整高度的滚轮操作。

进入关卡仍自动初始化平面，但首次初始化始终不提示。时长默认 **1 秒**，滑块范围 **0–5 秒**，Ctrl+点击可输入到 0.01 秒，0 秒不显示。已有提示中的图层和透明度立即更新，时长按原起始时间重新计算；关闭或到期的旧提示不会因调长时长而重新出现。同高度主动标定重新计时，新意图、换页、换关卡清除旧提示，异步旧结果不会恢复旧提示。

短时提示按**当前有效相机的格心射线**判断遮挡，包括自由相机：原生实心墙体位于相机与格心之间则整格隐藏，自身顶面接触不算遮挡。四个图层共用可见格掩码，隐藏格不绘制填充和独立边线，交点十字只沿可见格相邻边绘制。不要求目标高度实际存在原生地面；没有有效同关卡体素快照时完整显示。只覆盖原生实心体素，墙边会呈格子状。

提示围绕视口中心在平面上的落点生成；中心无交集时采用有效角点中距相机最近的落点，无可见交集则不画。视口裁剪后最多 **128×128 格**；相机实际投影、平面、区域与快照不变时复用遮挡结果。

### 滚轮绑定

设置提供四套独立下拉框，每套均可选“无／调整参考平面高度／切换画笔颜色”：

| 输入 | 默认动作 |
| --- | --- |
| 滚轮 | 调整参考平面高度 |
| Ctrl+滚轮 | 无 |
| Shift+滚轮 | 切换画笔颜色 |
| Alt+滚轮 | 无 |

仅绘图打开且指针位于可用画布时生效；同时按多个修饰键不触发。文本、活动控件、面板、弹出菜单、ReShade 菜单及失焦时不触发。高精度滚轮满一格执行一次，切换组合、修改绑定或失去画布资格时清空余量。关闭绘图后仍用于自由相机缩放。

调整高度每格将 Z 加 / 减 **1**，保留小数部分，取消旧标定请求并结束当前路径，不读取角色或地形，按显示策略触发提示。切色向上选择下一色、向下选择上一色，在现有五色中循环，不依赖相机或平面；正在绘制的笔迹和路径保留创建时颜色，切色不触发平面提示。

### 各工具

| 工具 | 网格绘画行为 |
| --- | --- |
| 画笔 | 连续投影到平面，自由绘制，不吸附 |
| 直线、箭头、矩形 | 起终点在格心和交点中就近吸附，等距优先格心 |
| 椭圆 | 使用连续平面坐标生成轮廓，投影后随透视变化 |
| 文字 | 锚点固定在创建平面，文字保持正向及固定屏幕字号 |
| 橡皮 | 按当前投影擦除整条笔迹，包括箭头头部 |
| 路径 | 仍吸附整数格心、沿横纵方向连接 |

Shift 的 45 度、正方形和圆形约束在平面坐标计算；需要吸附的工具在合法格心或交点中选择。笔宽、箭头头部和文字字号保持屏幕像素大小。世界笔迹沿用路径覆盖层，不增加深度采集或装饰模型遮挡。

### 拖绘与两次点击

直线、箭头、矩形、椭圆在普通和网格绘画中均支持两种手势：按下后移动超过 ImGui 拖动阈值，松开完成；或点击一次起点，移动查看实时预览，再点击终点并松开完成。右键取消未完成图形，该次按压不撤销旧笔迹，也不旋转相机。

待定图形只在完成后进入撤销历史，不保存、不导出。换页、换工具、切换绘画模式、改变平面、退出绘图或失焦会取消待定图形；已经实际拖绘的笔迹沿用原来的完成保存策略。保存或导出可保留待定预览，仅处理已完成笔迹。Esc 也可取消未完成图形。

### 草稿兼容与限制

草稿写入 **格式 v3**，为所有笔迹保存网格基准。继续读取 v1/v2：普通旧笔迹保持屏幕坐标，旧路径保留原网格。每页 2000 条、合计 120000 点、60 步撤销等原限制保留。

第一次覆盖旧格式草稿前，先保存独立的 `drafts.pre-v3.txt`；已有同名备份不覆盖。备份失败则停止保存，保留原草稿。常规 `.bak` 仍用于上一次保存和恢复。新版草稿交给旧插件前，必须恢复升级前备份。SVG 使用显示和橡皮同一投影，导出时无相机的世界笔迹会省略。

本次只做编译期断言、编译检查、静态分析和编译链接，未运行测试程序或游戏，也不进行 SHA 校验。新交互、实际透视贴合、设置持久化和文件升级恢复仍需实机确认。

## English

Press **E**, then toggle **Grid drawing** to anchor new marks to the target plane. Ordinary drawing remains the default. Every mark captures its own plane, tool, ink and width; later mode/plane changes leave it in place. Missing camera/plane data rejects new grid marks.

**Grid preview** combines centers, intersections, borders and checker fill over 11×11 cells around the pointer. Opacity defaults to 45%; centers alone start enabled. Ordinary drawing previews paths; grid drawing previews all tools. It never cuts walls. Both previews use gray RGB(190,190,190); intersections are perspective-aligned crosses with 0.12-cell arms and 1.5-pixel width, against 1-pixel borders.

**Settings** opens a movable, resizable window. Close it with its X or the Settings button; E hides/restores it with drawing while retaining the session's open state. Changes apply immediately and save automatically under `[Drawing]` in `settings.ini`, preserving other preferences.

Plane-change hints default to borders plus checker, centers/intersections off, 25% opacity, **1 second**. Choose On, Off or Height adjustment only; the last option responds only to wheel height actions. Automatic level initialization stays silent. Duration is 0–5 seconds, with Ctrl+click numeric entry to 0.01 seconds; zero disables display. Existing hints update style immediately and duration relative to their original start. Disabled or expired hints never revive. Same-height marks restart the timer; new intents/pages/levels clear old hints and stale replies cannot restore them.

The hint uses the current effective camera, including free camera, to cast through each cell center. A native solid wall strictly before the center hides the whole cell; contact with the target's own voxel top does not. All layers share the mask, including visible-cell borders/cross arms. It does not require an actual native floor at the target height. Missing terrain shows an uncut hint. The viewport-clipped region remains capped at 128×128 cells; unchanged view/plane/region/terrain reuses the mask.

Four wheel bindings offer None, Adjust reference plane height or Cycle ink color. Defaults are plain wheel for height, Shift+wheel for color, Ctrl/Alt+wheel for None. Multiple modifiers do nothing. Bindings apply only on the available drawing canvas; text, widgets, panels, popups, menus and lost focus suppress them. Fractional steps accumulate only within the same binding/action and reset when ownership or binding changes. Close drawing for free-camera wheel zoom.

Height actions change Z by whole units, preserve its fraction, cancel pending marks and end the active path without reading character/terrain data. Color actions cycle forward/up and backward/down through the five inks, work without camera/plane data, preserve existing stroke/path colors and never trigger hints.

Pen/ellipse use continuous plane coordinates. Line/arrow/rectangle endpoints snap to the nearest center or intersection, preferring centers on ties; Shift constraints are calculated on the plane. Text follows its world anchor while remaining upright with fixed screen size. Stroke widths and arrowheads also remain pixel-sized. Erasing checks current projected outlines, including arrowheads; paths keep their existing integer-center/row/column behavior.

Line, arrow, rectangle and ellipse support drag-release or click-start/click-end in both drawing modes, with continuous previews. Right-click cancels before undo or camera rotation. Pending click shapes never enter history/save/export; page/tool/mode/plane changes, leaving drawing and lost focus cancel them. Actual drag strokes retain the existing completion policy.

Draft **v3** stores every mark’s grid and reads v1/v2. Before the first legacy overwrite, `drafts.pre-v3.txt` is preserved without replacing an existing copy; backup failure blocks saving. Restore it before using an older add-on. Regular `.bak` recovery and existing count/input limits remain. SVG uses the same projection as display/erasing and omits world marks without a camera.

Validation is static compilation/assertions, analysis and add-on linking only. No test programs, game sessions or SHA checks are run. Actual alignment, interaction, persistence and upgrade/recovery behavior still need hardware confirmation.
