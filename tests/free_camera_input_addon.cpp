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
    overlay(rt);
    auto c=free_camera_bridge::snapshot().camera;
    if(tick==51)require(grid::length(c.offset)>.6&&grid::length(c.offset)<1.1,"WASD blocked or default speed too fast");
    if(tick==71)require(std::abs(c.yaw)>.5&&std::abs(c.pitch)>.2,"Right drag failed to rotate/tilt");
    if(tick==76)require(c.zoom>1,"Wheel failed to zoom");
    static free_camera::Controller before_blur;
    if(tick==89)before_blur=c;
    if(tick==100)require(grid::length(c.offset-before_blur.offset)<.0001,"Background key moved camera");
    if(tick==109)require(c.enabled,"Typing T disabled camera");
    if(tick==115){require(!c.enabled&&c.holding,"T did not retain the view");require(!io.WantCaptureKeyboard,"Held view still captures game keyboard");}
    if(tick==135){require(c.enabled&&!c.holding,"T did not resume camera after resize");require(grid::length(c.offset-before_blur.offset)<.0001,"Resume lost retained offset");}
    if(tick==145)require(!c.enabled&&c.holding,"Esc did not retain camera view");
    if(tick==150)std::ofstream("free-camera-input-result.txt")<<"PASS: T toggle, W over panel, right drag, wheel, blur, text entry, resize, T/Esc held view and keyboard release\n";
    if(tick==155){
        set_language(i18n::Language::English);
        free_camera_bridge::preferences(2,.12,true,false,false);camera_preferences_dirty=true;save_camera_preferences();
        free_camera_bridge::preferences(5,.08,false,true,true);load_camera_preferences();load_language();
        auto restored=free_camera_bridge::snapshot().camera;
        require(restored.speed==2&&restored.sensitivity==.12&&restored.invert_x&&!restored.invert_y&&!restored.keep_view,"Camera preferences did not round-trip");
        require(language==i18n::Language::English,"Camera settings overwrote language");
        std::ofstream("free-camera-input-result.txt",std::ios::app)<<"PASS: speed/sensitivity/X/Y/keep-view settings persist independently of language\n";
    }
}
}
extern "C" __declspec(dllexport) bool AddonInit(HMODULE addon,HMODULE runtime){if(!ProductionAddonInit(addon,runtime))return false;reshade::unregister_event<reshade::addon_event::reshade_overlay>(overlay);reshade::register_event<reshade::addon_event::reshade_overlay>(test_overlay);return true;}
