#pragma once
#include <array>
#include <string_view>

namespace sketch::i18n {
enum class Language { System, English, Chinese };
inline constexpr const char *language_code(Language l){return l==Language::English?"en":l==Language::Chinese?"zh-CN":"auto";}
inline Language parse_language(std::string_view s){return s=="en"?Language::English:s=="zh-CN"?Language::Chinese:Language::System;}
inline bool use_chinese(Language l,bool system_chinese){return l==Language::Chinese||(l==Language::System&&system_chinese);}

#define SKETCH_TEXTS(X) \
 X(Title,"SinkingStarSketchPad###SinkingStarSketch","SinkingStarSketchPad###SinkingStarSketch") \
 X(Pen,"Pen","画笔") X(Line,"Line","直线") X(Arrow,"Arrow","箭头") \
 X(Rectangle,"Rectangle","矩形") X(Ellipse,"Ellipse","椭圆") X(Text,"Text","文字") \
 X(Eraser,"Eraser","橡皮") X(Path,"Path","路径") X(Color,"Color","颜色") X(Width,"Width","笔宽") \
 X(Undo,"Undo","撤销") X(Redo,"Redo","重做") X(Clear,"Clear page","清空本页") X(Page,"Page ","草稿 ") \
 X(Save,"Save","保存") X(Saved,"Saved","已保存") X(Saving,"Saving...","正在保存") X(Export,"Export SVG","导出 SVG") \
 X(Return,"Return to game","返回游戏") \
 X(Shortcuts,"Shortcut help","快捷键说明") \
 X(ShortcutsTitle,"Shortcut help###SketchShortcuts","快捷键说明###SketchShortcuts") \
 X(ShortcutGeneralTitle,"General","常用操作") \
 X(ShortcutToolsTitle,"Drawing tools","绘图工具") \
 X(ShortcutDrawingTitle,"While drawing","绘图时") \
 X(KeyboardHelp,"You can move your character while drawing.","绘图时，可以继续操作角色移动") \
 X(ShortcutCameraTitle,"Free camera","自由相机") \
 X(Close,"Close","关闭") \
 X(WindowMoveHelp,"Drag the header to move. E hides / restores this window with drawing.","拖动标题栏移动；E 随绘图隐藏 / 恢复本窗口。") \
 X(DrawingGestures,"Shapes: drag or click start, then click end. Pen: drag; path / text: click. Esc cancels text.","图形：拖绘或点击起点再点击终点。画笔：拖绘；路径 / 文字：点击。Esc 取消文字。") \
 X(CameraToggle,"Free camera","自由相机") \
 X(CameraTitle,"Free camera (experimental)###SketchFreeCamera","自由相机（实验功能）###SketchFreeCamera") \
 X(CameraUnavailable,"Game camera constants are not ready. Enter a level and wait.","尚未连接可控制的游戏相机，请进入关卡后稍候。") \
 X(CameraLimits,"Experimental render camera: original game culling, shadows, reflections and mouse picking may not follow large view changes.","实验性渲染相机：大幅变换时，游戏原有裁剪、阴影、反射及鼠标选取可能不随视角更新。") \
 X(CameraSpeed,"Movement speed","移动速度") X(CameraYaw,"Yaw","旋转") X(CameraPitch,"Tilt","俯仰") X(CameraZoom,"Zoom","缩放") \
 X(CameraSensitivity,"Mouse sensitivity","鼠标灵敏度") X(CameraInvertX,"Invert mouse X","反转鼠标 X 轴") X(CameraInvertY,"Invert mouse Y","反转鼠标 Y 轴") \
 X(CameraHold,"Hold camera","固定相机") \
 X(CameraHeld,"Camera held. T resumes controls; Esc or Restore game view restores the native camera.","相机已固定。T 继续调整；Esc 或恢复游戏视角可归位。") \
 X(CameraPrefsHelp,"Camera preferences are saved. T / Esc restores the game view; Hold camera alone keeps the pose.","相机偏好自动保存；T / Esc 恢复游戏视角，只有固定相机按钮保留机位。") \
 X(CameraSettingsFailed,"Could not save camera settings.","无法保存相机设置。") \
 X(CameraX,"Offset X","偏移 X") X(CameraY,"Offset Y","偏移 Y") X(CameraZ,"Height offset","高度偏移") \
 X(CameraReset,"Reset to current game camera","重置到当前游戏相机") X(CameraOff,"Restore game view / disable","恢复游戏视角 / 关闭") \
 X(CameraLive,"Camera override connected","相机覆盖已连接") X(CameraWaiting,"Waiting for a fresh game camera frame","等待新的游戏相机帧") \
 X(Stats,"Page %u/3 | %zu marks | %s","草稿 %u/3 · %zu 条笔迹 · %s") \
 X(PageStats,"Page %u/3 | %zu marks","草稿 %u/3 · %zu 条笔迹") \
 X(GridPlane,"Grid / plane","网格 · 平面") X(PlaneHeight,"Height %.3g","高度 %.3g") X(PlaneUnset,"Not set","未确定") \
 X(CharacterPlaneButton,"Character\nplane","设为角色\n所在平面") X(MarkPlaneButton,"Mark ground\nplane","手动标记\n网格平面") \
 X(Language,"Language","语言 / Language") X(LanguageSystem,"System","跟随系统") X(LanguageChinese,"Chinese","中文") \
 X(UIScale,"UI scale","界面缩放") X(ApplyUIScale,"Apply scale","应用缩放") X(HintStyle,"Preview grid style","预览网格样式") X(DebugTitle,"Debug","Debug") X(DeveloperOnly,"Developer tools","开发用") \
 X(UISettingsFailed,"Could not save UI settings.","无法保存界面设置。") \
 X(NewPath,"Finish / New path","结束路径 / 新路径") X(GridPreview,"Grid preview","网格预览") \
 X(GridCenters,"Cell centers","中心点") X(GridIntersections,"Grid intersections","网格交点") \
 X(GridBorders,"Cell borders","网格边框") X(GridChecker,"Checkerboard","棋盘填充") X(GridOpacity,"Preview opacity","预览透明度") \
 X(DrawingSettingsFailed,"Could not save drawing settings.","无法保存绘图设置。") \
 X(Settings,"Settings","设置") X(SettingsTitle,"Settings###SketchSettings","设置###SketchSettings") \
 X(HintSettings,"Plane change preview","平面变动预览") X(HintMode,"Show on plane changes","在平面变动时显示预览") \
 X(HintOn,"On","开") X(HintOff,"Off","关") X(HintWheelOnly,"Height only","仅微调") \
 X(HintDuration,"Hold duration (seconds)","保持时长（秒）") \
 X(WheelSettings,"Wheel behavior while drawing","绘图时滚轮行为") X(Wheel,"Wheel","滚轮") X(CtrlWheel,"Ctrl + wheel","Ctrl + 滚轮") \
 X(ShiftWheel,"Shift + wheel","Shift + 滚轮") X(AltWheel,"Alt + wheel","Alt + 滚轮") \
 X(WheelNone,"None","无") X(WheelPlane,"Adjust reference plane height","调整参考平面高度") X(WheelColor,"Cycle ink color","切换画笔颜色") \
 X(WheelTool,"Cycle drawing tool","切换绘画工具") \
 X(WheelHelp,"Only on the drawing canvas. Multiple modifiers do nothing. Close drawing for camera zoom.","仅在绘图画布生效；多个修饰键同时按下不触发。关闭绘图后滚轮用于相机缩放。") \
 X(PathFinished,"Path finished.","本条路径已结束。") \
 X(VerticalSnapHelp,"Shift: lock start X/Y across heights for a vertical line. Same-plane Shift keeps angle snapping.","Shift：跨层时锁定起点 X/Y，快速绘制竖直线；同层保持角度约束。") \
 X(GridDrawing,"Grid drawing","网格绘画") \
 X(PathHelp,"Click centers along X, Y or Z; click the endpoint again or right-click to finish.","沿 X、Y、Z 点击格心，转弯处加点；重复点击终点或右键结束。") \
 X(GridWait,"Waiting for the initial character plane. Set the character plane or mark ground manually.","等待角色初始平面，也可设为角色所在平面或手动标记。") \
 X(GroundMark,"Manual marking","手动标记") X(GroundFinish,"Cancel marking","取消标记") \
 X(GroundHelp,"Left-click a visible ground cell to set the plane and finish. A wall or empty space keeps marking active. Esc cancels.","左键点击可见地面格，成功后自动结束标记；命中墙面或空处会保持标记。Esc 取消。") \
 X(GroundCharacter,"Set plane to character","设为角色所在平面") X(GroundCharacterSet,"Plane set to the character logical height.","已设为角色所在平面。") \
 X(GroundCharacterWait,"Could not read the controlled character. The target plane was kept.","无法读取当前控制角色，已保留原目标平面。") \
 X(GroundLoading,"Native ground / character data is loading or unavailable.","原生地面或角色数据正在读取，或暂不可用。") \
 X(GroundWall,"A wall blocks this position. Point at a visible ground cell.","此处被墙面遮挡，请指向可见的地面格。") \
 X(GroundShortcut,"Canvas: middle click marks ground; wheel bindings are configurable in Settings.","画布中键快速标记地面；滚轮及组合键行为可在设置中调整。") \
 X(GroundLimits,"Occlusion checks native solid walls. Decorative models, platforms and lilies are not included.","遮挡仅检查原生实心墙体；暂不包含装饰模型、升降台和莲叶。") \
 X(GroundHover,"Ground height %.3g","地面高度 %.3g") \
 X(GroundLocked,"Target plane: height %.3g","目标平面：高度 %.3g") \
 X(GroundNoHit,"No native ground cell under the pointer.","指针下没有原生地面格。") \
 X(GroundAnchored,"Target plane set. Marking finished.","目标平面已设置，标记已结束。") \
 X(NativeDebug,"Preview native ground (through walls)","预览原生地面格（透视）") \
 X(NativeLoading,"Reading the current scene's native ground...","正在读取当前场景的原生地面格……") \
 X(NativeError,"Native ground unavailable: %s","原生地面格暂不可用：%s") \
 X(NativeStats,"Scene: %s | cells: %zu | visible: %u | heights: %zu","关卡：%s · 格子：%zu · 屏幕内：%u · 高度：%zu") \
 X(NativeSources,"Wall sets: %zu | solid voxels: %zu | floors: %zu | water maps: %zu","墙体组：%zu · 实心体素：%zu · 地板：%zu · 水域：%zu") \
 X(NativeHeight,"Preview height","预览高度") \
 X(NativeAll,"All heights","全部高度") \
 X(NativeVoxel,"Voxel top","体素顶面") X(NativeBase,"Layer base","基准底层") X(NativeFence,"Floor fence","地板格") \
 X(NativeHover,"%s | entity %u\nX %.4f  Y %.4f  Z %.4f\nCell spacing: 1 world unit","%s · 实体 %u\nX %.4f  Y %.4f  Z %.4f\n格距：1 世界单位") \
 X(NativeLegend,"Colors show height. Filled dots: voxel tops; rings: layer bases; crosses: floors. Hover a center for XYZ and source.","颜色表示高度；实心点为体素顶面，空心圈为基准底层，十字为地板格。悬停格心查看 XYZ 和来源。") \
 X(NativeLimits,"Debug draws through walls. Layer bases are logical reference planes; wall tops are candidates, not a walkability result. Platforms and lilies are not included yet.","Debug 透视显示；基准底层是逻辑参考平面，墙顶也属于候选，不代表可通行。暂未包含升降台和莲叶。") \
 X(CameraReady,"Game camera connected","游戏主相机已连接") \
 X(CameraWait,"Waiting for the game camera. World marks are hidden until it is ready.","等待游戏主相机；连接后才显示世界笔迹。") \
 X(Anchoring,"Grid drawing anchors new marks to the plane; turn it off for screen drawing. Paths always use world cells.","网格绘画将新笔迹固定在当前平面；关闭后使用屏幕绘画。路径始终固定在世界格子上。") \
 X(TextTitle,"Text annotation###SketchText","文字标注###SketchText") X(Done,"Done","完成") X(Cancel,"Cancel","取消") \
 X(Hud,"SinkingStarSketchPad  E Draw | G Cursor | B Marks | T Camera","SinkingStarSketchPad  E绘图 G指针 B草稿 T相机") \
 X(SaveFailed,"Save failed: ","保存失败：") X(SettingsFailed,"Could not save the language setting.","无法保存语言设置。") \
 X(Recovered,"Draft recovered from backup.","已从备份恢复草稿。") X(Unreadable,"Could not read the old draft; the original was preserved.","旧草稿无法读取；原文件已保留。") \
 X(Full,"Page or point limit reached. Switch pages or remove marks.","本页或点数已满，请换页或清理笔迹。") \
 X(Cleared,"Page cleared. Undo restores it.","本页已清空，可撤销恢复。") \
 X(NoCamera,"Game camera unavailable. Enter a level and try again.","尚未读取到游戏主相机，请回到关卡后重试。") \
 X(Detecting,"No target plane. Set the character plane or mark ground manually.","尚未确定目标平面，请设为角色所在平面或手动标记。") \
 X(NoPlane,"Could not project this position onto the cell plane.","此处无法投影到格子平面。") \
 X(NeedCorner,"Add a point at the corner first: paths change only one of X, Y or Z per segment.","请先点击转弯格：路径每段只沿 X、Y 或 Z 一个方向连接。") \
 X(TooFar,"Points are too far apart. Add intermediate points.","两点距离过远，请分段添加。") \
 X(PathFull,"Path point limit reached.","路径点数已满。") \
 X(Exported,"Exported to SinkingStarSketch-data/exports.","已导出到 SinkingStarSketch-data/exports。") X(ExportFailed,"Export failed: ","导出失败：")

enum class Text {
#define ITEM(id,en,zh) id,
SKETCH_TEXTS(ITEM)
#undef ITEM
Count
};
struct Entry {const char *en,*zh;};
inline constexpr std::array<Entry,static_cast<size_t>(Text::Count)> catalog={{
#define ITEM(id,en,zh) {en,zh},
SKETCH_TEXTS(ITEM)
#undef ITEM
}};
inline const char *get(Text text,bool chinese){auto &entry=catalog[static_cast<size_t>(text)];return chinese?entry.zh:entry.en;}
#undef SKETCH_TEXTS

struct ShortcutEntry {const char *key,*en,*zh;const char *zh_key=nullptr;};
inline const char *get(const ShortcutEntry &entry,bool chinese){return chinese?entry.zh:entry.en;}
inline constexpr const char *shortcut_key(const ShortcutEntry &entry,bool chinese){return chinese&&entry.zh_key?entry.zh_key:entry.key;}
inline constexpr ShortcutEntry general_shortcuts[]={
    {"E","Enter / leave drawing","进入 / 退出绘图"},
    {"G","Show / hide cursor","指针显示 / 隐藏"},
    {"B","Show / hide marks and leave drawing","草稿显示 / 隐藏，并退出绘图"},
    {"T","Toggle free camera","开关自由相机"},
    {"Esc","Cancel marking, pending shape or text; otherwise restore game view / leave drawing","取消标记、未完成图形或文字；否则恢复游戏视角 / 退出绘图"},
    {"Alt+Tab","Keep drawing mode and text; finish drag strokes, cancel pending click shapes and save","保留绘图模式和文字；完成拖绘，取消待定图形并保存"}
};
inline constexpr ShortcutEntry tool_shortcuts[]={
    {"Alt+P","Pen","画笔"},{"Alt+L","Line","直线"},{"Alt+A","Arrow","箭头"},{"Alt+R","Rectangle","矩形"},
    {"Alt+O","Ellipse","椭圆"},{"Alt+T","Text","文字"},{"Alt+K","Eraser","橡皮"},{"Alt+H","Cell path","格子路径"},{"Alt+1-5","Ink color","笔色"}
};
inline constexpr ShortcutEntry drawing_shortcuts[]={
    {"Middle click","Mark ground under the pointer once","快速标记指针下地面所在平面","鼠标中键"},
    {"Wheel","Configured action; plane height by default","按设置执行，默认调整平面高度","滚轮"},
    {"Shift+wheel","Cycle color by default; change bindings in Settings","默认切色；可在设置中修改绑定","Shift + 滚轮"},
    {"Shift","Vertical across heights; same-plane 45-degree lines/arrows; square/circle constraint","跨层竖直线；同层 45 度直线 / 箭头；正方形 / 正圆约束"},
    {"Right click","Cancel shape, finish path, otherwise hold to erase","取消图形、结束路径，否则按住擦除","鼠标右键"},
    {"Alt+wheel","Cycle tool by default; existing bindings are kept","默认切换工具；保留已有绑定","Alt + 滚轮"},
    {"Ctrl+Z","Undo","撤销"},{"Ctrl+Y / Ctrl+Shift+Z","Redo","重做"},
    {"Ctrl+1 / Ctrl+2 / Ctrl+3","Switch scratch page","切换草稿"},{"Ctrl+S","Save completed marks","保存已完成笔迹"},
    {"Ctrl+Shift+Delete","Clear page (undoable)","清空本页（可撤销）"},{"Ctrl+Enter","Finish text","完成文字"}
};
inline constexpr ShortcutEntry camera_shortcuts[]={
    {"W/A/S/D","Pan","平移"},{"Q / Z","Up / down; Shift speeds up","升高 / 降低，Shift 加速"},
    {"Right drag","Rotate / tilt with drawing closed","旋转 / 俯仰（绘图关闭时）","右键拖动"},
    {"Wheel","Configured action while drawing; zoom while closed","绘图打开时按设置执行，关闭时缩放","滚轮"},
    {"Esc / T","Restore game view","恢复游戏视角"}
};
}
