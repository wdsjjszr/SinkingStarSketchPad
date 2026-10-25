#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <imgui.h>
#include <reshade.hpp>
#include "imgui_abi.hpp"
#include "ui_style.hpp"
#include "model.hpp"
#include "drawing_math.hpp"
#include "grid_preview.hpp"
#include "grid_visibility.hpp"
#include "preview_mesh.hpp"
#include "voxel_clip.hpp"
#include "i18n.hpp"
#include "cursor_style.hpp"
#include "native_ground_bridge.hpp"
#include "native_ground_pick.hpp"
#include "input_policy.hpp"
#include "shape_input.hpp"
#include "free_camera_bridge.hpp"
#include <chrono>
#include <mutex>

using reshade::api::effect_runtime;
using namespace sketch;
namespace {
HMODULE module_handle;
effect_runtime *primary = nullptr;
std::recursive_mutex state_mutex;
Document doc;
std::array<History,3> history;
std::filesystem::path data_path,legacy_draft_source;
ui::ScaleSetting ui_scale;
ui::UIScale frame_ui;
bool ui_preferences_dirty=false,grid_panel_open=false;
ULONGLONG ui_preferences_due=0;
ui::DismissClick ui_dismiss;
ui::Rect toolbar_bounds,grid_button_bounds,grid_panel_bounds;
ui::WindowPosition toolbar_position,settings_position,shortcuts_position,text_window_position,camera_position;
i18n::Language language=i18n::Language::System;
bool chinese(){return i18n::use_chinese(language,PRIMARYLANGID(GetUserDefaultUILanguage())==LANG_CHINESE);}
const char *tr(i18n::Text text){return i18n::get(text,chinese());}
using Label=i18n::Text;
void load_language(){
    wchar_t value[32];auto path=data_path.parent_path()/L"settings.ini";
    GetPrivateProfileStringW(L"Interface",L"Language",L"auto",value,32,path.c_str());
    language=_wcsicmp(value,L"en")==0?i18n::Language::English:_wcsicmp(value,L"zh-CN")==0?i18n::Language::Chinese:i18n::Language::System;
}
void load_camera_preferences(){
    auto path=data_path.parent_path()/L"settings.ini";
    free_camera_bridge::preferences(GetPrivateProfileIntW(L"Camera",L"SpeedMilli",5000,path.c_str())/1000.,
        GetPrivateProfileIntW(L"Camera",L"SensitivityMilli",80,path.c_str())/1000.,
        GetPrivateProfileIntW(L"Camera",L"InvertX",0,path.c_str())!=0,GetPrivateProfileIntW(L"Camera",L"InvertY",1,path.c_str())!=0);
}
bool loaded=false, editing=false, visible=true, dirty=false, disabled=false, reshade_menu=false;
bool dragging=false, erasing=false, text_open=false, text_focus=false, protected_file=false;
bool settings_open=false;
bool shortcuts_open=false;
Tool selected=Tool::Pen;
unsigned selected_color=0;
float width=3;
Stroke pending;
input::ShapeGesture shape_gesture;
Point shape_press;
bool suppress_right=false,erase_with_right=false;
void settle_drawing();
bool drawing_target(const Stroke &,Point,Point,bool,Point &);
grid::Camera camera;
native_ground::CharacterSample ground_character;
uintptr_t ground_scene=0;
uint64_t ground_generation=0;
bool ground_scene_ready=false,ground_initialized=false;
std::array<bool,3> ground_user_set{};
bool ground_marking=false;
bool native_ground_debug=false;
unsigned native_ground_visible=0;
std::shared_ptr<const native_ground::Snapshot> native_ground_snapshot;
double native_ground_height=std::numeric_limits<double>::quiet_NaN();
native_ground::Pick ground_hit;
enum class GroundAction {None,Character,Manual,Quick};
struct GroundRequest {
    native_ground::RequestTag tag;
    GroundAction action=GroundAction::None;
    bool continue_drawing=false;
    grid::Camera camera;
    Point pointer;
};
GroundRequest ground_request;
void cancel_ground_marking(){
    if(ground_request.tag.id)native_ground_bridge::cancel(ground_request.tag.id);
    ground_marking=false;ground_hit={};ground_request={};
}
bool path_active=false,grid_drawing=false;
grid::Preview grid_preview;
grid::PlaneHint plane_hint;
grid::HintPreferences hint_preferences;
auto wheel_actions=input::default_wheel;
grid::OcclusionCache plane_visibility;
double camera_plane(){const auto &g=doc.grids[doc.page];return g.ready?g.origin.z:0.;}
input::Focus input_focus;
input::WheelState drawing_wheel;
void finish_path(){path_active=false;}
void select_tool(Tool t){settle_drawing();cancel_ground_marking();selected=t;finish_path();}
Page erase_before;
char text_buffer[2049]={};
Point text_position;
grid::Lattice text_lattice;
unsigned text_color=0;
float text_width=3;
ULONGLONG save_due=0, toast_until=0, frame_count=0;
bool camera_preferences_dirty=false;
ULONGLONG camera_preferences_due=0;
bool drawing_preferences_dirty=false;
ULONGLONG drawing_preferences_due=0;
std::string toast;
constexpr ImU32 colors[] = {IM_COL32(255,218,105,255),IM_COL32(109,227,255,255),IM_COL32(255,130,158,255),IM_COL32(152,237,172,255),IM_COL32(255,255,255,255)};
Page &page() { return doc.pages[doc.page]; }
void note(const std::string &s, unsigned ms=4500) { toast=s; toast_until=GetTickCount64()+ms; }
void log(const std::string &s) { reshade::log::message(reshade::log::level::info,s.c_str()); }
void load_ui_preferences(){
    auto path=data_path.parent_path()/L"settings.ini";
    ui_scale=ui::ScaleSetting(int(GetPrivateProfileIntW(L"Interface",L"UIScalePercent",100,path.c_str())));
}
void save_ui_preferences(){
    if(!ui_preferences_dirty||data_path.empty())return;
    auto path=data_path.parent_path()/L"settings.ini";auto value=std::to_wstring(ui_scale.applied);
    if(WritePrivateProfileStringW(L"Interface",L"UIScalePercent",value.c_str(),path.c_str()))ui_preferences_dirty=false;
    else{note(tr(Label::UISettingsFailed));ui_preferences_due=GetTickCount64()+5000;}
}
void save_camera_preferences(){
    if(!camera_preferences_dirty||data_path.empty())return;
    const auto c=free_camera_bridge::snapshot().camera;auto path=data_path.parent_path()/L"settings.ini";
    bool ok=true;
    auto write=[&](const wchar_t*key,int value){auto text=std::to_wstring(value);ok=WritePrivateProfileStringW(L"Camera",key,text.c_str(),path.c_str())!=FALSE&&ok;};
    write(L"SpeedMilli",int(std::lround(c.speed*1000)));write(L"SensitivityMilli",int(std::lround(c.sensitivity*1000)));write(L"InvertX",c.invert_x);write(L"InvertY",c.invert_y);
    if(ok)camera_preferences_dirty=false;else{note(tr(Label::CameraSettingsFailed));camera_preferences_due=GetTickCount64()+5000;}
}
void load_drawing_preferences(){
    auto path=data_path.parent_path()/L"settings.ini";
    auto read=[&](const wchar_t *key,int value){return int(GetPrivateProfileIntW(L"Drawing",key,value,path.c_str()));};
    grid_drawing=read(L"GridMode",0)!=0;grid_preview.enabled=read(L"Preview",1)!=0;
    grid_preview.centers=read(L"Centers",1)!=0;grid_preview.intersections=read(L"Intersections",0)!=0;
    grid_preview.borders=read(L"Borders",0)!=0;grid_preview.checker=read(L"Checker",0)!=0;
    grid_preview.opacity=std::clamp(read(L"Opacity",45),0,100);
    auto &style=hint_preferences.style;
    style.centers=read(L"HintCenters",0)!=0;style.intersections=read(L"HintIntersections",0)!=0;
    style.borders=read(L"HintBorders",0)!=0;style.checker=read(L"HintChecker",1)!=0;style.opacity=std::clamp(read(L"HintOpacity",15),0,100);
    int mode=read(L"HintMode",0);hint_preferences.mode=mode>=0&&mode<=2?grid::HintMode(mode):grid::HintMode::On;
    hint_preferences.duration_ms=unsigned(std::clamp(read(L"HintDurationMs",750),0,5000));
    const wchar_t *keys[]={L"Wheel",L"CtrlWheel",L"ShiftWheel",L"AltWheel"};
    for(unsigned i=0;i<4;++i){int value=read(keys[i],int(input::default_wheel[i]));wheel_actions[i]=value>=0&&value<=3?input::WheelAction(value):input::default_wheel[i];}
    drawing_wheel.reset();
}
void drawing_preferences_changed(){drawing_preferences_dirty=true;drawing_preferences_due=GetTickCount64()+500;}
void save_drawing_preferences(){
    if(!drawing_preferences_dirty||data_path.empty())return;auto path=data_path.parent_path()/L"settings.ini";bool ok=true;
    auto write=[&](const wchar_t *key,int value){auto text=std::to_wstring(value);ok=WritePrivateProfileStringW(L"Drawing",key,text.c_str(),path.c_str())!=FALSE&&ok;};
    write(L"GridMode",grid_drawing);write(L"Preview",grid_preview.enabled);write(L"Centers",grid_preview.centers);
    write(L"Intersections",grid_preview.intersections);write(L"Borders",grid_preview.borders);write(L"Checker",grid_preview.checker);write(L"Opacity",grid_preview.opacity);
    const auto &style=hint_preferences.style;
    write(L"HintCenters",style.centers);write(L"HintIntersections",style.intersections);write(L"HintBorders",style.borders);
    write(L"HintChecker",style.checker);write(L"HintOpacity",style.opacity);write(L"HintMode",int(hint_preferences.mode));write(L"HintDurationMs",int(hint_preferences.duration_ms));
    const wchar_t *keys[]={L"Wheel",L"CtrlWheel",L"ShiftWheel",L"AltWheel"};for(unsigned i=0;i<4;++i)write(keys[i],int(wheel_actions[i]));
    if(ok)drawing_preferences_dirty=false;else{note(tr(Label::DrawingSettingsFailed));drawing_preferences_due=GetTickCount64()+5000;}
}
void status() {
    if (data_path.empty()) return;
    std::ofstream f(data_path.parent_path()/L"status.txt");
    auto free=free_camera_bridge::snapshot();
    f << "camera_offset=" << free.camera.offset.x << ',' << free.camera.offset.y << ',' << free.camera.offset.z
      << "\ncamera_held=" << free.camera.holding << "\ncamera_angles=" << free.camera.yaw << ',' << free.camera.pitch << "\ncamera_zoom=" << free.camera.zoom << '\n';
    f << "version=3.4.6\nprocess=" << GetCurrentProcessId() << "\nrenderer=DirectX12\nediting=" << editing
      << "\npage=" << doc.page+1 << "\nstrokes=" << page().size() << "\ncursor=" << doc.cursor
      << "\nlanguage=" << i18n::language_code(language) << "\nui_language=" << (chinese()?"zh-CN":"en") << "\nvisible=" << visible << "\nforeground=" << input_focus.active << "\nframes=" << frame_count << "\ncamera=" << camera.valid << "\ncamera_frame=" << camera.frame << "\ngrid=" << doc.grids[doc.page].ready << "\ngrid_centers=" << (native_ground_snapshot?native_ground_snapshot->cells.size():0) << "\nfree_camera=" << free.camera.enabled << "\nfree_camera_ready=" << free.ready << "\nfree_camera_patches=" << free.patches << "\n";
    f << "grid_drawing=" << grid_drawing << "\ngrid_preview=" << grid_preview.enabled << "\ngrid_preview_centers=" << grid_preview.centers
      << "\ngrid_preview_intersections=" << grid_preview.intersections << "\ngrid_preview_borders=" << grid_preview.borders
      << "\ngrid_preview_checker=" << grid_preview.checker << "\ngrid_preview_opacity=" << grid_preview.opacity << "\ngrid_preview_cells=" << grid::preview_cells
      << "\nplane_hint=" << plane_hint.active(GetTickCount64(),doc.page,ground_generation,hint_preferences)
      << "\nshape_gesture=" << unsigned(shape_gesture.phase) << "\ndraft_format=" << doc.format_version
      << "\nink_color=" << selected_color << "\nworld_strokes=" << std::count_if(page().begin(),page().end(),[](const auto &stroke){return stroke->lattice.ready;}) << '\n';
    f << "settings_open=" << settings_open << "\nhint_mode=" << unsigned(hint_preferences.mode) << "\nhint_duration_ms=" << hint_preferences.duration_ms
      << "\nhint_centers=" << hint_preferences.style.centers << "\nhint_intersections=" << hint_preferences.style.intersections
      << "\nhint_borders=" << hint_preferences.style.borders << "\nhint_checker=" << hint_preferences.style.checker << "\nhint_opacity=" << hint_preferences.style.opacity
      << "\npath_active=" << path_active << "\nright_erasing=" << erase_with_right
      << "\nhint_source=" << unsigned(plane_hint.source) << "\nhint_started=" << plane_hint.started << '\n';
    for(unsigned i=0;i<4;++i)f << "wheel_binding_" << i << '=' << unsigned(wheel_actions[i]) << '\n';
    f << "ground_marking=" << ground_marking << "\nground_request_pending=" << bool(ground_request.tag.id)
      << "\nground_scene_ready=" << ground_scene_ready << "\nground_generation=" << ground_generation
      << "\nground_ready=" << doc.grids[doc.page].ready << "\nground_z=" << camera_plane()
      << "\nground_layers=" << (native_ground_snapshot?native_ground_snapshot->heights.size():0)
      << "\nground_character_ready=" << ground_character.valid << "\nground_character_id=" << ground_character.entity_id
      << "\nground_controlled_count=" << ground_character.controlled_count
      << "\nground_character_position=" << ground_character.position.x << ',' << ground_character.position.y << ',' << ground_character.position.z
      << "\nground_character_error=" << ground_character.error << '\n';
    f << "native_ground_debug=" << native_ground_debug << "\nnative_ground_visible=" << native_ground_visible
      << "\nnative_ground_ready=" << (native_ground_snapshot&&native_ground_snapshot->ready)
      << "\nnative_ground_scene=" << (native_ground_snapshot?native_ground_snapshot->scene:"")
      << "\nnative_ground_cells=" << (native_ground_snapshot?native_ground_snapshot->cells.size():0)
      << "\nnative_ground_heights=" << (native_ground_snapshot?native_ground_snapshot->heights.size():0)
      << "\nnative_ground_voxels=" << (native_ground_snapshot?native_ground_snapshot->voxel_count:0)
      << "\nnative_ground_error=" << (native_ground_snapshot?native_ground_snapshot->error:"") << '\n';
}
void set_language(i18n::Language value){
    language=value;auto path=data_path.parent_path()/L"settings.ini";
    const wchar_t *code=value==i18n::Language::English?L"en":value==i18n::Language::Chinese?L"zh-CN":L"auto";
    if(!WritePrivateProfileStringW(L"Interface",L"Language",code,path.c_str()))note(tr(Label::SettingsFailed));
    status();
}
void changed() { dirty=true; save_due=GetTickCount64()+700; status(); }
bool save() {
    if (!dirty) return true;
    try {
        if (protected_file) throw std::runtime_error("Old draft is unreadable and could not be backed up");
        if(doc.format_version<4&&!legacy_draft_source.empty()){
            const auto keep=data_path.parent_path()/L"drafts.pre-v4.txt";
            if(!std::filesystem::exists(keep)){
                auto temporary=keep.wstring()+L"."+std::to_wstring(GetTickCount64())+L".tmp";
                std::filesystem::copy_file(legacy_draft_source,temporary,std::filesystem::copy_options::none);
                if(!MoveFileExW(temporary.c_str(),keep.c_str(),MOVEFILE_WRITE_THROUGH))throw std::runtime_error("Cannot preserve the pre-v4 draft backup");
            }
        }
        const auto temp=data_path.wstring()+L".tmp", backup=data_path.wstring()+L".bak";
        { std::ofstream out(temp.c_str(),std::ios::binary|std::ios::trunc); doc.write(out); out.flush(); if(!out) throw std::runtime_error("Write failed"); }
        HANDLE file=CreateFileW(temp.c_str(),GENERIC_WRITE,FILE_SHARE_READ,nullptr,OPEN_EXISTING,0,nullptr);
        if(file==INVALID_HANDLE_VALUE) throw std::runtime_error("Cannot flush draft");
        const bool flushed=FlushFileBuffers(file)!=FALSE; CloseHandle(file);
        if(!flushed) throw std::runtime_error("Flush failed");
        BOOL ok;
        if(std::filesystem::exists(data_path)) ok=ReplaceFileW(data_path.c_str(),temp.c_str(),backup.c_str(),0,nullptr,nullptr);
        else ok=MoveFileExW(temp.c_str(),data_path.c_str(),MOVEFILE_WRITE_THROUGH);
        if(!ok) throw std::runtime_error("Cannot replace draft file: "+std::to_string(GetLastError()));
        dirty=false;doc.format_version=4;legacy_draft_source.clear();return true;
    } catch(const std::exception &e) { note(std::string(tr(Label::SaveFailed))+e.what(),15000); log(toast); save_due=GetTickCount64()+15000; return false; }
}
void load() {
    wchar_t path[32768]; DWORD n=GetModuleFileNameW(module_handle,path,32768);
    if(!n || n>=32768) throw std::runtime_error("Cannot locate addon directory");
    auto dir=std::filesystem::path(path).parent_path()/L"SinkingStarSketch-data";
    std::filesystem::create_directories(dir); data_path=dir/L"drafts.txt";load_language();load_ui_preferences();load_camera_preferences();load_drawing_preferences();
    auto read=[&](const std::filesystem::path &p) {
        if(std::filesystem::file_size(p)>16*1024*1024) throw std::runtime_error("Draft file too large");
        std::ifstream in(p,std::ios::binary);auto result=Document::read(in);
        legacy_draft_source=result.format_version<4?p:std::filesystem::path{};return result;
    };
    if(std::filesystem::exists(data_path)) {
        try { doc=read(data_path); }
        catch(const std::exception &) {
            std::error_code ec;
            auto keep=data_path.wstring()+L".unreadable-"+std::to_wstring(GetTickCount64());
            std::filesystem::copy_file(data_path,keep,std::filesystem::copy_options::none,ec);
            protected_file=bool(ec);
            try { doc=read(data_path.wstring()+L".bak"); note(tr(Label::Recovered),8000); }
            catch(...) { note(tr(Label::Unreadable),10000); }
        }
    }
    for(auto &g:doc.grids)g={};
    loaded=true; status(); log("SinkingStarSketchPad initialized; rendering inside the game swap chain.");
}
void commit() {
    if(dragging&&shape_gesture.can_commit()) {
        if(page().size()<2000 && doc.point_count()+pending.points.size()<=120000) {
            history[doc.page].record(page()); page().push_back(std::make_shared<const Stroke>(std::move(pending))); changed();
        } else note(tr(Label::Full));
        dragging=false;shape_gesture.clear();
    }
    if(erasing) { if(page().size()!=erase_before.size()) { history[doc.page].record(erase_before); changed(); } erasing=false;erase_with_right=false; erase_before.clear(); }
}
void cancel() { dragging=false;shape_gesture.clear(); if(erasing) {page()=std::move(erase_before); erasing=false;erase_with_right=false;} }
void settle_drawing(){commit();if(dragging)cancel();}
void finish_text() {
    if(text_buffer[0] && page().size()<2000 && doc.point_count()<120000) {
        Stroke s;s.tool=Tool::Text;s.color=text_color;s.width=text_width;s.lattice=text_lattice;s.points={text_position};s.text=text_buffer;
        history[doc.page].record(page());page().push_back(std::make_shared<const Stroke>(std::move(s)));changed();
    }
    text_open=false; text_buffer[0]=0;
}
void set_edit(bool value) { settle_drawing(); if(text_open) finish_text(); editing=value;if(value)visible=true;else{grid_panel_open=false;cancel_ground_marking();finish_path();}status();if(!value)save(); }
void undo() { finish_path(); cancel(); if(history[doc.page].back(page()))changed(); }
void redo() { finish_path(); cancel(); if(history[doc.page].forward(page()))changed(); }
void clear() { finish_path(); settle_drawing(); if(!page().empty()){history[doc.page].record(page());page().clear();changed();note(tr(Label::Cleared));} }
ImVec2 v(Point p) {return ImVec2(p.x,p.y);}
void polyline(ImDrawList *draw,const std::vector<Point>&p,ImU32 color,float w) {
    if(p.empty())return;
    if(p.size()==1){draw->AddCircleFilled(v(p[0]),w*.5f,color);return;}
    std::vector<ImVec2> vertices;vertices.reserve(p.size());for(auto a:p)vertices.push_back(v(a));
    draw->AddPolyline(vertices.data(),static_cast<int>(vertices.size()),color,ImDrawFlags_None,w);
}
std::vector<Point> projected(const Stroke &s,Point size){return project_stroke(s,camera,size);}
void path_click(Point mouse,Point size){
    if(!camera.valid){note(tr(Label::NoCamera));return;}
    double x=mouse.x/size.x,y=mouse.y/size.y;
    auto &g=doc.grids[doc.page];
    if(!g.ready){note(tr(Label::Detecting),6000);return;}
    double i,j;if(!g.snap(camera,x,y,i,j)){note(tr(Label::NoPlane));return;}
    Point target{float(i),float(j)};
    const bool append=path_active&&!page().empty()&&page().back()->tool==Tool::Path;
    auto stroke=append?std::make_shared<Stroke>(*page().back()):std::make_shared<Stroke>();
    if(!append){stroke->tool=Tool::Path;stroke->color=selected_color;stroke->width=width;stroke->lattice=g;}
    size_t extra=1;
    if(append){
        target.z=float(std::round(g.origin.z-stroke->lattice.origin.z));
        auto last=stroke->points.back();int dx=int(target.x-last.x),dy=int(target.y-last.y),dz=int(target.z-last.z);
        if(!dx&&!dy&&!dz){finish_path();note(tr(Label::PathFinished),1500);status();return;}
        if(path_axes(last,target)>1){note(tr(Label::NeedCorner));return;}
        extra=size_t(std::abs(dx)+std::abs(dy)+std::abs(dz));
        if(extra>1000){note(tr(Label::TooFar));return;}
        if(stroke->points.size()+extra>10000||doc.point_count()+extra>120000){note(tr(Label::PathFull));return;}
        for(size_t n=1;n<=extra;++n)stroke->points.push_back({last.x+float((dx>0)-(dx<0))*float(n),last.y+float((dy>0)-(dy<0))*float(n),last.z+float((dz>0)-(dz<0))*float(n)});
    }else stroke->points.push_back(target);
    if((!append&&page().size()>=2000)||doc.point_count()+extra>120000){note(tr(Label::Full));return;}
    history[doc.page].record(page());if(append)page().back()=stroke;else page().push_back(stroke);
    path_active=true;changed();
}
void draw_preview_mesh(ImDrawList *draw,const grid::PreviewMesh &mesh,double opacity=1){
    auto uv=gui_vec2(imgui_function_table_instance()->GetFontTexUvWhitePixel);
    for(size_t offset=0;offset<mesh.triangles.size();){
        unsigned count=unsigned(std::min(size_t(grid::preview_batch_triangles),mesh.triangles.size()-offset));draw->PrimReserve(int(count*3),int(count*3));
        for(unsigned n=0;n<count;++n){const auto &triangle=mesh.triangles[offset++];unsigned base=draw->_VtxCurrentIdx;
            for(unsigned v=0;v<3;++v)draw->PrimWriteIdx(ImDrawIdx(base+v));
            for(auto point:triangle.points){int alpha=int(std::lround(std::clamp(point.alpha*opacity,0.,1.)*255));draw->PrimWriteVtx(ImVec2(float(point.x),float(point.y)),uv,IM_COL32(triangle.shade,triangle.shade,triangle.shade,alpha));}
        }
    }
}
grid::PreviewMesh preview_mesh(const grid::Preview &style,const grid::Lattice &g,const grid::CellMask &mask,Point size,double fade=1,bool radial=false,double i=0,double j=0){
    grid::PreviewPainter painter{camera,g,size.x,size.y,style.opacity*.01*fade,i,j,radial,{}};painter.layers(style,mask);return std::move(painter.mesh);
}
void draw_grid_preview(ImDrawList *draw,Point mouse,Point size){
    if((selected!=Tool::Path&&!grid_drawing)||!camera.valid)return;
    const auto &g=doc.grids[doc.page];double i,j;if(!g.coordinates(camera,mouse.x/size.x,mouse.y/size.y,i,j))return;
    if(grid_preview.enabled)draw_preview_mesh(draw,preview_mesh(grid_preview,g,grid::CellMask(int(std::round(i))-grid::preview_radius,int(std::round(j))-grid::preview_radius,grid::preview_cells,grid::preview_cells),size,1,true,i,j));
}
void draw_snap_marker(ImDrawList *draw,Point mouse,Point size,bool shift){
    if((selected!=Tool::Path&&!grid_drawing)||!camera.valid||erasing)return;
    const auto &g=doc.grids[doc.page];Point target;auto lattice=g;unsigned color=selected_color;
    if(dragging&&snaps_to_grid(selected)){
        if(!drawing_target(pending,mouse,size,shift,target))return;lattice=pending.lattice;color=pending.color;
    }else{
        double i,j;if(!g.coordinates(camera,mouse.x/size.x,mouse.y/size.y,i,j))return;
        if(selected==Tool::Path)target={float(std::round(i)),float(std::round(j))};
        else if(snaps_to_grid(selected))target=nearest_grid_point({float(i),float(j)});else return;
    }
    Point pixel;if(screen_point(lattice,target,camera,size,pixel))draw->AddCircle(v(pixel),9,colors[color],24,2.5f);
}
bool ground_ready(){
    return ground_scene_ready&&native_ground_snapshot&&native_ground_snapshot->ready&&native_ground_snapshot->scene_id==ground_scene;
}
bool draw_plane_hint(ImDrawList *draw,Point size){
    if(!camera.valid||!hint_preferences.style.visible()||!plane_hint.active(GetTickCount64(),doc.page,ground_generation,hint_preferences))return false;
    const auto &g=plane_hint.lattice;double i,j;if(!grid::preview_focus(camera,g,i,j))return false;
    int x=int(std::floor(i+.5)),y=int(std::floor(j+.5));
    plane_visibility.update(camera,g,ground_ready()?native_ground_snapshot:nullptr,x-64,y-64,size.x,size.y,hint_preferences.style);
    draw_preview_mesh(draw,plane_visibility.mesh,hint_preferences.style.opacity*.01*grid::fade_weight(double(GetTickCount64()-plane_hint.started),hint_preferences.duration_ms));
    return !plane_visibility.mesh.triangles.empty();
}
bool preserve_z_shape(){return dragging&&pending.lattice.ready&&cross_height_shape(pending.tool)&&shape_gesture.waiting();}
void set_ground(double z,grid::HintSource source=grid::HintSource::Mark,bool continue_drawing=false){
    auto &g=doc.grids[doc.page];
    bool different=!g.ready||g.origin.z!=z||g.origin.x!=0||g.origin.y!=0||g.u.x!=1||g.v.y!=-1;
    if(different){if(!continue_drawing&&(source!=grid::HintSource::Wheel||!preserve_z_shape()))settle_drawing();g=native_ground::lattice(z);if(!continue_drawing&&source!=grid::HintSource::Wheel)finish_path();}
    ground_user_set[doc.page]=true;plane_hint.show(g,GetTickCount64(),doc.page,ground_generation,source,hint_preferences);
    if(different)changed();else status();
}
void request_ground(GroundAction action,Point mouse={},Point size={}){
    const bool keep_shape=action==GroundAction::Quick&&dragging&&pending.lattice.ready&&cross_height_shape(pending.tool);
    const bool keep_path=action==GroundAction::Quick&&path_active;
    if(keep_shape)shape_gesture.await_end();else settle_drawing();
    if(!keep_path)finish_path();cancel_ground_marking();plane_hint.clear();
    if(action==GroundAction::Manual){select_tool(Tool::Path);ground_marking=true;}
    if(!ground_scene_ready){note(tr(Label::GroundLoading));status();return;}
    if(action==GroundAction::Quick&&!camera.valid){note(tr(Label::NoCamera));status();return;}
    auto kind=action==GroundAction::Character?native_ground_bridge::Read::Character:native_ground_bridge::Read::Ground;
    ground_request.tag={native_ground_bridge::request(kind,ground_generation),ground_generation,doc.page};
    ground_request.action=action;
    ground_request.continue_drawing=keep_shape||keep_path;
    if(action==GroundAction::Quick){ground_request.camera=camera;ground_request.pointer={mouse.x/size.x,mouse.y/size.y};}
    status();
}
void update_ground_pick(Point mouse,Point size){
    ground_hit=!ground_request.tag.id&&ground_ready()?native_ground::pick(*native_ground_snapshot,camera,mouse.x/size.x,mouse.y/size.y):native_ground::Pick{};
}
void anchor_ground(bool quick=false,bool continue_drawing=false){
    if(ground_hit.result==native_ground::PickResult::Ground){set_ground(ground_hit.center.z,grid::HintSource::Mark,continue_drawing);note(tr(Label::GroundAnchored),2500);}
    else note(tr(!camera.valid?Label::NoCamera:!ground_ready()?Label::GroundLoading:
        ground_hit.result==native_ground::PickResult::Wall?Label::GroundWall:Label::GroundNoHit));
    ground_marking=native_ground::keep_marking(quick,ground_hit.result);status();
}
void poll_ground(){
    auto state=native_ground_bridge::snapshot();ground_character=state.character;ground_scene_ready=state.scene.ready;
    if(state.generation!=ground_generation){
        if(ground_generation)free_camera_bridge::restore();
        settle_drawing();drawing_wheel.reset();
        ground_generation=state.generation;ground_scene=state.scene.id;ground_initialized=false;ground_user_set={};plane_hint.clear();plane_visibility={};
        for(auto &g:doc.grids)g={};cancel_ground_marking();finish_path();native_ground_height=std::numeric_limits<double>::quiet_NaN();
    }
    if(state.ground!=native_ground_snapshot){
        if(!state.ground||(!std::isnan(native_ground_height)&&!state.ground->heights.count(native_ground_height)))native_ground_height=std::numeric_limits<double>::quiet_NaN();
        native_ground_snapshot=state.ground;
    }
    const bool initialize_current=!ground_initialized&&!ground_user_set[doc.page];
    if(ground_scene_ready&&state.initialized&&native_ground::initialize_planes(doc.grids,ground_user_set,state.initial_z,ground_initialized)){
        if(initialize_current)settle_drawing();
        finish_path();changed();
    }
    if(!ground_request.tag.matches(state.completed,ground_generation,doc.page))return;
    const auto requested=ground_request;ground_request={};
    if(requested.action==GroundAction::Character){
        const auto &character=state.character_reply;
        if(ground_scene_ready&&character.valid&&character.scene_id==ground_scene){set_ground(character.position.z);note(tr(Label::GroundCharacterSet),2500);}
        else note(tr(Label::GroundCharacterWait));
    }else if(requested.action==GroundAction::Quick){
        ground_hit=ground_ready()?native_ground::pick(*native_ground_snapshot,requested.camera,requested.pointer.x,requested.pointer.y):native_ground::Pick{};
        anchor_ground(true,requested.continue_drawing);
    }else if(!ground_ready())note(tr(Label::GroundLoading));
    status();
}
void draw_ground_preview(ImDrawList *draw,Point size){
    if(ground_hit.result==native_ground::PickResult::Wall){ImGui::SetTooltip("%s",tr(Label::GroundWall));return;}
    if(ground_hit.result!=native_ground::PickResult::Ground)return;
    const auto center=ground_hit.center;
    const grid::V world[]={{center.x-.5,center.y-.5,center.z},{center.x+.5,center.y-.5,center.z},
                          {center.x+.5,center.y+.5,center.z},{center.x-.5,center.y+.5,center.z}};ImVec2 corners[4];
    for(unsigned n=0;n<4;++n){double x,y;if(!camera.project(world[n],x,y))return;corners[n]=ImVec2(float(x*size.x),float(y*size.y));}
    draw->AddConvexPolyFilled(corners,4,IM_COL32(109,227,255,65));
    draw->AddPolyline(corners,4,IM_COL32(109,227,255,240),ImDrawFlags_Closed,2);
    ImGui::SetTooltip(tr(Label::GroundHover),center.z);
}
const char *native_ground_source(native_ground::Source source){
    return tr(source==native_ground::Source::Voxel?Label::NativeVoxel:source==native_ground::Source::Base?Label::NativeBase:Label::NativeFence);
}
bool native_ground_corners(grid::V center,Point size,ImVec2 (&corners)[4]){
    const grid::V offsets[]={{-.5,-.5,0},{.5,-.5,0},{.5,.5,0},{-.5,.5,0}};
    for(unsigned n=0;n<4;++n){double x,y;if(!camera.project(center+offsets[n],x,y))return false;corners[n]=ImVec2(float(x*size.x),float(y*size.y));}
    return true;
}
void draw_native_ground_debug(ImDrawList *draw,Point mouse,Point size,bool over_ui){
    native_ground_visible=0;if(!ground_ready()||!camera.valid)return;
    const native_ground::Cell *nearest=nullptr;double nearest_distance=100;
    for(const auto &cell:native_ground_snapshot->cells){
        if(!std::isnan(native_ground_height)&&cell.center.z!=native_ground_height)continue;
        double x,y;if(!camera.project(cell.center,x,y)||x<0||x>=1||y<0||y>=1)continue;
        const float px=float(x*size.x),py=float(y*size.y);float red,green,blue;
        double hue=std::fmod(cell.center.z*.61803398875,1.);if(hue<0)hue+=1;
        ImGui::ColorConvertHSVtoRGB(float(hue),.7f,1.f,red,green,blue);
        const auto color=IM_COL32(int(red*255),int(green*255),int(blue*255),230);
        ImVec2 corners[4];
        if(native_ground_corners(cell.center,size,corners)&&std::hypot(corners[1].x-corners[0].x,corners[1].y-corners[0].y)>6)
            draw->AddPolyline(corners,4,IM_COL32(int(red*255),int(green*255),int(blue*255),cell.source==native_ground::Source::Base?40:95),ImDrawFlags_Closed,1);
        if(cell.source==native_ground::Source::Base)draw->AddCircle(ImVec2(px,py),3,color,8,1);
        else if(cell.source==native_ground::Source::Fence){draw->AddLine(ImVec2(px-4,py),ImVec2(px+4,py),color,1.5f);draw->AddLine(ImVec2(px,py-4),ImVec2(px,py+4),color,1.5f);}
        else draw->AddCircleFilled(ImVec2(px,py),3,color,8);
        ++native_ground_visible;
        const double distance=(px-mouse.x)*(px-mouse.x)+(py-mouse.y)*(py-mouse.y);
        if(distance<nearest_distance){nearest_distance=distance;nearest=&cell;}
    }
    if(nearest&&!over_ui){
        ImVec2 corners[4];if(native_ground_corners(nearest->center,size,corners))draw->AddPolyline(corners,4,IM_COL32(255,255,255,240),ImDrawFlags_Closed,2);
        ImGui::SetTooltip(tr(Label::NativeHover),native_ground_source(nearest->source),nearest->entity_id,nearest->center.x,nearest->center.y,nearest->center.z);
    }
}
void draw_stroke(ImDrawList *draw,const Stroke&s,Point size) {
    auto p=projected(s,size);if(p.empty())return;auto a=p.front();
    ImU32 c=colors[s.color],shadow=IM_COL32(9,14,23,210);
    if(s.tool==Tool::Text) {
        float fs=std::max(20.f,s.width*4+12);
        draw->AddText(ImGui::GetFont(),fs,ImVec2(a.x+1.5f,a.y+1.5f),shadow,s.text.c_str());
        draw->AddText(ImGui::GetFont(),fs,v(a),c,s.text.c_str());return;
    }
    polyline(draw,p,shadow,s.width+3);polyline(draw,p,c,s.width);
    if(s.tool==Tool::Path){
        for(size_t i=0;i<p.size();++i){draw->AddCircleFilled(v(p[i]),5,shadow);draw->AddCircleFilled(v(p[i]),3, c);}
        if(!p.empty()){draw->AddCircle(v(p.front()),8,c,24,2);draw->AddCircle(v(p.back()),6,c,24,2);}
    }
    if(s.tool==Tool::Arrow){auto head=arrow_head(p,s.width);polyline(draw,head,shadow,s.width+3);polyline(draw,head,c,s.width);}
}
void erase_at(Point p,Point size) {
    auto &marks=page();
    marks.erase(std::remove_if(marks.begin(),marks.end(),[&](const auto&s){
        auto points=projected(*s,size);if(points.empty())return false;
        if(s->tool==Tool::Text) {
            Point a=points[0];ImVec2 measure=gui_text_size(s->text.c_str());float scale=std::max(20.f,s->width*4+12)/ImGui::GetFontSize();
            return p.x>=a.x-8&&p.y>=a.y-8&&p.x<=a.x+measure.x*scale+8&&p.y<=a.y+measure.y*scale+8;
        }
        return projected_hit(points,p,s->width)||(s->tool==Tool::Arrow&&projected_hit(arrow_head(points,s->width),p,s->width));
    }),marks.end());
}
void export_svg(Point size) {
    try {
        std::filesystem::path outdir=data_path.parent_path()/L"exports";std::filesystem::create_directories(outdir);
        std::ofstream out(outdir/(L"draft-"+std::to_wstring(doc.page+1)+L"-"+std::to_wstring(GetTickCount64())+L".svg"));
        out<<"<svg xmlns=\"http://www.w3.org/2000/svg\" width=\""<<size.x<<"\" height=\""<<size.y<<"\" viewBox=\"0 0 "<<size.x<<' '<<size.y<<"\">\n";
        const char *hex[]={"#ffda69","#6de3ff","#ff829e","#98edac","#ffffff"};
        for(const auto&s:page()) {
            auto p=projected(*s,size);if(p.empty())continue;const char *c=hex[s->color];
            if(s->tool==Tool::Text) {
                out<<"<text x=\""<<p[0].x<<"\" y=\""<<p[0].y+std::max(20.f,s->width*4+12)<<"\" fill=\""<<c<<"\" font-size=\""<<std::max(20.f,s->width*4+12)<<"\" font-family=\"Microsoft YaHei,sans-serif\" xml:space=\"preserve\">";
                for(char ch:s->text){if(ch=='&')out<<"&amp;";else if(ch=='<')out<<"&lt;";else if(ch=='>')out<<"&gt;";else if(ch=='\n')out<<"<tspan x=\""<<p[0].x<<"\" dy=\"1.2em\">&#160;</tspan>";else out<<ch;}out<<"</text>\n";
            } else {
                if(p.size()==1)out<<"<circle cx=\""<<p[0].x<<"\" cy=\""<<p[0].y<<"\" r=\""<<s->width*.5f<<"\" fill=\""<<c<<"\"/>\n";
                else {out<<"<polyline fill=\"none\" stroke=\""<<c<<"\" stroke-width=\""<<s->width<<"\" stroke-linecap=\"round\" stroke-linejoin=\"round\" points=\"";for(auto pt:p)out<<pt.x<<','<<pt.y<<' ';out<<"\"/>\n";}
                if(s->tool==Tool::Arrow){auto head=arrow_head(p,s->width);if(!head.empty()){out<<"<polyline fill=\"none\" stroke=\""<<c<<"\" stroke-width=\""<<s->width<<"\" points=\"";for(auto pt:head)out<<pt.x<<','<<pt.y<<' ';out<<"\"/>\n";}}
            }
        }
        out<<"</svg>\n";out.flush();if(!out)throw std::runtime_error("Export failed");note(tr(Label::Exported));
    }catch(const std::exception&e){note(std::string(tr(Label::ExportFailed))+e.what());}
}
template<size_t N> void help_section(Label title,const i18n::ShortcutEntry (&rows)[N],ui::UIScale s){
    ImGui::PushID(int(title));ui::label(tr(title),s,ui::typography::section,ui::muted);
    float width=gui_vec2(imgui_function_table_instance()->GetContentRegionAvail).x;
    if(ImGui::BeginTable("##keys",2,ImGuiTableFlags_NoSavedSettings|ImGuiTableFlags_SizingStretchProp)){
        ImGui::TableSetupColumn("",ImGuiTableColumnFlags_WidthFixed,std::min(s.px(115),width*.42f));
        ImGui::TableSetupColumn("",ImGuiTableColumnFlags_WidthStretch);
        for(const auto &row:rows){
            const char *key=i18n::shortcut_key(row,chinese());
            ImGui::TableNextRow();ImGui::TableNextColumn();
            {
                ui::FontScope small(11.f/12);auto p=gui_vec2(imgui_function_table_instance()->GetCursorScreenPos);
                float available=gui_vec2(imgui_function_table_instance()->GetContentRegionAvail).x,wrap=std::max(1.f,available-s.px(10));
                auto measure=gui_text_size(key,wrap);auto *draw=ImGui::GetWindowDrawList();
                ImVec2 end=ui::vec(p.x+std::min(available,measure.x+s.px(10)),p.y+measure.y+s.px(4));
                draw->AddRectFilled(p,end,IM_COL32(18,20,26,255),s.px(5));draw->AddRect(p,end,ui::border,s.px(5),0,s.px(1));
                draw->AddText(ImGui::GetFont(),ImGui::GetFontSize(),ui::vec(ui::pixel_round(p.x+s.px(5)),ui::pixel_round(p.y+s.px(2))),ui::ink,key,nullptr,wrap);
                ImGui::Dummy(ui::vec(available,measure.y+s.px(4)));
            }
            ImGui::TableNextColumn();ui::label(i18n::get(row,chinese()),s,ui::typography::body,IM_COL32(201,207,219,255),true);
        }
        ImGui::EndTable();
    }
    ImGui::PopID();
}
void shortcut_help(Point size,Point mouse,bool &over_ui) {
    if(!shortcuts_open)return;
    auto s=frame_ui;ui::FontScope font(s.ratio());ui::Style style(s);
    ui::place_window(shortcuts_position,ui::vec(size.x*.5f,size.y*.5f),ImGuiCond_FirstUseEver,ui::vec(.5f,.5f));
    ui::panel_size(std::min(s.px(560),size.x-s.px(16)),size.y-s.px(16));
    if(ImGui::Begin(tr(Label::ShortcutsTitle),nullptr,ui::panel_flags|ImGuiWindowFlags_NoNav|ImGuiWindowFlags_NoFocusOnAppearing|ImGuiWindowFlags_NoScrollbar|ImGuiWindowFlags_NoScrollWithMouse)){
        ui::header(tr(Label::ShortcutsTitle),&shortcuts_open,s,shortcuts_position);
        if(ui::begin_body(size.y-s.px(80))){
            if(ImGui::BeginTable("##columns",2,ImGuiTableFlags_BordersInnerV|ImGuiTableFlags_SizingStretchSame|ImGuiTableFlags_NoSavedSettings)){
                ImGui::TableNextRow();ImGui::TableNextColumn();
                help_section(Label::ShortcutGeneralTitle,i18n::general_shortcuts,s);ImGui::Spacing();ImGui::Separator();
                help_section(Label::ShortcutDrawingTitle,i18n::drawing_shortcuts,s);ui::label(tr(Label::DrawingGestures),s,ui::typography::note,ui::muted,true);
                ImGui::TableNextColumn();help_section(Label::ShortcutToolsTitle,i18n::tool_shortcuts,s);ImGui::Spacing();ImGui::Separator();
                help_section(Label::ShortcutCameraTitle,i18n::camera_shortcuts,s);ui::label(tr(Label::CameraPrefsHelp),s,ui::typography::note,ui::muted,true);
                ImGui::EndTable();
            }
            ImGui::Separator();ui::label(tr(Label::KeyboardHelp),s,ui::typography::note,ui::muted,true);ui::label(tr(Label::WindowMoveHelp),s,ui::typography::caption,ui::muted,true);
        }
        ImGui::EndChild();
    }
    auto r=ui::finish_panel(size.x,size.y,s,&shortcuts_position);over_ui|=r.contains(mouse.x,mouse.y);
}
bool segmented(const char *id,const char *const *labels,int count,int &selected,ui::UIScale s){
    bool changed=false;ImGui::PushID(id);ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize,0);ImGui::PushStyleVar(ImGuiStyleVar_FramePadding,ui::vec(s.px(6),s.px(4)));
    for(int i=0;i<count;++i){
        if(i)ImGui::SameLine(0,s.px(2));ImGui::PushID(i);bool active=selected==i;
        ui::FontScope weight(1,active);
        ImGui::PushStyleColor(ImGuiCol_Button,active?ui::gold:IM_COL32(18,20,26,255));ImGui::PushStyleColor(ImGuiCol_Text,active?IM_COL32(26,20,5,255):ui::ink);
        if(ImGui::Button(labels[i])){changed=selected!=i;selected=i;}
        ImGui::PopStyleColor(2);ImGui::PopID();
    }
    ImGui::PopStyleVar(2);ImGui::PopID();return changed;
}
bool setting_rows(const char *id){
    if(!ImGui::BeginTable(id,2,ImGuiTableFlags_SizingStretchProp|ImGuiTableFlags_NoSavedSettings))return false;
    ImGui::TableSetupColumn("",ImGuiTableColumnFlags_WidthStretch,.42f);ImGui::TableSetupColumn("",ImGuiTableColumnFlags_WidthStretch,.58f);return true;
}
void setting_label(Label label){ImGui::TableNextRow();ImGui::TableNextColumn();ImGui::AlignTextToFramePadding();ImGui::TextWrapped("%s",tr(label));ImGui::TableNextColumn();}
void native_debug_settings(){
    auto s=frame_ui;ImGui::Separator();ui::label(tr(Label::DebugTitle),s,ui::typography::section,ui::muted);ImGui::SameLine();ui::label(tr(Label::DeveloperOnly),s,ui::typography::section,ui::gold);
    if(ui::toggle(tr(Label::NativeDebug),native_ground_debug,s)){native_ground_height=std::numeric_limits<double>::quiet_NaN();status();}
    if(!native_ground_debug)return;
    if(!native_ground_snapshot)ImGui::TextWrapped("%s",tr(Label::NativeLoading));
    else if(!native_ground_snapshot->ready)ImGui::TextWrapped(tr(Label::NativeError),native_ground_snapshot->error.c_str());
    else{
        const auto &native=*native_ground_snapshot;char height[64];std::snprintf(height,sizeof(height),"Z %.4f",native_ground_height);
        if(setting_rows("##native")){
            setting_label(Label::NativeHeight);ImGui::SetNextItemWidth(-FLT_MIN);
            if(ImGui::BeginCombo("##height",std::isnan(native_ground_height)?tr(Label::NativeAll):height)){
                if(ImGui::Selectable(tr(Label::NativeAll),std::isnan(native_ground_height)))native_ground_height=std::numeric_limits<double>::quiet_NaN();
                for(double z:native.heights){std::snprintf(height,sizeof(height),"Z %.4f",z);if(ImGui::Selectable(height,native_ground_height==z))native_ground_height=z;}
                ImGui::EndCombo();
            }
            ImGui::EndTable();
        }
        ui::FontScope note(s.px(ui::typography::note.size)/ImGui::GetFontSize());ImGui::PushStyleColor(ImGuiCol_Text,ui::muted);
        ImGui::TextWrapped(tr(Label::NativeStats),native.scene.c_str(),native.cells.size(),native_ground_visible,native.heights.size());
        ImGui::TextWrapped(tr(Label::NativeSources),native.wall_sets,native.voxel_count,native.fences,native.oceans);ImGui::PopStyleColor();
    }
    ui::label(tr(Label::NativeLegend),s,ui::typography::note,ui::muted,true);ui::label(tr(Label::NativeLimits),s,ui::typography::note,ui::muted,true);
}
void drawing_settings(Point size,Point mouse,bool &over_ui){
    if(!settings_open)return;
    auto s=frame_ui;ui::FontScope font(s.ratio());ui::Style theme(s);
    ui::place_window(settings_position,ui::vec(size.x*.5f,size.y*.5f),ImGuiCond_FirstUseEver,ui::vec(.5f,.5f));
    ui::panel_size(std::min(s.px(360),size.x-s.px(16)),size.y-s.px(16));
    if(ImGui::Begin(tr(Label::SettingsTitle),nullptr,(ui::panel_flags&~ImGuiWindowFlags_NoNavInputs)|ImGuiWindowFlags_NoScrollbar|ImGuiWindowFlags_NoScrollWithMouse)){
        ui::header(tr(Label::SettingsTitle),&settings_open,s,settings_position);
        if(ui::begin_body(size.y-s.px(80),0)){
            if(setting_rows("##interface")){
                setting_label(Label::Language);int choice=int(language);const char *languages[]={tr(Label::LanguageSystem),"English",tr(Label::LanguageChinese)};
                if(segmented("##language",languages,3,choice,s))set_language(i18n::Language(choice));
                setting_label(Label::UIScale);float available=gui_vec2(imgui_function_table_instance()->GetContentRegionAvail).x;
                ui::slider("##scale",ui_scale.pending,75,200,available-s.px(45),s);
                ImGui::SameLine();ui::percent_value(ui_scale.pending,s);
                ImGui::TableNextRow();ImGui::TableSetColumnIndex(1);ImGui::BeginDisabled(ui_scale.pending==ui_scale.applied);
                if(ImGui::Button(tr(Label::ApplyUIScale))&&ui_scale.apply()){ui_preferences_dirty=true;ui_preferences_due=GetTickCount64()+500;}
                ImGui::EndDisabled();ImGui::EndTable();
            }
            ImGui::Separator();ui::label(tr(Label::WheelSettings),s,ui::typography::section,ui::muted);
            bool changed=false;
            if(setting_rows("##wheel")){
                const char *actions[]={tr(Label::WheelNone),tr(Label::WheelPlane),tr(Label::WheelColor),tr(Label::WheelTool)};constexpr Label bindings[]={Label::Wheel,Label::CtrlWheel,Label::ShiftWheel,Label::AltWheel};
                for(unsigned i=0;i<4;++i){
                    setting_label(bindings[i]);ImGui::PushID(int(i));int action=int(wheel_actions[i]);ImGui::SetNextItemWidth(-FLT_MIN);
                    if(ImGui::Combo("##action",&action,actions,4)){wheel_actions[i]=input::WheelAction(action);drawing_wheel.reset();changed=true;}ImGui::PopID();
                }
                ImGui::EndTable();
            }
            ui::label(tr(Label::WheelHelp),s,ui::typography::note,ui::muted,true);ImGui::Separator();ui::label(tr(Label::HintSettings),s,ui::typography::section,ui::muted);
            if(setting_rows("##hint")){
                setting_label(Label::HintMode);const char *modes[]={tr(Label::HintOn),tr(Label::HintOff),tr(Label::HintWheelOnly)};int mode=int(hint_preferences.mode);
                if(segmented("##mode",modes,3,mode,s)){hint_preferences.mode=grid::HintMode(mode);changed=true;}
                setting_label(Label::HintDuration);float seconds=hint_preferences.duration_ms/1000.f;ImGui::SetNextItemWidth(-FLT_MIN);
                if(ImGui::DragFloat("##duration",&seconds,.01f,0,5,"%.2f",ImGuiSliderFlags_AlwaysClamp)){hint_preferences.duration_ms=unsigned(std::lround(seconds*1000));changed=true;}
                ImGui::EndTable();
            }
            ImGui::Separator();ui::label(tr(Label::HintStyle),s,ui::typography::section,ui::muted);
            auto &preview=hint_preferences.style;
            if(ImGui::BeginTable("##layers",2,ImGuiTableFlags_SizingStretchSame|ImGuiTableFlags_NoSavedSettings)){
                ImGui::TableNextRow();ImGui::TableNextColumn();changed|=ui::checkbox(tr(Label::GridCenters),preview.centers,s);ImGui::TableNextColumn();changed|=ui::checkbox(tr(Label::GridIntersections),preview.intersections,s);
                ImGui::TableNextRow();ImGui::TableNextColumn();changed|=ui::checkbox(tr(Label::GridBorders),preview.borders,s);ImGui::TableNextColumn();changed|=ui::checkbox(tr(Label::GridChecker),preview.checker,s);ImGui::EndTable();
            }
            if(setting_rows("##opacity")){
                setting_label(Label::GridOpacity);float available=gui_vec2(imgui_function_table_instance()->GetContentRegionAvail).x;
                changed|=ui::slider("##opacity",preview.opacity,0,100,available-s.px(40),s);ImGui::SameLine();ui::percent_value(preview.opacity,s);ImGui::EndTable();
            }
            if(changed){plane_hint.expire(GetTickCount64(),doc.page,ground_generation,hint_preferences);drawing_preferences_changed();status();}
            native_debug_settings();
        }
        ImGui::EndChild();
    }
    auto r=ui::finish_panel(size.x,size.y,s,&settings_position);over_ui|=r.contains(mouse.x,mouse.y);
}
void grid_preview_controls(Point size,Point mouse,bool &over_ui){
    if(!grid_panel_open)return;
    auto s=frame_ui;
    auto placement=ui::beside(toolbar_bounds,grid_button_bounds.y,s.px(224),grid_panel_bounds.h>0?grid_panel_bounds.h:s.px(210),size.x,size.y,s.px(8));
    ImGui::SetNextWindowPos(ui::vec(placement.x,placement.y),ImGuiCond_Always);
    ui::panel_size(placement.w,size.y-s.px(16));
    if(ImGui::Begin("##SketchGridPreview",nullptr,ui::panel_flags|ImGuiWindowFlags_NoMove)){
        ui::label(tr(Label::GridPreview),s,{12,true});ImGui::Separator();
        bool changed=ui::checkbox(tr(Label::GridPreview),grid_preview.enabled,s);
        changed|=ui::checkbox(tr(Label::GridCenters),grid_preview.centers,s);
        changed|=ui::checkbox(tr(Label::GridIntersections),grid_preview.intersections,s);
        changed|=ui::checkbox(tr(Label::GridBorders),grid_preview.borders,s);
        changed|=ui::checkbox(tr(Label::GridChecker),grid_preview.checker,s);
        ImGui::AlignTextToFramePadding();ui::label(tr(Label::GridOpacity),s,ui::typography::note,ui::muted);
        float available=gui_vec2(imgui_function_table_instance()->GetContentRegionAvail).x;
        changed|=ui::slider("##opacity",grid_preview.opacity,0,100,available-s.px(38),s);ImGui::SameLine();ui::percent_value(grid_preview.opacity,s);
        if(changed){drawing_preferences_changed();status();}
    }
    grid_panel_bounds=ui::finish_panel(size.x,size.y,s);over_ui|=grid_panel_bounds.contains(mouse.x,mouse.y);
    if((ImGui::IsMouseClicked(ImGuiMouseButton_Left)||ImGui::IsMouseClicked(ImGuiMouseButton_Right)||ImGui::IsMouseClicked(ImGuiMouseButton_Middle))
       &&!grid_panel_bounds.contains(mouse.x,mouse.y)&&!grid_button_bounds.contains(mouse.x,mouse.y)){
        grid_panel_open=false;ui_dismiss.consume();
    }
}
void controls(effect_runtime*,Point size,Point mouse,bool &over_ui) {
    auto s=frame_ui;ui::FontScope font(s.ratio());ui::Style style(s);
    ui::place_window(toolbar_position,ui::vec(s.px(16),std::max(s.px(8),size.y-s.px(455))));
    ui::panel_size(std::min(s.px(256),size.x-s.px(16)),size.y-s.px(16));
    if(ImGui::Begin(tr(Label::Title),nullptr,ui::panel_flags)){
        float available=gui_vec2(imgui_function_table_instance()->GetContentRegionAvail).x;
        ImGui::InvisibleButton("##title",ui::vec(std::max(s.px(1),available-s.px(114)),s.px(28)));
        auto title=ui::item_rect();ui::text(ui::vec(title.x,title.y+s.px(6)),"SketchPad",s,ui::typography::title.size,ui::ink,ui::typography::title.bold);
        ui::drag_title(toolbar_position);
        for(unsigned i=0;i<3;++i){
            ImGui::SameLine(0,s.px(2));ImGui::PushID(int(i));
            bool active=doc.page==i;
            if(active){ImGui::PushStyleColor(ImGuiCol_Button,ui::gold);ImGui::PushStyleColor(ImGuiCol_Text,IM_COL32(26,20,5,255));}
            ui::FontScope weight(1,active);
            char number[]={char('1'+i),0};
            if(ImGui::Button(number,ui::vec(s.px(24),s.px(28)))){settle_drawing();finish_path();cancel_ground_marking();plane_hint.clear();doc.page=i;changed();}
            if(active)ImGui::PopStyleColor(2);
            char keys[16];std::snprintf(keys,sizeof(keys),"Ctrl+%u",i+1);ui::hint(tr(Label::Page),keys);ImGui::PopID();
        }
        ImGui::SameLine(0,s.px(2));if(ui::button("##return",ui::Icon::Return,nullptr,s.px(28),s.px(28),s))set_edit(false);ui::hint(tr(Label::Return),"E");
        ImGui::Separator();
        constexpr Label tools[]={Label::Pen,Label::Line,Label::Arrow,Label::Rectangle,Label::Ellipse,Label::Text,Label::Eraser,Label::Path};
        float tool_width=(available-3*ImGui::GetStyle().ItemSpacing.x)/4;
        for(unsigned i=0;i<8;++i){
            if(i%4)ImGui::SameLine();ImGui::PushID(int(i));
            if(ui::button("##tool",ui::Icon(i),tr(tools[i]),tool_width,s.px(50),s,selected==Tool(i),ui::Tone::Gold,true)){commit();select_tool(Tool(i));}
            ui::hint(tr(tools[i]),i18n::tool_shortcuts[i].key);ImGui::PopID();
        }
        ImGui::Separator();
        for(unsigned i=0;i<std::size(colors);++i){
            if(i)ImGui::SameLine(0,(available-s.px(32)*5)/4);ImGui::PushID(int(i));
            if(ImGui::InvisibleButton("##color",ui::vec(s.px(32),s.px(34))))selected_color=i;
            auto r=ui::item_rect();auto *d=ImGui::GetWindowDrawList();auto center=ui::vec(r.x+r.w*.5f,r.y+r.h*.5f);
            d->AddCircle(center,s.px(15),selected_color==i?ui::gold:ui::border,24,s.px(2));d->AddCircleFilled(center,s.px(12),colors[i],24);
            char key[16];std::snprintf(key,sizeof(key),"Alt+%u",i+1);ui::hint(tr(Label::Color),key);ImGui::PopID();
        }
        ImGui::AlignTextToFramePadding();ui::label(tr(Label::Width),s,ui::typography::note,ui::muted);ImGui::SameLine();
        ui::slider("##width",width,1,16,gui_vec2(imgui_function_table_instance()->GetContentRegionAvail).x-s.px(25),s);ImGui::SameLine();
        {ui::FontScope note(s.px(ui::typography::note.size)/ImGui::GetFontSize());ImGui::Text("%.0f",width);}
        ImGui::Separator();
        {ui::FontScope note(s.px(ui::typography::note.size)/ImGui::GetFontSize());ImGui::TextDisabled("%s",tr(Label::GridPlane));
        ImGui::SameLine();if(doc.grids[doc.page].ready)ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(ui::teal),tr(Label::PlaneHeight),camera_plane());else ImGui::TextDisabled("%s",tr(Label::PlaneUnset));}
        float chip_width=(available-s.px(6))*.5f;
        if(ui::button("##grid",ui::Icon::Grid,tr(Label::GridDrawing),chip_width,s.px(38),s,grid_drawing,ui::Tone::Teal)){
            settle_drawing();if(text_open)finish_text();cancel_ground_marking();finish_path();grid_drawing=!grid_drawing;drawing_preferences_changed();status();
        }
        auto grid=ui::item_rect();ImGui::GetWindowDrawList()->AddCircleFilled(ui::vec(grid.x+grid.w-s.px(9),grid.y+grid.h*.5f),s.px(3),grid_drawing?ui::teal:ui::border,16);
        ui::hint(tr(Label::Anchoring));ImGui::SameLine();
        if(ui::button("##preview",ui::Icon::Eye,tr(Label::GridPreview),chip_width,s.px(38),s,grid_preview.enabled,ui::Tone::Teal)){
            settle_drawing();grid_panel_open=!grid_panel_open;if(!grid_panel_open)ui_dismiss.consume();
        }
        grid_button_bounds=ui::item_rect();ui::icon(ui::Icon::Chevron,ui::vec(grid_button_bounds.x+grid_button_bounds.w-s.px(14),grid_button_bounds.y+(grid_button_bounds.h-s.px(10))*.5f),s.px(10),ui::muted);ui::hint(tr(Label::GridPreview));
        if(ui::button("##character",ui::Icon::Character,tr(Label::CharacterPlaneButton),chip_width,s.px(42),s,true,ui::Tone::Primary)){
            commit();if(text_open)finish_text();request_ground(GroundAction::Character);
        }
        ui::hint(tr(Label::GroundCharacter));ImGui::SameLine();
        if(ui::button("##mark",ui::Icon::Target,tr(ground_marking?Label::GroundFinish:Label::MarkPlaneButton),chip_width,s.px(42),s,ground_marking,ui::Tone::Teal)){
            commit();if(text_open)finish_text();if(ground_marking){cancel_ground_marking();status();}else request_ground(GroundAction::Manual);
        }
        ui::hint(tr(Label::GroundHelp));
        ui::label(tr(Label::GroundShortcut),s,ui::typography::caption,ui::muted,true);ui::hint(tr(Label::GroundLimits));
        if(ground_marking)ImGui::TextWrapped("%s",tr(Label::GroundHelp));
        if(ground_request.tag.id||!ground_ready())ImGui::TextWrapped("%s",tr(Label::GroundLoading));
        if((grid_drawing||selected==Tool::Path)&&!camera.valid)ImGui::TextWrapped("%s",tr(Label::CameraWait));
        if(selected==Tool::Path){if(ImGui::Button(tr(Label::NewPath)))finish_path();ui::hint(tr(Label::PathHelp));if(!doc.grids[doc.page].ready)ImGui::TextWrapped("%s",tr(Label::GridWait));}
        if(grid_drawing&&(selected==Tool::Line||selected==Tool::Arrow)){ImGui::TextWrapped("%s",tr(Label::VerticalSnapHelp));}
        auto free=free_camera_bridge::snapshot().camera;
        if(free.holding){ImGui::TextWrapped("%s",tr(Label::CameraHeld));if(ImGui::Button(tr(Label::CameraOff)))free_camera_bridge::restore();}
        ImGui::Separator();
        constexpr ui::Icon icons[]={ui::Icon::Undo,ui::Icon::Redo,ui::Icon::Clear,ui::Icon::Save,ui::Icon::Export,ui::Icon::Camera,ui::Icon::Help,ui::Icon::Settings};
        constexpr Label labels[]={Label::Undo,Label::Redo,Label::Clear,Label::Save,Label::Export,Label::CameraToggle,Label::Shortcuts,Label::Settings};
        constexpr const char *shortcuts[]={"Ctrl+Z","Ctrl+Y / Ctrl+Shift+Z","Ctrl+Shift+Delete","Ctrl+S",nullptr,"T",nullptr,nullptr};
        for(unsigned i=0;i<8;++i){
            if(i)ImGui::SameLine(0,std::max(0.f,(available-8*s.px(28))/7));ImGui::PushID(int(i));
            bool active=i==5?free.active_view():i==6?shortcuts_open:i==7?settings_open:false;
            if(ui::button("##action",icons[i],nullptr,s.px(28),s.px(28),s,active,ui::Tone::Teal))switch(i){
            case 0:undo();break;case 1:redo();break;case 2:clear();break;
            case 3:commit();if(save())note(tr(Label::Saved));break;case 4:commit();export_svg(size);break;
            case 5:if(!free_camera_bridge::enable(!free.enabled,camera_plane()))note(tr(Label::CameraUnavailable));break;
            case 6:commit();shortcuts_open=!shortcuts_open;break;case 7:settings_open=!settings_open;break;
            }
            ui::hint(tr(labels[i]),shortcuts[i]);ImGui::PopID();
        }
        ImGui::Separator();ui::FontScope footer(s.px(ui::typography::caption.size)/ImGui::GetFontSize());ImGui::TextDisabled(tr(Label::PageStats),doc.page+1,page().size());
        const char *saved=tr(dirty?Label::Saving:Label::Saved);float saved_width=gui_text_size(saved).x;
        ImGui::SameLine(std::max(0.f,available-saved_width+s.px(10)));ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(dirty?ui::muted:ui::teal),"%s",saved);
    }
    toolbar_bounds=ui::finish_panel(size.x,size.y,s,&toolbar_position);over_ui|=toolbar_bounds.contains(mouse.x,mouse.y);
    grid_preview_controls(size,mouse,over_ui);
    if(text_open){
        float w=std::min(s.legacy(440),size.x-s.px(16)),h=std::min(s.legacy(220),size.y-s.px(16));
        auto position=ui::fit({mouse.x,mouse.y,w,h},size.x,size.y,s.px(8));
        ui::place_window(text_window_position,ui::vec(position.x,position.y),ImGuiCond_Appearing);ui::panel_size(w,size.y-s.px(16));
        if(ImGui::Begin(tr(Label::TextTitle),nullptr,ui::panel_flags)){
            ui::header(tr(Label::TextTitle),nullptr,s,text_window_position);
            if(text_focus){ImGui::SetKeyboardFocusHere();text_focus=false;}
            ImGui::InputTextMultiline("##text",text_buffer,sizeof(text_buffer),ui::vec(std::max(s.px(20),w-s.px(20)),std::min(s.legacy(120),size.y*.4f)));
            if(ImGui::Button(tr(Label::Done)))finish_text();ui::hint(tr(Label::Done),"Ctrl+Enter");ImGui::SameLine();if(ImGui::Button(tr(Label::Cancel))){text_open=false;text_buffer[0]=0;}ui::hint(tr(Label::Cancel),"Esc");
        }
        auto r=ui::finish_panel(size.x,size.y,s,&text_window_position);over_ui|=r.contains(mouse.x,mouse.y);
    }
}
void keys(effect_runtime*rt) {
    if(!text_open&&ImGui::GetIO().WantTextInput)return;
    const bool ctrl=rt->is_key_down(VK_CONTROL),shift=rt->is_key_down(VK_SHIFT),alt=rt->is_key_down(VK_MENU);
    if((ground_marking||ground_request.tag.id)&&rt->is_key_pressed(VK_ESCAPE)){cancel_ground_marking();status();return;}
    if(shape_gesture.active()&&rt->is_key_pressed(VK_ESCAPE)){cancel();status();return;}
    if(!text_open&&rt->is_key_pressed(VK_ESCAPE)&&free_camera_bridge::snapshot().camera.active_view()){free_camera_bridge::enable(false,camera_plane());status();return;}
    // These single-letter shortcuts must never consume text or modifier chords.
    if(!text_open&&!ctrl&&!shift&&!alt) {
        if(rt->is_key_pressed('T')&&!ImGui::IsAnyItemActive()){auto c=free_camera_bridge::snapshot();if(!free_camera_bridge::enable(!c.camera.enabled,camera_plane()))note(tr(Label::CameraUnavailable));status();}
        if(rt->is_key_pressed('G')){doc.cursor=!doc.cursor;changed();}
        if(rt->is_key_pressed('E'))set_edit(!editing);
        if(rt->is_key_pressed('B')){if(editing)set_edit(false);visible=!visible;status();}
    }
    if(!editing)return;
    if(rt->is_key_pressed(VK_ESCAPE)){if(text_open){text_open=false;text_buffer[0]=0;}else if(dragging||erasing)cancel();else set_edit(false);return;}
    if(text_open){if(ctrl&&rt->is_key_pressed(VK_RETURN))finish_text();return;}
    if(ctrl){
        if(rt->is_key_pressed('Z')){if(shift)redo();else undo();}
        if(rt->is_key_pressed('Y'))redo();
        if(rt->is_key_pressed('S')){commit();if(save())note(tr(Label::Saved));}
        if(shift&&rt->is_key_pressed(VK_DELETE))clear();
        for(unsigned i=0;i<3;++i)if(rt->is_key_pressed('1'+i)){settle_drawing();finish_path();cancel_ground_marking();plane_hint.clear();doc.page=i;changed();}
    }else if(input::tool_shortcuts(ctrl,shift,alt)){
        const int codes[]={'P','L','A','R','O','T','K','H'};
        for(unsigned i=0;i<8;++i)if(rt->is_key_pressed(codes[i])){commit();select_tool(Tool(i));}
        for(unsigned i=0;i<5;++i)if(rt->is_key_pressed('1'+i))selected_color=i;
    }
}
bool drawing_target(const Stroke &stroke,Point mouse,Point size,bool shift,Point &target){
    if(stroke.lattice.ready){
        const auto &plane=(stroke.tool==Tool::Line||stroke.tool==Tool::Arrow)?doc.grids[doc.page]:stroke.lattice;
        if(!plane.ready||!camera.valid)return false;
        float z=float(plane.origin.z-stroke.lattice.origin.z);bool vertical=vertical_snap(stroke.tool,stroke.points.front(),z,shift);
        if(vertical)target={stroke.points.front().x,stroke.points.front().y,z};
        else if(!plane_point(plane,camera,mouse,size,target))return false;
        target.z=z;target=grid_shape_end(stroke.tool,stroke.points.front(),target,shift);
        if(vertical){Point pixel;if(!screen_point(stroke.lattice,target,camera,size,pixel))return false;}
        return std::abs(target.x)<=1000000&&std::abs(target.y)<=1000000;
    }
    if(shift)mouse=snapped(pixel(stroke.points.front(),size),mouse,stroke.tool==Tool::Rectangle||stroke.tool==Tool::Ellipse);
    target=normal(mouse,size);return true;
}
void canvas(effect_runtime*rt,Point size,Point mouse,bool over_ui) {
    if(suppress_right||erase_with_right)return;
    if(!text_open&&!over_ui&&!ImGui::IsAnyItemActive()&&mouse.x>=0&&mouse.y>=0&&mouse.x<size.x&&mouse.y<size.y
       &&ImGui::IsMouseClicked(ImGuiMouseButton_Middle)){
        request_ground(GroundAction::Quick,mouse,size);
        return;
    }
    if(ground_marking){
        if(!over_ui&&ImGui::IsMouseClicked(ImGuiMouseButton_Left)){
            if(ground_request.tag.id)note(tr(Label::GroundLoading));else anchor_ground();
        }
        return;
    }
    if(ground_request.tag.id)return;
    bool left=ImGui::IsMouseDown(ImGuiMouseButton_Left);
    if(!text_open&&!over_ui&&ImGui::IsMouseClicked(ImGuiMouseButton_Left)&&shape_gesture.waiting()){
        Point target;if(!drawing_target(pending,mouse,size,rt->is_key_down(VK_SHIFT),target)){note(tr(Label::NoPlane));return;}
        pending.points[1]=target;shape_gesture.press();
    }else if(!text_open&&!over_ui&&ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
        if(selected==Tool::Path){path_click(mouse,size);return;}
        if(selected==Tool::Eraser){erase_before=page();erasing=true;erase_with_right=false;}
        else{
            auto lattice=grid_drawing?doc.grids[doc.page]:grid::Lattice{};
            if(grid_drawing&&(!lattice.ready||!camera.valid)){note(tr(!camera.valid?Label::NoCamera:Label::Detecting));return;}
            Point point;if(!plane_point(lattice,camera,mouse,size,point)){note(tr(Label::NoPlane));return;}
            if(selected==Tool::Text){text_position=point;text_lattice=lattice;text_color=selected_color;text_width=width;text_buffer[0]=0;text_open=true;text_focus=true;return;}
            if(lattice.ready&&snaps_to_grid(selected))point=nearest_grid_point(point);
            pending=Stroke();pending.tool=selected;pending.color=selected_color;pending.width=width;pending.lattice=lattice;pending.points={point};
            if(selected!=Tool::Pen){pending.points.push_back(point);shape_gesture.press();shape_press=mouse;}dragging=true;
        }
    }
    if(erasing)erase_at(mouse,size);
    if(shape_gesture.active()&&left)shape_gesture.move(distance(shape_press,mouse)>=ImGui::GetIO().MouseDragThreshold);
    if(dragging&&!over_ui&&mouse.x>=0&&mouse.y>=0&&mouse.x<size.x&&mouse.y<size.y){
        if(pending.tool==Tool::Pen){
            Point point,last;
            if(plane_point(pending.lattice,camera,mouse,size,point)&&pending.points.size()<10000
                &&(!screen_point(pending.lattice,pending.points.back(),camera,size,last)||distance(last,mouse)>=1.4f))pending.points.push_back(point);
        }else{
            Point target;if(drawing_target(pending,mouse,size,rt->is_key_down(VK_SHIFT),target))pending.points[1]=target;
        }
    }
    if(!left&&dragging&&shape_gesture.phase==input::ShapeGesture::Phase::SecondPress&&pending.lattice.ready&&(pending.tool==Tool::Line||pending.tool==Tool::Arrow)){
        Point target;if(!drawing_target(pending,mouse,size,rt->is_key_down(VK_SHIFT),target)){shape_gesture.phase=input::ShapeGesture::Phase::AwaitEnd;note(tr(Label::NoPlane));return;}
        pending.points[1]=target;
    }
    if(!left&&(dragging||erasing)){if(!shape_gesture.active()||shape_gesture.release())commit();}
}
void handle_right_button(Point mouse,Point size,bool over_ui){
    bool down=ImGui::IsMouseDown(ImGuiMouseButton_Right);
    if(!down){suppress_right=false;if(erasing&&erase_with_right){commit();status();}}
    bool owns=editing&&input_focus.canvas_ready()&&!reshade_menu&&!text_open&&!over_ui&&!ImGui::IsAnyItemActive()
        &&!ground_marking&&!ground_request.tag.id&&mouse.x>=0&&mouse.y>=0&&mouse.x<size.x&&mouse.y<size.y;
    if(down&&owns&&ImGui::IsMouseClicked(ImGuiMouseButton_Right)){
        auto action=input::right_action(owns,shape_gesture.active(),path_active);
        if(action==input::RightAction::CancelShape)cancel();
        else if(action==input::RightAction::FinishPath){finish_path();note(tr(Label::PathFinished),1500);}
        else if(action==input::RightAction::Erase){settle_drawing();erase_before=page();erasing=true;erase_with_right=true;}
        suppress_right=true;status();
    }
    if(down&&owns&&erasing&&erase_with_right)erase_at(mouse,size);
}
void cursor(ImDrawList*draw,Point p) {
    const auto mode=!editing?cursor_style::Mode::Pointer:((selected==Tool::Eraser||erase_with_right)?cursor_style::Mode::Eraser:cursor_style::Mode::Drawing);
    float scale=std::clamp(ImGui::GetIO().DisplaySize.y/1440.f,1.f,1.5f);
    cursor_style::draw(draw,v(p),mode,colors[selected_color],scale);
}
void handle_drawing_wheel(effect_runtime *rt,Point mouse,Point size,bool over_ui){
    int binding=input::wheel_binding(rt->is_key_down(VK_CONTROL),rt->is_key_down(VK_SHIFT),rt->is_key_down(VK_MENU));
    auto action=binding>=0?wheel_actions[size_t(binding)]:input::WheelAction::None;
    bool owns=editing&&input_focus.canvas_ready()&&!reshade_menu&&!over_ui&&!text_open&&!ImGui::IsAnyItemActive()
        &&mouse.x>=0&&mouse.y>=0&&mouse.x<size.x&&mouse.y<size.y;
    if(action==input::WheelAction::Plane&&!doc.grids[doc.page].ready)owns=false;
    int steps=drawing_wheel.consume(ImGui::GetIO().MouseWheel,binding,action,owns);if(!steps)return;
    if(action==input::WheelAction::Color){selected_color=input::cycle_index(selected_color,steps,std::size(colors));status();return;}
    if(action==input::WheelAction::Tool){auto tool=Tool(input::cycle_index(unsigned(selected),steps,unsigned(Tool::Path)+1));if(tool!=selected)select_tool(tool);status();return;}
    if(!preserve_z_shape())settle_drawing();cancel_ground_marking();plane_hint.clear();set_ground(camera_plane()+steps,grid::HintSource::Wheel);
}
void camera_controls(Point size,Point mouse,bool foreground,bool &over_ui){
    auto state=free_camera_bridge::snapshot();if(!state.camera.enabled||!foreground)return;auto c=state.camera;
    auto s=frame_ui;ui::FontScope font(s.ratio());ui::LegacyStyle style(s);
    ui::place_window(camera_position,ImVec2(std::max(s.legacy(24),size.x-s.legacy(24)),std::max(s.legacy(24),size.y-s.legacy(24))),ImGuiCond_FirstUseEver,ImVec2(1,1));
    ui::panel_size(std::min(s.legacy(460),size.x-s.px(16)),size.y-s.px(16));
    ImGui::Begin(tr(Label::CameraTitle),nullptr,ImGuiWindowFlags_AlwaysAutoResize|ImGuiWindowFlags_NoSavedSettings|ImGuiWindowFlags_NoNavInputs);
    ImGui::PushTextWrapPos(ImGui::GetFontSize()*28);
    if(ImGui::Button(tr(Label::Shortcuts))){commit();shortcuts_open=!shortcuts_open;if(shortcuts_open&&!editing)set_edit(true);}
    auto slider=[&](Label label,double &value,float low,float high){float x=float(value);bool changed=ImGui::SliderFloat(tr(label),&x,low,high,"%.2f");if(changed)value=x;return changed;};
    bool preferences_changed=slider(Label::CameraSpeed,c.speed,.1f,30);
    preferences_changed|=slider(Label::CameraSensitivity,c.sensitivity,.01f,.3f);
    preferences_changed|=ImGui::Checkbox(tr(Label::CameraInvertX),&c.invert_x);
    preferences_changed|=ImGui::Checkbox(tr(Label::CameraInvertY),&c.invert_y);
    ImGui::TextWrapped("%s",tr(Label::CameraPrefsHelp));
    slider(Label::CameraX,c.offset.x,-1000,1000);slider(Label::CameraY,c.offset.y,-1000,1000);slider(Label::CameraZ,c.offset.z,-1000,1000);
    const auto limits=c.pitch_limits();
    slider(Label::CameraYaw,c.yaw,-180,180);slider(Label::CameraPitch,c.pitch,float(limits[0]),float(limits[1]));slider(Label::CameraZoom,c.zoom,.05f,20);c.limit_angles();
    bool reset=ImGui::Button(tr(Label::CameraReset));bool hold=ImGui::Button(tr(Label::CameraHold));ImGui::SameLine();bool off=ImGui::Button(tr(Label::CameraOff));
    ImGui::TextUnformatted(state.last_patch&&GetTickCount64()-state.last_patch<1500?tr(Label::CameraLive):tr(Label::CameraWaiting));
    ImGui::TextWrapped("%s",tr(Label::CameraLimits));ImGui::PopTextWrapPos();
    auto r=ui::finish_panel(size.x,size.y,s,&camera_position);over_ui|=r.contains(mouse.x,mouse.y);
    if(preferences_changed){free_camera_bridge::preferences(c.speed,c.sensitivity,c.invert_x,c.invert_y);camera_preferences_dirty=true;camera_preferences_due=GetTickCount64()+500;}
    if(off){free_camera_bridge::restore();status();return;}
    if(reset){free_camera_bridge::reset_view(camera_plane());return;}
    free_camera_bridge::change(c);
    if(hold){free_camera_bridge::hold();status();}
}
void camera_navigation(effect_runtime*rt,bool foreground,bool over_ui){
    auto state=free_camera_bridge::snapshot();if(!state.camera.enabled||!foreground)return;auto c=state.camera;
    const bool chord=rt->is_key_down(VK_CONTROL)||rt->is_key_down(VK_MENU);
    // Hovering a panel keeps navigation active; only active widgets own keys.
    if(!text_open&&!chord&&!ImGui::IsAnyItemActive()){
        c.move((rt->is_key_down('D')?1:0)-(rt->is_key_down('A')?1:0),(rt->is_key_down('W')?1:0)-(rt->is_key_down('S')?1:0),
               (rt->is_key_down('Q')?1:0)-(rt->is_key_down('Z')?1:0),ImGui::GetIO().DeltaTime,rt->is_key_down(VK_SHIFT));
        if(!over_ui&&input_focus.canvas_ready()){
            if(!editing&&!suppress_right&&ImGui::IsMouseDown(ImGuiMouseButton_Right))c.mouse_rotate(ImGui::GetIO().MouseDelta.x,ImGui::GetIO().MouseDelta.y);
            if(!editing)c.magnify(ImGui::GetIO().MouseWheel);
        }
    }
    free_camera_bridge::change(c);
}
void overlay(effect_runtime*rt) {
    std::lock_guard<std::recursive_mutex> lock(state_mutex);
    if(disabled)return;
    if(primary==nullptr){if(rt->get_device()->get_api()!=reshade::api::device_api::d3d12)return;primary=rt;}
    if(rt!=primary)return;
    try {
        if(!loaded)load();
        plane_hint.expire(GetTickCount64(),doc.page,ground_generation,hint_preferences);
        ++frame_count;if(frame_count==30)status();
        ImGuiIO &io=ImGui::GetIO();Point size={io.DisplaySize.x,io.DisplaySize.y};if(size.x<1||size.y<1)return;
        free_camera_bridge::viewport(uint32_t(size.x),uint32_t(size.y));
        camera=free_camera_bridge::projection(uint32_t(size.x),uint32_t(size.y));
        frame_ui=ui::UIScale(ui_scale.applied,ImGui::GetFontSize());
        if(frame_count%120==0)status();
        HWND window=static_cast<HWND>(rt->get_hwnd());bool foreground=GetForegroundWindow()==window;
        // ReShade virtualizes Win32 GetCursorPos while it blocks game input.
        // Its ImGui input has the real client position, already scaled to this back buffer.
        Point mouse={io.MousePos.x,io.MousePos.y};
        const bool mouse_down=ImGui::IsMouseDown(ImGuiMouseButton_Left)||ImGui::IsMouseDown(ImGuiMouseButton_Right)||ImGui::IsMouseDown(ImGuiMouseButton_Middle);
        const bool was_foreground=input_focus.active;
        if(input_focus.update(foreground,mouse_down)&&editing){settle_drawing();finish_path();save();status();}
        if(foreground&&!was_foreground)status();
        // Ignore activation-frame keys (including an Alt+Tab chord), and keep
        // an unfinished text dialog intact while another application has focus.
        if(foreground&&was_foreground&&!reshade_menu)keys(rt);
        bool over_ui=false;
        if(!foreground)grid_panel_open=false;
        if(editing&&foreground&&!reshade_menu) {
            controls(rt,size,mouse,over_ui);
            drawing_settings(size,mouse,over_ui);
            shortcut_help(size,mouse,over_ui);
        }
        over_ui|=ui_dismiss.block(mouse_down);
        over_ui|=ImGui::IsPopupOpen(nullptr,ImGuiPopupFlags_AnyPopup);
        if(!reshade_menu)camera_controls(size,mouse,foreground,over_ui);
        poll_ground();
        handle_right_button(mouse,size,over_ui);
        if(!reshade_menu)camera_navigation(rt,foreground,over_ui);
        handle_drawing_wheel(rt,mouse,size,over_ui);
        if(ground_marking&&editing&&foreground&&!over_ui&&!reshade_menu&&input_focus.canvas_ready())update_ground_pick(mouse,size);
        else ground_hit={};
        if(editing&&foreground&&!reshade_menu&&input_focus.canvas_ready())canvas(rt,size,mouse,over_ui);
        if(!reshade_menu){
            const bool free=free_camera_bridge::snapshot().camera.enabled;
            const bool capture_mouse=(editing||free)&&foreground;
            const bool capture_keys=capture_mouse&&(free||input::capture_keyboard(text_open||ImGui::IsAnyItemActive(),rt->is_key_down(VK_CONTROL),rt->is_key_down(VK_MENU)));
            // InputProcessing=1 lets ReShade honor the two capture flags
            // separately. The block request enables add-on input routing even
            // when ReShade's own Home overlay is closed. Set current AND next
            // frame flags so a previously focused widget cannot eat movement.
            if(capture_mouse)rt->block_input_next_frame();
            io.WantCaptureMouse=capture_mouse;io.WantCaptureKeyboard=capture_keys;
            ImGui::SetNextFrameWantCaptureMouse(capture_mouse);ImGui::SetNextFrameWantCaptureKeyboard(capture_keys);
        }
        ImDrawList *draw=ImGui::GetBackgroundDrawList();
        const bool hint_drawn=visible&&foreground&&!reshade_menu&&draw_plane_hint(draw,size);
        if(editing&&foreground&&!over_ui&&!reshade_menu){if(ground_marking)draw_ground_preview(draw,size);else if(!hint_drawn)draw_grid_preview(draw,mouse,size);}
        if(native_ground_debug&&visible&&foreground&&!reshade_menu)draw_native_ground_debug(draw,mouse,size,over_ui);else native_ground_visible=0;
        if(visible){for(const auto&s:page())draw_stroke(draw,*s,size);if(dragging)draw_stroke(draw,pending,size);}
        if(editing&&foreground&&!over_ui&&!reshade_menu&&!ground_marking)draw_snap_marker(draw,mouse,size,rt->is_key_down(VK_SHIFT));
        if(foreground&&!reshade_menu&&(doc.cursor||editing||free_camera_bridge::snapshot().camera.enabled)&&mouse.x>=0&&mouse.y>=0&&mouse.x<size.x&&mouse.y<size.y)cursor(ImGui::GetForegroundDrawList(),mouse);
        if(!editing&&!reshade_menu){
            auto s=frame_ui;ui::FontScope font(s.ratio());ImDrawList*d=ImGui::GetForegroundDrawList();
            float right=std::min(size.x-s.legacy(18),s.legacy(42)+gui_text_size(tr(Label::Hud)).x);
            d->AddRectFilled(ImVec2(s.legacy(18),size.y-s.legacy(47)),ImVec2(right,size.y-s.legacy(14)),IM_COL32(18,27,41,205),s.legacy(7));
            d->AddText(ImVec2(s.legacy(30),size.y-s.legacy(41)),colors[0],tr(Label::Hud));
        }
        if(GetTickCount64()<toast_until){
            auto s=frame_ui;ui::FontScope font(s.ratio());ui::Style style(s);
            ImGui::SetNextWindowPos(ImVec2(size.x*.5f,s.legacy(22)),ImGuiCond_Always,ImVec2(.5f,0));
            ImGui::SetNextWindowSizeConstraints(ImVec2(0,0),ImVec2(size.x-s.px(16),size.y-s.px(16)));
            ImGui::Begin("##SketchToast",nullptr,ImGuiWindowFlags_NoDecoration|ImGuiWindowFlags_AlwaysAutoResize|ImGuiWindowFlags_NoInputs|ImGuiWindowFlags_NoSavedSettings);
            ImGui::PushTextWrapPos(std::max(s.px(20),std::min(s.px(500),size.x-s.px(36))));ImGui::TextWrapped("%s",toast.c_str());ImGui::PopTextWrapPos();ImGui::End();
        }
        if(dirty&&(!dragging||shape_gesture.waiting())&&!erasing&&GetTickCount64()>=save_due)save();
        if(camera_preferences_dirty&&GetTickCount64()>=camera_preferences_due)save_camera_preferences();
        if(drawing_preferences_dirty&&GetTickCount64()>=drawing_preferences_due)save_drawing_preferences();
        if(ui_preferences_dirty&&GetTickCount64()>=ui_preferences_due)save_ui_preferences();
    } catch(const std::exception&e) {log(std::string("Sketch disabled after error: ")+e.what());disabled=true;editing=false;}
    catch(...) {log("Sketch disabled after unexpected exception");disabled=true;editing=false;}
}
bool opened(effect_runtime*rt,bool open,reshade::api::input_source) {
    std::lock_guard<std::recursive_mutex> lock(state_mutex);
    if(rt==primary){reshade_menu=open;if(open)set_edit(false);}return false;
}
void destroyed(effect_runtime*rt) {
    std::lock_guard<std::recursive_mutex> lock(state_mutex);
    // ResizeBuffers/fullscreen transitions also destroy this runtime. Retain
    // editing intent and the text buffer for the replacement swap chain.
    if(rt==primary){settle_drawing();save();primary=nullptr;reshade_menu=false;input_focus={};if(text_open)text_focus=true;}
}
}
extern "C" __declspec(dllexport) const char *NAME="SinkingStarSketchPad";
extern "C" __declspec(dllexport) const char *DESCRIPTION="In-game drawing, scratch pages and cell-snapped paths for Order of the Sinking Star. English and Simplified Chinese.";
extern "C" __declspec(dllexport) bool AddonInit(HMODULE addon,HMODULE reshade_module) {
    wchar_t process[32768];GetModuleFileNameW(nullptr,process,32768);const auto name=std::filesystem::path(process).filename().wstring();
    if(_wcsicmp(name.c_str(),L"sinking_star.exe")!=0&&_wcsicmp(name.c_str(),L"SketchDX12Test.exe")!=0)return false;
    module_handle=addon;
    if(!reshade::register_addon(addon,reshade_module))return false;
    native_ground_bridge::start();
    reshade::register_event<reshade::addon_event::init_pipeline_layout>(free_camera_bridge::layout_init);
    reshade::register_event<reshade::addon_event::destroy_pipeline_layout>(free_camera_bridge::layout_destroy);
    reshade::register_event<reshade::addon_event::update_descriptor_tables>(free_camera_bridge::updated);
    reshade::register_event<reshade::addon_event::copy_descriptor_tables>(free_camera_bridge::copied);
    reshade::register_event<reshade::addon_event::bind_descriptor_tables>(free_camera_bridge::bound);
    reshade::register_event<reshade::addon_event::push_descriptors>(free_camera_bridge::pushed);
    reshade::register_event<reshade::addon_event::reset_command_list>(free_camera_bridge::reset);
    reshade::register_event<reshade::addon_event::destroy_command_list>(free_camera_bridge::reset);
    reshade::register_event<reshade::addon_event::destroy_resource>(free_camera_bridge::resource_destroy);
    reshade::register_event<reshade::addon_event::execute_command_list>(free_camera_bridge::execute);
    reshade::register_event<reshade::addon_event::copy_resource>(free_camera_bridge::resource_copy);
    reshade::register_event<reshade::addon_event::copy_buffer_region>(free_camera_bridge::buffer_copy);
    reshade::register_event<reshade::addon_event::reshade_overlay>(overlay);
    reshade::register_event<reshade::addon_event::reshade_open_overlay>(opened);
    reshade::register_event<reshade::addon_event::destroy_effect_runtime>(destroyed);
    log("SinkingStarSketchPad 3.4.6 loaded (E: draw, G: cursor, B: visibility; T: experimental camera).");return true;
}
extern "C" __declspec(dllexport) void AddonUninit(HMODULE addon,HMODULE) {
    {std::lock_guard<std::recursive_mutex> lock(state_mutex);settle_drawing();if(text_open)finish_text();save();save_camera_preferences();save_drawing_preferences();save_ui_preferences();}
    reshade::unregister_event<reshade::addon_event::init_pipeline_layout>(free_camera_bridge::layout_init);
    reshade::unregister_event<reshade::addon_event::destroy_pipeline_layout>(free_camera_bridge::layout_destroy);
    reshade::unregister_event<reshade::addon_event::update_descriptor_tables>(free_camera_bridge::updated);
    reshade::unregister_event<reshade::addon_event::copy_descriptor_tables>(free_camera_bridge::copied);
    reshade::unregister_event<reshade::addon_event::bind_descriptor_tables>(free_camera_bridge::bound);
    reshade::unregister_event<reshade::addon_event::push_descriptors>(free_camera_bridge::pushed);
    reshade::unregister_event<reshade::addon_event::reset_command_list>(free_camera_bridge::reset);
    reshade::unregister_event<reshade::addon_event::destroy_command_list>(free_camera_bridge::reset);
    reshade::unregister_event<reshade::addon_event::destroy_resource>(free_camera_bridge::resource_destroy);
    reshade::unregister_event<reshade::addon_event::execute_command_list>(free_camera_bridge::execute);
    reshade::unregister_event<reshade::addon_event::reshade_overlay>(overlay);
    reshade::unregister_event<reshade::addon_event::reshade_open_overlay>(opened);
    reshade::unregister_event<reshade::addon_event::destroy_effect_runtime>(destroyed);
    reshade::unregister_event<reshade::addon_event::copy_resource>(free_camera_bridge::resource_copy);
    reshade::unregister_event<reshade::addon_event::copy_buffer_region>(free_camera_bridge::buffer_copy);
    native_ground_bridge::cleanup();
    free_camera_bridge::cleanup();
    reshade::unregister_addon(addon);
}
