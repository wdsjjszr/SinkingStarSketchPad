# 网格绘画、预览与草稿升级 / Grid drawing

## 中文

按 **E** 打开绘图菜单。默认仍是普通屏幕绘画，点击 **网格绘画** 后，新笔迹绘制在当前标定的平面上，随游戏或自由相机重新投影。切换模式和重新标定都不会移动已有笔迹；每条笔迹保存创建时的平面、工具、颜色和笔宽。缺少有效相机或平面时，拒绝新建网格笔迹并提示原因。

### 预览、渐隐与设置

**网格预览** 默认开启，初始仅显示中心点，透明度默认 45%。中心点、透视交点十字、边框和黑白棋盘可以叠加。范围为鼠标附近 **17×17 格**，普通绘画时路径显示，网格绘画时所有工具可显示，始终不检查墙体。沿平面空间距离使用 `1-smoothstep(0,8.5格,距离)` 径向渐隐；颜色和透明度按顶点插值。

两类预览的白色点半径为 2.5 像素、边框宽 2 像素、十字宽 3 像素，每条十字臂长仍为 0.12 格。点、线框和交点十字使用白色主体与黑色外描边：点外圈半径 5.5 像素，线条两侧各加 1 像素描边，线框与十字主体尺寸不变。描边位于主体外侧，不用黑底盘压暗点中心。棋盘继续黑白交替。所有预览位于笔迹下方，吸附圈和鼠标指针在笔迹上方。网格直线、箭头、矩形和路径显示清晰的当前吸附点提示圈，关闭网格预览仍显示；有起点时采用实际 Shift 约束结果。

**设置** 窗口可移动、调整大小，修改即时生效并自动保存到 `settings.ini`。E 随绘图菜单隐藏 / 恢复它，保留本次会话打开状态。以下默认值只用于尚未保存对应偏好的情况，升级不会重置已有设置。

平面变动提示默认边框和棋盘开启、点和十字关闭，基础透明度 **15%**、保持 **0.75 秒**，之后另有保持时长 **30%** 的 Ease-out Fade。渐隐透明度为 `(1-u)³`，默认额外 0.225 秒、总计 0.975 秒。保持时长范围 0–5 秒，Ctrl+点击可直接输入；0 秒不显示。显示策略仍为“开／关／仅微调”，仅微调只响应滚轮改高；关卡首次初始化始终静默。

同高度重新标定重新计时，新意图、换页、换关卡清除旧提示；过期异步结果不能恢复旧提示。即时调整时长仍按原起点计算，关闭或到期的提示不复活。

平面提示使用当前有效相机，包括自由相机，将原生体素面投影到屏幕并裁剪提示图元，同一格只隐藏被挡住的部分。四图层均应用此裁剪。与平面接触的面按 `1e-4` 世界单位容差排除自身遮挡，不改变或取整目标 Z，避免整格忽隐忽现；棋盘奇偶固定在世界格索引。无有效同关卡快照时完整显示。不使用游戏深度或装饰模型，实际轮廓仍受原生体素精度限制。

提示保持视口中心落点／最近有效角点定位、视口裁剪与 **128×128 格** 上限。体素面合并后按快照缓存，屏幕裁剪网格按相机、平面、视口、范围和图层缓存；渐隐只改变透明度。

### 滚轮、跨高度和右键

四套滚轮绑定均可选“无／调整参考平面高度／切换画笔颜色”。默认：滚轮改高、Shift+滚轮切色（向上上一色、向下下一色，循环选择），Ctrl／Alt+滚轮无；多个修饰键同时按下不触发。仅绘图打开的可用画布生效，文本、控件、面板、菜单及失焦时屏蔽。精细滚轮满一格执行一次，切换绑定或输入资格清空余量。关闭绘图后滚轮仍缩放自由相机。

滚轮改高每格 Z 加 / 减 1、保留小数，不读取角色或地形。**等待第二点的网格直线／箭头保留起点**，终点在新高度吸附；跨层时按住 Shift 锁定起点的世界 X/Y，仅改变 Z，快速绘制竖直线，不要求指针射线先命中新平面。释放 Shift 恢复普通吸附，同高度仍为 45 度约束。相机／平面不可用或竖直终点不在相机前方时，提示并继续等待有效终点。活动路径也保留：下一点每段只能改变 X、Y、Z 中一个轴，竖向段按一格补点；滚轮本身不加点，需要在同 XY 新高度点击确认。同时横移和升降会提示先加转折点。

画笔、矩形、椭圆、实际拖绘中的图形保持原来的切平面处理。角色按钮、手动／中键标定、切页、切换工具或模式、离开绘图和失焦仍结束或取消操作。颜色切换不依赖相机／平面，只影响新笔迹，已开始笔迹保留其颜色。

右键按优先级**取消未完成图形 → 结束活动路径 → 按住临时擦除**。取消或结束的整次按压不擦除；擦除松开后恢复原工具，一次拖动为一步撤销。Ctrl+Z 负责撤销。绘图打开时右键不旋转相机，关闭后右拖仍旋转。路径再次点击同一 XYZ 终点也会结束，不增加重复点或历史记录；双击终点即可快速结束。

### 各工具

| 工具 | 网格绘画行为 |
| --- | --- |
| 画笔 | 连续投影到平面，自由绘制，不吸附 |
| 直线、箭头、矩形 | 起终点在格心和交点中就近吸附，等距优先格心 |
| 椭圆 | 使用连续平面坐标生成轮廓，投影后随透视变化 |
| 文字 | 锚点固定在创建平面，文字保持正向及固定屏幕字号 |
| 橡皮 | 按当前投影擦除整条笔迹，包括箭头头部 |
| 路径 | 吸附整数格心，沿 X、Y、Z 单轴连接 |

