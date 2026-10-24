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
 X(Help,"Middle click: follow (auto) / mark once (manual) | E / Esc: return | Shift: snap | Right click / Ctrl+Z: undo | Ctrl+Y: redo","中键：跟随（自动）/ 标定一次（手动）· E / Esc 返回游戏 · Shift 吸附 · 右键 / Ctrl+Z 撤销 · Ctrl+Y 重做") \
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
 X(PathHelp,"Click centers in a row or column. Add a point at each turn.","点击格心加点，转弯处加点。") \
 X(GridWait,"Waiting for native ground. Automatic mode uses the controlled character; manual mode needs a marked plane.","等待原生地面。自动模式按当前角色选面；手动模式请先标记平面。") \
 X(GroundMark,"Mark plane","标记平面") X(GroundFinish,"Cancel marking","取消标记") \
 X(GroundHelp,"Left-click a visible ground cell to set the plane and finish. A wall or empty space keeps marking active. Esc cancels.","左键点击可见地面格，成功后自动结束标记；命中墙面或空处会保持标记。Esc 取消。") \
 X(GroundManual,"Manual marking","手动标识") X(GroundFollow,"Follow character","随角色移动") \
 X(GroundFollowing,"Following the controlled character.","已开启随角色移动。") X(GroundFrozen,"Target plane frozen.","目标平面已固定。") \
 X(GroundCharacterWait,"Controlled character unavailable. Automatic updates are waiting.","当前控制角色暂不可用，自动选面等待恢复。") \
 X(GroundLoading,"Native ground is loading or unavailable.","原生地面正在加载或暂不可用。") \
 X(GroundWall,"A wall blocks this position. Point at a visible ground cell.","此处被墙面遮挡，请指向可见的地面格。") \
 X(GroundShortcut,"Middle click on canvas: automatic mode toggles following; manual mode marks once and exits, including on failure.","画布中键：自动模式切换随角色移动；手动模式快速标定一次，无论成功与否均结束标记。") \
 X(GroundLimits,"Occlusion checks native solid walls. Decorative models, platforms and lilies are not included.","遮挡仅检查原生实心墙体；暂不包含装饰模型、升降台和莲叶。") \
 X(GroundHover,"Ground height %.3g","地面高度 %.3g") \
 X(GroundLocked,"Target plane: height %.3g","目标平面：高度 %.3g") X(GroundAuto,"Automatic marking","自动标识") \
 X(GroundNoHit,"No native ground cell under the pointer.","指针下没有原生地面格。") \
 X(GroundAnchored,"Target plane set. Marking finished.","目标平面已设置，标记已结束。") \
 X(NativeDebug,"Debug: preview native ground cells","Debug：预览原生地面格") \
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
 X(CameraWait,"Waiting for the game camera. Enter a level to show paths.","等待游戏主相机；请先进入关卡。路径暂不显示。") \
 X(Anchoring,"Pen/text: screen space. Path Alt+H: anchored to world cells.","画笔和文字固定在画面；路径 Alt+H 固定在游戏格子上。") \
 X(TextTitle,"Text annotation###SketchText","文字标注###SketchText") X(Done,"Done Ctrl+Enter","完成 Ctrl+Enter") X(Cancel,"Cancel Esc","取消 Esc") \
 X(Hud,"SinkingStarSketchPad  E Draw | G Cursor | B Marks | T Camera","SinkingStarSketchPad  E绘图 G指针 B草稿 T相机") \
 X(SaveFailed,"Save failed: ","保存失败：") X(SettingsFailed,"Could not save the language setting.","无法保存语言设置。") \
 X(Recovered,"Draft recovered from backup.","已从备份恢复草稿。") X(Unreadable,"Could not read the old draft; the original was preserved.","旧草稿无法读取；原文件已保留。") \
 X(Full,"Page or point limit reached. Switch pages or remove marks.","本页或点数已满，请换页或清理笔迹。") \
 X(Cleared,"Page cleared. Ctrl+Z restores it.","本页已清空，Ctrl+Z 可恢复。") \
 X(NoCamera,"Game camera unavailable. Enter a level and try again.","尚未读取到游戏主相机，请回到关卡后重试。") \
 X(Detecting,"No target plane. Wait for automatic ground or mark a plane manually.","尚未确定目标平面，请等待自动识别或手动标记平面。") \
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
