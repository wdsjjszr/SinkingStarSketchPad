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
 X(Pen,"Pen Alt+P","画笔 Alt+P") X(Line,"Line Alt+L","直线 Alt+L") X(Arrow,"Arrow Alt+A","箭头 Alt+A") \
 X(Rectangle,"Rectangle Alt+R","矩形 Alt+R") X(Ellipse,"Ellipse Alt+O","椭圆 Alt+O") X(Text,"Text Alt+T","文字 Alt+T") \
 X(Eraser,"Eraser Alt+K","橡皮 Alt+K") X(Path,"Path Alt+H","路径 Alt+H") X(Color,"Color","颜色") X(Width,"Width","笔宽") \
 X(Undo,"Undo","撤销") X(Redo,"Redo","重做") X(Clear,"Clear page","清空本页") X(Page,"Page ","草稿 ") \
 X(Save,"Save","保存") X(Saved,"Saved","已保存") X(Saving,"Saving...","正在保存") X(Export,"Export SVG","导出 SVG") \
 X(Return,"Return to game E","返回游戏 E") \
 X(Help,"E / Esc: return | Shift: snap | Right click / Ctrl+Z: undo | Ctrl+Y: redo","E / Esc 返回游戏 · Shift 吸附 · 右键 / Ctrl+Z 撤销 · Ctrl+Y 重做") \
 X(KeyboardHelp,"Keyboard: game | Alt+letter: tool | Alt+1-5: color | Alt+Tab: keep drawing mode","键盘操作游戏 · Alt+字母选工具 · Alt+1-5 选色 · 切出后保留绘图模式") \
 X(CameraToggle,"Free camera T","自由相机 T") \
 X(CameraTitle,"Free camera (experimental)###SketchFreeCamera","自由相机（实验功能）###SketchFreeCamera") \
 X(CameraUnavailable,"Game camera constants are not ready. Enter a level and wait.","尚未连接可控制的游戏相机，请进入关卡后稍候。") \
 X(CameraHelp,"WASD: pan | Q/Z: up/down | Shift: faster | Right drag / wheel on canvas: rotate / zoom. E: drawing. Ctrl+Z: undo. Esc: restore game view.","WASD 平移 · Q/Z 升降 · Shift 加速 · 在画面空白处右键拖动旋转、滚轮缩放。E 绘图；Ctrl+Z 撤销；Esc 恢复游戏相机。") \
 X(CameraLimits,"Experimental render camera: original game culling, shadows, reflections and mouse picking may not follow large view changes.","实验性渲染相机：大幅变换时，游戏原有裁剪、阴影、反射及鼠标选取可能不随视角更新。") \
 X(CameraSpeed,"Movement speed","移动速度") X(CameraYaw,"Yaw","旋转") X(CameraPitch,"Tilt","俯仰") X(CameraZoom,"Zoom","缩放") \
 X(CameraSensitivity,"Mouse sensitivity","鼠标灵敏度") X(CameraInvertX,"Invert mouse X","反转鼠标 X 轴") X(CameraInvertY,"Invert mouse Y","反转鼠标 Y 轴") \
 X(CameraPrefsHelp,"Speed and axis settings are saved. Tilt stops at the top and before the horizon.","速度和轴向设置会自动保存；俯仰在顶部及地平线前停止，避免翻转。") \
 X(CameraSettingsFailed,"Could not save camera settings.","无法保存相机设置。") \
 X(CameraX,"Offset X","偏移 X") X(CameraY,"Offset Y","偏移 Y") X(CameraZ,"Height offset","高度偏移") \
 X(CameraReset,"Reset to current game camera","重置到当前游戏相机") X(CameraOff,"Restore game view / disable","恢复游戏视角 / 关闭") \
 X(CameraLive,"Camera override connected","相机覆盖已连接") X(CameraWaiting,"Waiting for a fresh game camera frame","等待新的游戏相机帧") \
 X(Stats,"Page %u/3 | %zu marks | %s","草稿 %u/3 · %zu 条笔迹 · %s") \
 X(NewPath,"Finish / New path","结束路径 / 新路径") X(GridPreview,"Cell-center preview","格心预览") \
 X(GridStats,"Auto grid: %.3g cell size | %u detected centers","自动格子：间距 %.3g · %u 个格心") \
 X(PathHelp,"Click centers in a row or column. Add a point at each turn.","点击格心加点，转弯处加点。") \
 X(GridWait,"Detecting the grid from game instances. No calibration needed.","正在从游戏实例自动识别格子，无需手动校准。") \
 X(GroundMark,"Mark ground manually","手动标识地面") X(GroundFinish,"Finish ground marking","结束地面标识") \
 X(GroundHelp,"Hover a floor and left-click to anchor it. Use the wheel to switch overlapping layers. Esc finishes marking; then click to draw a path.","悬停地面预览，左键锚定；重叠处用滚轮切层。Esc 结束标识，随后点击可绘制路径。") \
 X(GroundLayers,"Candidate height layers: %u","候选高度层：%u") \
 X(GroundLimits,"Experimental: selection uses inferred grid cells; wall occlusion is not checked.","实验版：按候选格子选层，暂不判断墙体遮挡。") \
 X(GroundHover,"Height %.3g | candidate %u/%u","高度 %.3g · 候选 %u/%u") \
 X(GroundLocked,"Ground locked at height %.3g","地面已锁定：高度 %.3g") X(GroundAuto,"Use automatic ground","恢复自动地面") \
 X(GroundNoHit,"No candidate ground here. Try a detected cell center or wait for grid detection.","此处没有候选地面，请靠近已识别格心，或等待网格识别。") \
 X(GroundAnchored,"Ground anchored. Keep clicking to switch floors; finish marking to draw a path.","地面已锚定；可继续点击切换高度，结束标识后绘制路径。") \
 X(GroundDebug,"Debug: show detection input points","Debug：显示识别输入点") \
 X(GroundDebugStats,"Input points: %zu | on-screen: %u | candidate cells: %zu","输入点：%zu · 屏幕内：%u · 可选格心：%zu") \
 X(GroundDebugLegend,"Colors distinguish input heights; white rings mark selectable cells. Hover a point for XYZ. Points without a ring did not form a selectable grid cell.","彩色点按高度区分，白圈表示可选格心；悬停查看 XYZ。没有白圈的点未形成可选网格格心。") \
 X(GroundDebugPoint,"Input point: X %.4f  Y %.4f  Z %.4f","输入点：X %.4f  Y %.4f  Z %.4f") \
 X(CameraReady,"Game camera connected","游戏主相机已连接") \
 X(CameraWait,"Waiting for the game camera. Enter a level to show paths.","等待游戏主相机；请先进入关卡。路径暂不显示。") \
 X(Anchoring,"Pen/text: screen space. Path Alt+H: anchored to world cells.","画笔和文字固定在画面；路径 Alt+H 固定在游戏格子上。") \
 X(TextTitle,"Text annotation###SketchText","文字标注###SketchText") X(Done,"Done Ctrl+Enter","完成 Ctrl+Enter") X(Cancel,"Cancel Esc","取消 Esc") \
 X(Hud,"SinkingStarSketchPad  E Draw | G Cursor | B Marks | T Camera","SinkingStarSketchPad  E绘图 G指针 B草稿 T相机") \
 X(SaveFailed,"Save failed: ","保存失败：") X(SettingsFailed,"Could not save the language setting.","无法保存语言设置。") \
 X(Recovered,"Draft recovered from backup.","已从备份恢复草稿。") X(Unreadable,"Could not read the old draft; the original was preserved.","旧草稿无法读取；原文件已保留。") \
 X(Full,"Page or point limit reached. Switch pages or remove marks.","本页或点数已满，请换页或清理笔迹。") \
 X(Cleared,"Page cleared. Ctrl+Z restores it.","本页已清空，Ctrl+Z 可恢复。") \
 X(NoCamera,"Game camera unavailable. Enter a level and try again.","尚未读取到游戏主相机，请回到关卡后重试。") \
 X(Detecting,"Detecting the grid. Please wait before placing points.","正在自动识别关卡格子，稍候再点击。") \
 X(NoPlane,"Could not project this position onto the cell plane.","此处无法投影到格子平面。") \
 X(NeedCorner,"Add a point at the corner first: paths follow rows or columns.","请先点击转弯格：路径沿格子横向或纵向连接。") \
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
}