Shift 在跨层直线／箭头中锁定世界竖直方向；同层 45 度、正方形和圆形约束仍在平面坐标计算；需要吸附的工具在合法格心或交点中选择。笔宽、箭头头部和文字字号保持屏幕像素大小。世界笔迹沿用路径覆盖层，不增加深度采集或装饰模型遮挡。

### 拖绘与两次点击

直线、箭头、矩形、椭圆在普通和网格绘画中均支持两种手势：按下后移动超过 ImGui 拖动阈值，松开完成；或点击一次起点，移动查看实时预览，再点击终点并松开完成。右键取消未完成图形，该次按压不撤销旧笔迹，也不旋转相机。

待定图形只在完成后进入撤销历史，不保存、不导出。换页、换工具、切换绘画模式、非滚轮标定、退出绘图或失焦会取消待定图形；滚轮改高仅保留待定网格直线／箭头；已经实际拖绘的笔迹沿用原来的完成保存策略。保存或导出可保留待定预览，仅处理已完成笔迹。Esc 也可取消未完成图形。

### 草稿兼容与限制

草稿写入 **格式 v4**，保存网格及每点 XYZ，Z 为相对该笔迹基准的偏移。继续读取 v1/v2/v3 并补零 Z：普通旧笔迹保持屏幕坐标，旧路径保留原网格。每页 2000 条、合计 120000 点、60 步撤销等原限制保留。

第一次覆盖旧格式草稿前，先保存独立的 `drafts.pre-v4.txt`；已有同名备份不覆盖，原有 pre-v3 备份也不删除。备份失败则停止保存，保留原草稿。常规 `.bak` 仍用于上一次保存和恢复。新版草稿交给旧插件前，必须恢复升级前备份。SVG 使用显示和橡皮同一投影，导出时无相机的世界笔迹会省略。

本次只做编译期断言、编译检查、静态分析和编译链接，未运行测试程序或游戏，也不进行 SHA 校验。新交互、实际透视贴合、设置持久化和文件升级恢复仍需实机确认。

## English

Press **E**, then toggle **Grid drawing** to anchor new marks to the target plane. Ordinary drawing remains the default. Every mark captures its own plane, tool, ink and width; later mode/plane changes leave it in place. Missing camera/plane data rejects new grid marks.

**Grid preview** defaults on with centers only at 45% opacity over 17×17 cells. A plane-space radial smoothstep fades from the pointer to an 8.5-cell radius, without wall cutting. Both previews use white points/lines/crosses with black outer outlines, plus equal-opacity black/white checker fill. The dot outer radius is 5.5 pixels; line outlines add 1 pixel on either side without changing core sizes. Previews are below saved/pending ink; snap markers and the cursor remain above it. White point radius is 2.5 pixels, borders 2 pixels, crosses 3 pixels with 0.12-cell arms. Line/arrow/rectangle/path snapping has a clear marker independent of the preview switch.

**Settings** applies and saves immediately. New/missing hint preferences default to borders/checker, 15% opacity and a **0.75-second hold**, followed by an additional **30% cubic ease-out fade** (0.975 seconds total by default). Existing saved preferences remain. On/Off/Wheel-only and silent initialization remain; zero duration disables display. New intents replace old hints, and expired hints never revive.

Hint occlusion now projects native voxel faces and subtracts their screen polygons from all hint layers, retaining visible portions within a cell. A 1e-4 world-unit contact tolerance excludes self-occlusion without shifting target Z. World checker parity stays fixed. Native geometry and clipped meshes are cached; fade changes opacity only. Missing terrain shows an uncut hint. No game depth/decorative occlusion is added; the 128×128 cap remains.

Wheel defaults remain height/plain, color/Shift (up: previous, down: next, wrapping), None/Ctrl/Alt, with multiple modifiers disabled. Height changes preserve a waiting grid line/arrow's original start and put its endpoint on the new plane; Shift locks the original world X/Y across heights, changing only Z; it does not require the pointer ray to intersect the new plane. Release Shift for normal snapping; same-height Shift retains angle snapping. Missing camera/plane or a vertical endpoint behind the camera rejects completion and keeps waiting. Paths may change one of X/Y/Z per segment and fill unit centers, including vertical runs. Wheel changes add no point automatically. Other tools and non-wheel transitions retain completion/cancellation rules.

Right press cancels a pending shape first, finishes an active path next, otherwise temporarily erases whole marks until release. Cancel/finish presses never erase. One erase drag is one undo step; Ctrl+Z remains undo. Camera right-drag rotation is available with drawing closed. Clicking the same XYZ path endpoint again finishes it without adding history.

Shapes support drag-release and click-start/click-end with live preview. Only completed shapes enter history/save/export. Each mark retains its own grid, color and width. Text stays upright at a world anchor; pen/ellipse use continuous plane coordinates. Zero-width/height rectangles render as one normal-width line, or one point if both dimensions vanish.

Draft **v4** stores XYZ points with relative Z and reads v1–v3 as zero Z offsets. Before overwriting legacy drafts, `drafts.pre-v4.txt` is preserved without replacing an existing backup; backup failure blocks saving. Earlier backups and regular `.bak` recovery remain. Restore the pre-v4 file before downgrading. Display, erasing and SVG share the same projection.

Validation is static compilation/assertions, analysis and add-on linking only. No test programs, game sessions or SHA checks are run. Actual alignment, interaction, persistence and upgrade/recovery behavior still need hardware confirmation.

Upstream 3.4.6 camera retention/defaults are integrated while keeping the full font-bearing package. Preview opacity defaults change only when no value has been saved; a previously stored 45% remains until edited in Settings. Wheel modifier bindings are unchanged.
