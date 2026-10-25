// Deterministic native integration fixture. The distributed add-on never
// enables or moves the camera automatically.
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
static HWND preview_window;
static bool camera_preview(){return wcsstr(GetCommandLineW(),L"--camera-preview")!=nullptr;}
static HWND WINAPI fixture_foreground(){return camera_preview()?preview_window:GetForegroundWindow();}
#define GetForegroundWindow fixture_foreground
#define AddonInit ProductionAddonInit
#include "../src/addon.cpp"
#undef AddonInit
#undef GetForegroundWindow
namespace {
unsigned test_frame=0;
void test_overlay(effect_runtime*rt){
    preview_window=static_cast<HWND>(rt->get_hwnd());
    if(camera_preview()&&test_frame==5)language=wcsstr(GetCommandLineW(),L"--english")?i18n::Language::English:i18n::Language::Chinese;
    overlay(rt);++test_frame;
    if(test_frame==10){
        visible=true;
    }
    if(test_frame==12){
        auto path=std::make_shared<Stroke>();path->tool=Tool::Path;path->color=1;path->width=3;path->lattice={true,{0,0,0},{1,0,0},{0,1,0}};
        for(int i=-3;i<=3;++i)path->points.push_back({float(i),1});page().push_back(path);
    }
    if(test_frame==30&&!free_camera_bridge::enable(true,0))std::ofstream("free-camera-test-error.txt")<<"Source camera unavailable";
    if(test_frame==70){auto s=free_camera_bridge::snapshot();auto c=s.camera;c.offset={2,1,1};c.rotate(25,35);c.magnify(3);free_camera_bridge::change(c);}
    if(test_frame==110&&!camera_preview())free_camera_bridge::enable(false,0);
    if(test_frame==150&&!camera_preview())free_camera_bridge::enable(true,0);
    if(test_frame==170&&!camera_preview())free_camera_bridge::restore();
}
}
extern "C" __declspec(dllexport) bool AddonInit(HMODULE addon,HMODULE runtime){if(!ProductionAddonInit(addon,runtime))return false;reshade::unregister_event<reshade::addon_event::reshade_overlay>(overlay);reshade::register_event<reshade::addon_event::reshade_overlay>(test_overlay);return true;}
