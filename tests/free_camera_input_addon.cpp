// Private fixture: only this process receives synthetic window messages.
// Simulate foreground and pointer samples without changing desktop focus/input.
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
static HWND test_window;
static HWND WINAPI test_foreground(){return test_window;}
#define GetForegroundWindow test_foreground
#define AddonInit ProductionAddonInit
#include "../src/addon.cpp"
#undef AddonInit
#undef GetForegroundWindow
namespace {
unsigned tick=0;
free_camera::Controller help_camera;
size_t help_marks=0;
void require(bool ok,const char*message){if(!ok)std::ofstream("free-camera-input-error.txt",std::ios::app)<<message<<'\n';}
void test_overlay(effect_runtime*rt){
    ++tick;test_window=static_cast<HWND>(rt->get_hwnd());
    auto &io=ImGui::GetIO();
    io.MousePos={20,20};io.MouseDelta={0,0};io.MouseWheel=0;io.DeltaTime=1.f/60;
    io.MouseDown[ImGuiMouseButton_Right]=false;
    if(tick==32){auto c=free_camera_bridge::snapshot().camera;require(c.enabled,"Single T did not enable camera");require(c.invert_y&&!c.invert_x,"Default Y inversion not loaded");}
    // Hover an inactive panel while holding W: keyboard must still pan.
    if(tick>=40&&tick<=51)io.MousePos={io.DisplaySize.x-100,io.DisplaySize.y-100};
    if(tick>=65&&tick<=70){io.MouseDown[ImGuiMouseButton_Right]=true;io.MouseDelta={2,-1};}
    if(tick==75)io.MouseWheel=1;
    // Background activity must not control the camera.
    if(tick>=90&&tick<=100)test_window=nullptr;
    if(tick==105){text_open=true;text_buffer[0]='x';text_buffer[1]=0;}
    if(tick==110)text_open=false;
    if(tick==165){
        editing=true;selected=Tool::Pen;language=i18n::Language::English;
        free_camera_bridge::enable(true,camera_plane());help_camera=free_camera_bridge::snapshot().camera;
        help_marks=page().size();shortcuts_open=true;
        PostMessageW(test_window,WM_KEYDOWN,'W',1);PostMessageW(test_window,WM_KEYDOWN,'E',1);
    }
    if(tick==167)ImGui::SetWindowPos(tr(Label::ShortcutsTitle),ImVec2(80,120),ImGuiCond_Always);
    if(tick==170){PostMessageW(test_window,WM_KEYUP,'W',1);PostMessageW(test_window,WM_KEYUP,'E',1);}
    if(tick==172){
        language=i18n::Language::Chinese;help_camera=free_camera_bridge::snapshot().camera;
        PostMessageW(test_window,WM_KEYDOWN,'W',1);PostMessageW(test_window,WM_KEYDOWN,'E',1);
    }
    if(tick==174){io.MouseClicked[ImGuiMouseButton_Left]=true;io.MousePos={20,20};}
    if(tick==175)PostMessageW(test_window,WM_KEYDOWN,VK_ESCAPE,1);
    if(tick==178){PostMessageW(test_window,WM_KEYUP,VK_ESCAPE,1);PostMessageW(test_window,WM_KEYUP,'W',1);PostMessageW(test_window,WM_KEYUP,'E',1);}
    if(tick==182){shortcuts_open=false;set_edit(false);set_edit(true);}
    overlay(rt);
    auto c=free_camera_bridge::snapshot().camera;
    if(tick==51)require(grid::length(c.offset)>.6&&grid::length(c.offset)<1.1,"WASD blocked or default speed too fast");
    if(tick==71)require(std::abs(c.yaw)>.5&&std::abs(c.pitch)>.2,"Right drag failed to rotate/tilt");
    if(tick==76)require(c.zoom>1,"Wheel failed to zoom");
    static free_camera::Controller before_blur;
    if(tick==89)before_blur=c;
    if(tick==100)require(grid::length(c.offset-before_blur.offset)<.0001,"Background key moved camera");
    if(tick==109)require(c.enabled,"Typing T disabled camera");
    if(tick==115){require(!c.active_view(),"T did not restore the game view");require(!io.WantCaptureKeyboard,"Held view still captures game keyboard");}
    if(tick==135){require(c.enabled&&!c.holding,"T did not enable camera after resize");require(grid::length(c.offset)<.0001,"New camera controls did not start from the game view");}
    if(tick==145)require(!c.active_view(),"Esc did not restore camera view");
    if(tick==150)std::ofstream("free-camera-input-result.txt")<<"PASS: T toggle, W over panel, right drag, wheel, blur, text entry, resize, T/Esc restored view and keyboard release\n";
    if(tick==155){
        set_language(i18n::Language::English);
        free_camera_bridge::preferences(2,.12,true,false);camera_preferences_dirty=true;save_camera_preferences();
        free_camera_bridge::preferences(5,.08,false,true);load_camera_preferences();load_language();
        auto restored=free_camera_bridge::snapshot().camera;
        require(restored.speed==2&&restored.sensitivity==.12&&restored.invert_x&&!restored.invert_y,"Camera preferences did not round-trip");
        require(language==i18n::Language::English,"Camera settings overwrote language");
        std::ofstream("free-camera-input-result.txt",std::ios::app)<<"PASS: speed/sensitivity/X/Y settings persist independently of language\n";
    }
    if(tick==170){
        require(shortcuts_open&&!editing,"E did not hide drawing while retaining the help's open state");
        require(grid::length(c.offset-help_camera.offset)>.05,"Help blocked camera movement");
    }
    if(tick==174){
        require(shortcuts_open&&editing,"E did not restore drawing with help still open");
        require(page().size()==help_marks+1&&!dragging,"Help blocked drawing outside its window");
        require(grid::length(c.offset-help_camera.offset)>.01,"Visible help blocked camera movement");
        ImGui::Begin(tr(Label::ShortcutsTitle),nullptr,ImGuiWindowFlags_NoSavedSettings|ImGuiWindowFlags_NoCollapse|ImGuiWindowFlags_NoNav|ImGuiWindowFlags_NoFocusOnAppearing);
        auto pos=gui_vec2(imgui_function_table_instance()->GetWindowPos);
        require(std::abs(pos.x-80)<1&&std::abs(pos.y-120)<1,"Help position changed after E toggles or a language change");
        ImGui::End();
    }
    if(tick==180){
        require(shortcuts_open&&editing&&!c.enabled,"Help consumed Esc instead of restoring the game camera");
        require(!io.WantCaptureKeyboard&&io.WantCaptureMouse,"Help captured ordinary game keys");
    }
    if(tick==183){
        require(!shortcuts_open&&editing,"Manually closed help reopened with the toolbar");
        std::ofstream("free-camera-input-result.txt",std::ios::app)<<"PASS: movable help preserves position/open state across E; camera, drawing and keyboard input remain available\n";
    }
}
}
extern "C" __declspec(dllexport) bool AddonInit(HMODULE addon,HMODULE runtime){if(!ProductionAddonInit(addon,runtime))return false;reshade::unregister_event<reshade::addon_event::reshade_overlay>(overlay);reshade::register_event<reshade::addon_event::reshade_overlay>(test_overlay);return true;}
