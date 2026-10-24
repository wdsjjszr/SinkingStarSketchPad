# 网格绘画、预览与草稿升级 / Grid drawing

## 中文

按 **E** 打开绘图菜单。默认仍是普通屏幕绘画，点击 **网格绘画** 后，新笔迹绘制在当前标定的平面上，随游戏或自由相机重新投影。切换模式和重新标定都不会移动已有笔迹；每条笔迹保存创建时的平面、工具、颜色和笔宽。缺少有效相机或平面时，拒绝新建网格笔迹并提示原因。

### 预览与改高

**网格预览** 的中心点、网格交点、格子边框、棋盘填充可以独立叠加，统一透明度为 0–100%，默认 45%，初始只显示中心点。范围为指针附近 **11×11 格**；交点相对格心偏移半格。普通绘画时仅路径工具显示，网格绘画时所有工具都可显示。常驻预览不检查墙体，不挖空。

角色初始化、角色按钮、手动标记、中键标记或滚轮改高成功后，另行显示 **2 秒**、约 25% 透明度的棋盘平面提示；即使关闭网格预览也会显示。同高度重新标记会重新计时，新的标定意图立即清除旧提示，换页和换关卡也会清除。异步结果仍检查请求、页面和关卡，旧请求不会恢复旧提示。

提示围绕视口中心在平面上的落点生成；中心无交集时，选有效视口角点中距相机最近的落点，无可见交集则不画。使用视口裁剪，极远景最多 **128×128 格**。只挖去平面穿入原生实心体素内部的格子，体素顶面保留，不按视线挖掉墙后格子；无有效体素快照时完整显示。

菜单打开时在画布滚轮向上 / 向下，当前 Z 每格加 / 减 **1**，保留小数部分。高精度滚轮累计满一格才移动。改高会取消旧标定请求、结束当前路径并触发两秒提示，不读取角色或地形。文字输入、活动控件、面板、ReShade 菜单及失焦期间不触发；关闭绘图后，滚轮继续缩放自由相机。

网格绘画开关、预览总开关、四个图层与透明度保存到 `SinkingStarSketch-data/settings.ini` 的 `[Drawing]`，保留语言和相机设置。

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

**Grid preview** combines centers, half-cell intersections, borders and checker fill over 11×11 cells around the pointer. Opacity defaults to 45%; centers alone start enabled. Ordinary drawing previews paths; grid drawing previews all tools. It never cuts walls. Preferences persist under `[Drawing]` in `settings.ini`, preserving language and camera settings.

Every successful plane mark, including same-height selection and wheel changes, displays a separate two-second checker hint at roughly 25% opacity. Only solid voxel interiors are cut; voxel tops and cells behind walls remain. Missing terrain shows an uncut hint. The viewport-clipped region is capped at 128×128 cells, using the center-plane hit or nearest valid corner hit. New intents, pages and levels clear the old hint; stale asynchronous replies cannot restore it.

With drawing open, the canvas wheel changes target Z by whole units while retaining its fraction. Precision-wheel fractions accumulate. This cancels pending marks, ends the active path and shows the hint without reading character/terrain data. Text, active widgets, panels, menus and lost focus suppress it. Close drawing to use free-camera wheel zoom.

Pen/ellipse use continuous plane coordinates. Line/arrow/rectangle endpoints snap to the nearest center or intersection, preferring centers on ties; Shift constraints are calculated on the plane. Text follows its world anchor while remaining upright with fixed screen size. Stroke widths and arrowheads also remain pixel-sized. Erasing checks current projected outlines, including arrowheads; paths keep their existing integer-center/row/column behavior.

Line, arrow, rectangle and ellipse support drag-release or click-start/click-end in both drawing modes, with continuous previews. Right-click cancels before undo or camera rotation. Pending click shapes never enter history/save/export; page/tool/mode/plane changes, leaving drawing and lost focus cancel them. Actual drag strokes retain the existing completion policy.

Draft **v3** stores every mark’s grid and reads v1/v2. Before the first legacy overwrite, `drafts.pre-v3.txt` is preserved without replacing an existing copy; backup failure blocks saving. Restore it before using an older add-on. Regular `.bak` recovery and existing count/input limits remain. SVG uses the same projection as display/erasing and omits world marks without a camera.

Validation is static compilation/assertions, analysis and add-on linking only. No test programs, game sessions or SHA checks are run. Actual alignment, interaction, persistence and upgrade/recovery behavior still need hardware confirmation.
