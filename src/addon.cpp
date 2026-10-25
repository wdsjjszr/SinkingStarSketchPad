#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <imgui.h>
#include <reshade.hpp>
#include "imgui_abi.hpp"
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
    free_camera_bridge::preferences(GetPrivateProfileIntW(L"Camera",L"SpeedMilli",3000,path.c_str())/1000.,
        GetPrivateProfileIntW(L"Camera",L"SensitivityMilli",80,path.c_str())/1000.,
        GetPrivateProfileIntW(L"Camera",L"InvertX",0,path.c_str())!=0,GetPrivateProfileIntW(L"Camera",L"InvertY",0,path.c_str())!=0);
}
bool loaded=false, editing=false, visible=true, dirty=false, disabled=false, reshade_menu=false;
bool dragging=false, erasing=false, text_open=false, text_focus=false, protected_file=false;
bool settings_open=false;
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
    style.borders=read(L"HintBorders",1)!=0;style.checker=read(L"HintChecker",1)!=0;style.opacity=std::clamp(read(L"HintOpacity",45),0,100);
    int mode=read(L"HintMode",0);hint_preferences.mode=mode>=0&&mode<=2?grid::HintMode(mode):grid::HintMode::On;
    hint_preferences.duration_ms=unsigned(std::clamp(read(L"HintDurationMs",750),0,5000));
    const wchar_t *keys[]={L"Wheel",L"CtrlWheel",L"ShiftWheel",L"AltWheel"};
    for(unsigned i=0;i<4;++i){int value=read(keys[i],int(input::default_wheel[i]));wheel_actions[i]=value>=0&&value<=2?input::WheelAction(value):input::default_wheel[i];}
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
      << "\ncamera_angles=" << free.camera.yaw << ',' << free.camera.pitch << "\ncamera_zoom=" << free.camera.zoom << '\n';
    f << "version=3.4.5\nprocess=" << GetCurrentProcessId() << "\nrenderer=DirectX12\nediting=" << editing
      << "\npage=" << doc.page+1 << "\nstrokes=" << page().size() << "\ncursor=" << doc.cursor
      << "\nlanguage=" << i18n::language_code(language) << "\nui_language=" << (chinese()?"zh-CN":"en") << "\nvisible=" << visible << "\nforeground=" << input_focus.active << "\nframes=" << frame_count << "\ncamera=" << camera.valid << "\ncamera_frame=" << camera.frame << "\ngrid=" << doc.grids[doc.page].ready << "\ngrid_centers=" << (native_ground_snapshot?native_ground_snapshot->cells.size():0) << "\nfree_camera=" << free.camera.enabled << "\nfree_camera_ready=" << free.ready << "\nfree_camera_patches=" << free.patches << "\n";
    f << "grid_drawing=" << grid_drawing << "\ngrid_preview=" << grid_preview.enabled << "\ngrid_preview_centers=" << grid_preview.centers
      << "\ngrid_preview_intersections=" << grid_preview.intersections << "\ngrid_preview_borders=" << grid_preview.borders
      << "\ngrid_preview_checker=" << grid_preview.checker << "\ngrid_preview_opacity=" << grid_preview.opacity
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
    std::filesystem::create_directories(dir); data_path=dir/L"drafts.txt";load_language();load_camera_preferences();load_drawing_preferences();
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
void set_edit(bool value) { settle_drawing(); if(text_open) finish_text(); editing=value;if(value)visible=true;else{cancel_ground_marking();finish_path();}status();if(!value)save(); }
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
void draw_grid_preview(ImDrawList *draw,Point mouse,Point size,bool shift){
    if((selected!=Tool::Path&&!grid_drawing)||!camera.valid)return;
    const auto &g=doc.grids[doc.page];double i,j;if(!g.coordinates(camera,mouse.x/size.x,mouse.y/size.y,i,j))return;
    if(grid_preview.enabled)draw_preview_mesh(draw,preview_mesh(grid_preview,g,grid::CellMask(int(std::round(i))-5,int(std::round(j))-5,11,11),size,1,true,i,j));
    if(erasing)return;
    Point target;auto lattice=g;unsigned color=selected_color;
    if(selected==Tool::Path)target={float(std::round(i)),float(std::round(j))};
    else if(snaps_to_grid(selected)){
        if(dragging){if(!drawing_target(pending,mouse,size,shift,target))return;lattice=pending.lattice;color=pending.color;}
        else target=nearest_grid_point({float(i),float(j)});
    }else return;
    Point pixel;if(screen_point(lattice,target,camera,size,pixel))draw->AddCircle(v(pixel),9,colors[color],24,2.5f);
}
bool ground_ready(){
    return ground_scene_ready&&native_ground_snapshot&&native_ground_snapshot->ready&&native_ground_snapshot->scene_id==ground_scene;
}
void draw_plane_hint(ImDrawList *draw,Point size){
    if(!camera.valid||!hint_preferences.style.visible()||!plane_hint.active(GetTickCount64(),doc.page,ground_generation,hint_preferences))return;
    const auto &g=plane_hint.lattice;double i,j;if(!grid::preview_focus(camera,g,i,j))return;
    int x=int(std::floor(i+.5)),y=int(std::floor(j+.5));
    plane_visibility.update(camera,g,ground_ready()?native_ground_snapshot:nullptr,x-64,y-64,size.x,size.y,hint_preferences.style);
    draw_preview_mesh(draw,plane_visibility.mesh,hint_preferences.style.opacity*.01*grid::fade_weight(double(GetTickCount64()-plane_hint.started),hint_preferences.duration_ms));
}
bool preserve_z_shape(){return dragging&&pending.lattice.ready&&(pending.tool==Tool::Line||pending.tool==Tool::Arrow)&&shape_gesture.waiting();}
void set_ground(double z,grid::HintSource source=grid::HintSource::Mark){
    auto &g=doc.grids[doc.page];
    bool different=!g.ready||g.origin.z!=z||g.origin.x!=0||g.origin.y!=0||g.u.x!=1||g.v.y!=-1;
    if(different){if(source!=grid::HintSource::Wheel||!preserve_z_shape())settle_drawing();g=native_ground::lattice(z);if(source!=grid::HintSource::Wheel)finish_path();}
    ground_user_set[doc.page]=true;plane_hint.show(g,GetTickCount64(),doc.page,ground_generation,source,hint_preferences);
    if(different)changed();else status();
}
void request_ground(GroundAction action,Point mouse={},Point size={}){
    settle_drawing();finish_path();cancel_ground_marking();plane_hint.clear();
    if(action==GroundAction::Manual){select_tool(Tool::Path);ground_marking=true;}
    if(!ground_scene_ready){note(tr(Label::GroundLoading));status();return;}
    if(action==GroundAction::Quick&&!camera.valid){note(tr(Label::NoCamera));status();return;}
    auto kind=action==GroundAction::Character?native_ground_bridge::Read::Character:native_ground_bridge::Read::Ground;
    ground_request.tag={native_ground_bridge::request(kind,ground_generation),ground_generation,doc.page};
    ground_request.action=action;
    if(action==GroundAction::Quick){ground_request.camera=camera;ground_request.pointer={mouse.x/size.x,mouse.y/size.y};}
    status();
}
void update_ground_pick(Point mouse,Point size){
    ground_hit=!ground_request.tag.id&&ground_ready()?native_ground::pick(*native_ground_snapshot,camera,mouse.x/size.x,mouse.y/size.y):native_ground::Pick{};
}
void anchor_ground(bool quick=false){
    if(ground_hit.result==native_ground::PickResult::Ground){set_ground(ground_hit.center.z);note(tr(Label::GroundAnchored),2500);}
    else note(tr(!camera.valid?Label::NoCamera:!ground_ready()?Label::GroundLoading:
        ground_hit.result==native_ground::PickResult::Wall?Label::GroundWall:Label::GroundNoHit));
    ground_marking=native_ground::keep_marking(quick,ground_hit.result);status();
}
void poll_ground(){
    auto state=native_ground_bridge::snapshot();ground_character=state.character;ground_scene_ready=state.scene.ready;
    if(state.generation!=ground_generation){
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
        anchor_ground(true);
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
bool in_rect(Point p,ImVec2 a,ImVec2 b){return p.x>=a.x&&p.y>=a.y&&p.x<=b.x&&p.y<=b.y;}
void drawing_settings(Point size,Point mouse,bool &over_ui){
    if(!settings_open)return;
    ImGui::SetNextWindowPos(ImVec2(size.x*.5f,size.y*.5f),ImGuiCond_FirstUseEver,ImVec2(.5f,.5f));
    ImGui::SetNextWindowSize(ImVec2(std::min(480.f,size.x-40),std::min(580.f,size.y-40)),ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSizeConstraints(ImVec2(320,200),ImVec2(size.x-40,size.y-40));
    if(ImGui::Begin(tr(Label::SettingsTitle),&settings_open,ImGuiWindowFlags_NoSavedSettings|ImGuiWindowFlags_NoCollapse)){
        ImGui::TextWrapped("%s",tr(Label::SettingsHelp));ImGui::SeparatorText(tr(Label::HintSettings));
        const char *modes[]={tr(Label::HintOn),tr(Label::HintOff),tr(Label::HintWheelOnly)};int mode=int(hint_preferences.mode);
        bool changed=ImGui::Combo(tr(Label::HintMode),&mode,modes,3);hint_preferences.mode=grid::HintMode(mode);
        auto &style=hint_preferences.style;
        changed|=ImGui::Checkbox(tr(Label::GridCenters),&style.centers);ImGui::SameLine();changed|=ImGui::Checkbox(tr(Label::GridIntersections),&style.intersections);
        changed|=ImGui::Checkbox(tr(Label::GridBorders),&style.borders);ImGui::SameLine();changed|=ImGui::Checkbox(tr(Label::GridChecker),&style.checker);
        changed|=ImGui::SliderInt(tr(Label::GridOpacity),&style.opacity,0,100,"%d%%",ImGuiSliderFlags_AlwaysClamp);
        float seconds=hint_preferences.duration_ms/1000.f;
        if(ImGui::SliderFloat(tr(Label::HintDuration),&seconds,0,5,"%.2f s",ImGuiSliderFlags_AlwaysClamp)){hint_preferences.duration_ms=unsigned(std::lround(seconds*1000));changed=true;}
        ImGui::TextWrapped("%s",tr(Label::HintHelp));ImGui::SeparatorText(tr(Label::WheelSettings));
        const char *actions[]={tr(Label::WheelNone),tr(Label::WheelPlane),tr(Label::WheelColor)};
        const Label bindings[]={Label::Wheel,Label::CtrlWheel,Label::ShiftWheel,Label::AltWheel};
        for(unsigned i=0;i<4;++i){int action=int(wheel_actions[i]);if(ImGui::Combo(tr(bindings[i]),&action,actions,3)){wheel_actions[i]=input::WheelAction(action);drawing_wheel.reset();changed=true;}}
        ImGui::TextWrapped("%s",tr(Label::WheelHelp));
        if(changed){plane_hint.expire(GetTickCount64(),doc.page,ground_generation,hint_preferences);drawing_preferences_changed();status();}
    }
    auto pos=gui_vec2(imgui_function_table_instance()->GetWindowPos),extent=gui_vec2(imgui_function_table_instance()->GetWindowSize);
    over_ui|=in_rect(mouse,pos,ImVec2(pos.x+extent.x,pos.y+extent.y));ImGui::End();
}
void controls(effect_runtime*rt,Point size,Point mouse,bool &over_ui) {
    static ImVec2 toolbar_position;static float toolbar_overflow=0;
    if(toolbar_overflow>0)ImGui::SetNextWindowPos(ImVec2(toolbar_position.x,std::max(20.f,toolbar_position.y-toolbar_overflow)),ImGuiCond_Always);
    else ImGui::SetNextWindowPos(ImVec2(24,std::max(20.f,size.y-365)),ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSizeConstraints(ImVec2(0,0),ImVec2(size.x-40,size.y-40));
    ImGui::SetNextWindowBgAlpha(.96f);
    ImGui::Begin(tr(Label::Title),nullptr,ImGuiWindowFlags_AlwaysAutoResize|ImGuiWindowFlags_NoCollapse|ImGuiWindowFlags_NoSavedSettings|ImGuiWindowFlags_NoNavInputs);
    ImGui::SetNextItemWidth(195);int choice=static_cast<int>(language);
    const char *languages[]={"System / 系统","English","简体中文 / Chinese"};
    if(ImGui::Combo("Language / 语言###SketchLanguage",&choice,languages,3))set_language(static_cast<i18n::Language>(choice));
    ImGui::SameLine();if(ImGui::Button(tr(Label::Settings)))settings_open=!settings_open;
    const char *labels[]={tr(Label::Pen),tr(Label::Line),tr(Label::Arrow),tr(Label::Rectangle),tr(Label::Ellipse),tr(Label::Text),tr(Label::Eraser),tr(Label::Path)};
    for(unsigned i=0;i<8;++i){if(i%4)ImGui::SameLine();const bool active=selected==Tool(i);if(active)ImGui::PushStyleColor(ImGuiCol_Button,ImVec4(.46f,.36f,.18f,1));if(ImGui::Button(labels[i])){commit();select_tool(Tool(i));}if(active)ImGui::PopStyleColor();}
    for(unsigned i=0;i<5;++i){if(i)ImGui::SameLine();ImGui::PushID(int(i));auto c=ImGui::ColorConvertU32ToFloat4(colors[i]);if(ImGui::ColorButton(tr(Label::Color),c,0,ImVec2(28,25)))selected_color=i;ImGui::PopID();}
    ImGui::SameLine();ImGui::SetNextItemWidth(145);ImGui::SliderFloat(tr(Label::Width),&width,1,16,"%.0f");
    ImGui::SameLine();if(ImGui::Button(tr(Label::Undo)))undo();ImGui::SameLine();if(ImGui::Button(tr(Label::Redo)))redo();ImGui::SameLine();if(ImGui::Button(tr(Label::Clear)))clear();
    for(unsigned i=0;i<3;++i){if(i)ImGui::SameLine();std::string label=tr(Label::Page)+std::to_string(i+1);if(ImGui::RadioButton(label.c_str(),doc.page==i)){settle_drawing();finish_path();cancel_ground_marking();plane_hint.clear();doc.page=i;changed();}}
    ImGui::SameLine();if(ImGui::Button(tr(Label::Save))){commit();if(save())note(tr(Label::Saved));}
    ImGui::SameLine();if(ImGui::Button(tr(Label::Export))){commit();export_svg(size);}
    ImGui::SameLine();if(ImGui::Button(tr(Label::Return)))set_edit(false);
    if(ImGui::Button(tr(Label::CameraToggle))){auto c=free_camera_bridge::snapshot();if(!free_camera_bridge::enable(!c.camera.enabled,camera_plane()))note(tr(Label::CameraUnavailable));}
    ImGui::TextUnformatted(tr(Label::Help));
    ImGui::TextUnformatted(tr(Label::KeyboardHelp));
    if(grid_drawing)ImGui::PushStyleColor(ImGuiCol_Button,ImVec4(.12f,.42f,.5f,1));
    const bool toggle_grid=ImGui::Button(tr(Label::GridDrawing));if(grid_drawing)ImGui::PopStyleColor();
    if(toggle_grid){settle_drawing();if(text_open)finish_text();cancel_ground_marking();finish_path();grid_drawing=!grid_drawing;drawing_preferences_changed();status();}
    ImGui::Text(tr(Label::Stats),doc.page+1,page().size(),dirty?tr(Label::Saving):tr(Label::Saved));
    if(selected==Tool::Path){
        if(ImGui::Button(tr(Label::NewPath)))finish_path();
        if(doc.grids[doc.page].ready)ImGui::TextUnformatted(tr(Label::PathHelp));
        else ImGui::TextUnformatted(tr(Label::GridWait));
        ImGui::TextDisabled("%s",camera.valid?tr(Label::CameraReady):tr(Label::CameraWait));
    }else ImGui::TextDisabled("%s",tr(Label::Anchoring));
    ImGui::Separator();
    bool preview_changed=ImGui::Checkbox(tr(Label::GridPreview),&grid_preview.enabled);
    if(grid_preview.enabled){
        preview_changed|=ImGui::Checkbox(tr(Label::GridCenters),&grid_preview.centers);ImGui::SameLine();preview_changed|=ImGui::Checkbox(tr(Label::GridIntersections),&grid_preview.intersections);
        preview_changed|=ImGui::Checkbox(tr(Label::GridBorders),&grid_preview.borders);ImGui::SameLine();preview_changed|=ImGui::Checkbox(tr(Label::GridChecker),&grid_preview.checker);
        preview_changed|=ImGui::SliderInt(tr(Label::GridOpacity),&grid_preview.opacity,0,100,"%d%%");
    }
    if(preview_changed){drawing_preferences_changed();status();}
    if(ImGui::Button(tr(Label::GroundCharacter))){commit();if(text_open)finish_text();request_ground(GroundAction::Character);}
    ImGui::SameLine();
    if(ImGui::Button(tr(ground_marking?Label::GroundFinish:Label::GroundMark))){
        commit();if(text_open)finish_text();
        if(ground_marking){cancel_ground_marking();status();}else request_ground(GroundAction::Manual);
    }
    if(ground_marking)ImGui::TextWrapped("%s",tr(Label::GroundHelp));
    if(ground_request.tag.id)ImGui::TextWrapped("%s",tr(Label::GroundLoading));
    if(doc.grids[doc.page].ready)ImGui::Text(tr(Label::GroundLocked),doc.grids[doc.page].origin.z);
    if(!ground_ready())ImGui::TextWrapped("%s",tr(Label::GroundLoading));
    ImGui::TextWrapped("%s",tr(Label::GroundShortcut));
    ImGui::TextWrapped("%s",tr(Label::GroundLimits));
    if(ImGui::Checkbox(tr(Label::NativeDebug),&native_ground_debug)){
        native_ground_height=std::numeric_limits<double>::quiet_NaN();status();
    }
    if(native_ground_debug){
        if(!native_ground_snapshot)ImGui::TextWrapped("%s",tr(Label::NativeLoading));
        else if(!native_ground_snapshot->ready)ImGui::TextWrapped(tr(Label::NativeError),native_ground_snapshot->error.c_str());
        else{
            const auto &native=*native_ground_snapshot;
            ImGui::Text(tr(Label::NativeStats),native.scene.c_str(),native.cells.size(),native_ground_visible,native.heights.size());
            ImGui::Text(tr(Label::NativeSources),native.wall_sets,native.voxel_count,native.fences,native.oceans);
            char height[64];std::snprintf(height,sizeof(height),"Z %.4f",native_ground_height);
            if(ImGui::BeginCombo(tr(Label::NativeHeight),std::isnan(native_ground_height)?tr(Label::NativeAll):height)){
                if(ImGui::Selectable(tr(Label::NativeAll),std::isnan(native_ground_height)))native_ground_height=std::numeric_limits<double>::quiet_NaN();
                for(double z:native.heights){std::snprintf(height,sizeof(height),"Z %.4f",z);if(ImGui::Selectable(height,native_ground_height==z))native_ground_height=z;}
                ImGui::EndCombo();
            }
        }
        ImGui::TextWrapped("%s",tr(Label::NativeLegend));
        ImGui::TextWrapped("%s",tr(Label::NativeLimits));
    }
    auto pos=gui_vec2(imgui_function_table_instance()->GetWindowPos),extent=gui_vec2(imgui_function_table_instance()->GetWindowSize);
    toolbar_position=pos;toolbar_overflow=pos.y+extent.y-size.y+20;
    over_ui=in_rect(mouse,pos,ImVec2(pos.x+extent.x,pos.y+extent.y));
    ImGui::End();
    if(text_open) {
        ImGui::SetNextWindowPos(ImVec2(std::clamp(mouse.x,20.f,std::max(20.f,size.x-450)),std::clamp(mouse.y,20.f,std::max(20.f,size.y-250))),ImGuiCond_Appearing);
        ImGui::Begin(tr(Label::TextTitle),nullptr,ImGuiWindowFlags_AlwaysAutoResize|ImGuiWindowFlags_NoSavedSettings);
        if(text_focus){ImGui::SetKeyboardFocusHere();text_focus=false;}
        ImGui::InputTextMultiline("##text",text_buffer,sizeof(text_buffer),ImVec2(400,120));
        if(ImGui::Button(tr(Label::Done)))finish_text();ImGui::SameLine();if(ImGui::Button(tr(Label::Cancel))){text_open=false;text_buffer[0]=0;}
        pos=gui_vec2(imgui_function_table_instance()->GetWindowPos);extent=gui_vec2(imgui_function_table_instance()->GetWindowSize);over_ui|=in_rect(mouse,pos,ImVec2(pos.x+extent.x,pos.y+extent.y));ImGui::End();
    }
}
void keys(effect_runtime*rt) {
    if(!text_open&&ImGui::GetIO().WantTextInput)return;
    const bool ctrl=rt->is_key_down(VK_CONTROL),shift=rt->is_key_down(VK_SHIFT),alt=rt->is_key_down(VK_MENU);
    if((ground_marking||ground_request.tag.id)&&rt->is_key_pressed(VK_ESCAPE)){cancel_ground_marking();status();return;}
    if(shape_gesture.active()&&rt->is_key_pressed(VK_ESCAPE)){cancel();status();return;}
    if(!text_open&&rt->is_key_pressed(VK_ESCAPE)&&free_camera_bridge::snapshot().camera.enabled){free_camera_bridge::enable(false,camera_plane());status();return;}
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
        if(!plane.ready||!plane_point(plane,camera,mouse,size,target))return false;
        target=grid_shape_end(stroke.tool,stroke.points.front(),target,shift);target.z=float(plane.origin.z-stroke.lattice.origin.z);
        return std::abs(target.x)<=1000000&&std::abs(target.y)<=1000000;
    }
    if(shift)mouse=snapped(pixel(stroke.points.front(),size),mouse,stroke.tool==Tool::Rectangle||stroke.tool==Tool::Ellipse);
    target=normal(mouse,size);return true;
}
void canvas(effect_runtime*rt,Point size,Point mouse,bool over_ui) {
    if(suppress_right||erase_with_right)return;
    if(!text_open&&!over_ui&&!ImGui::IsAnyItemActive()&&mouse.x>=0&&mouse.y>=0&&mouse.x<size.x&&mouse.y<size.y
       &&ImGui::IsMouseClicked(ImGuiMouseButton_Middle)){
        commit();
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
    if(action==input::WheelAction::Color){selected_color=input::cycle_color(selected_color,steps,std::size(colors));status();return;}
    if(!preserve_z_shape())settle_drawing();cancel_ground_marking();plane_hint.clear();set_ground(camera_plane()+steps,grid::HintSource::Wheel);
}
void camera_controls(Point size,Point mouse,bool foreground,bool &over_ui){
    auto state=free_camera_bridge::snapshot();if(!state.camera.enabled||!foreground)return;auto c=state.camera;
    ImGui::SetNextWindowPos(ImVec2(std::max(20.f,size.x-485),80),ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(460,0),ImGuiCond_FirstUseEver);
    ImGui::Begin(tr(Label::CameraTitle),nullptr,ImGuiWindowFlags_AlwaysAutoResize|ImGuiWindowFlags_NoSavedSettings|ImGuiWindowFlags_NoNavInputs);
    ImGui::PushTextWrapPos(ImGui::GetFontSize()*28);
    ImGui::TextWrapped("%s",tr(Label::CameraHelp));
    auto slider=[&](Label label,double &value,float low,float high){float x=float(value);bool changed=ImGui::SliderFloat(tr(label),&x,low,high,"%.2f");if(changed)value=x;return changed;};
    bool preferences_changed=slider(Label::CameraSpeed,c.speed,.1f,30);
    preferences_changed|=slider(Label::CameraSensitivity,c.sensitivity,.01f,.3f);
    preferences_changed|=ImGui::Checkbox(tr(Label::CameraInvertX),&c.invert_x);
    preferences_changed|=ImGui::Checkbox(tr(Label::CameraInvertY),&c.invert_y);
    ImGui::TextWrapped("%s",tr(Label::CameraPrefsHelp));
    slider(Label::CameraX,c.offset.x,-1000,1000);slider(Label::CameraY,c.offset.y,-1000,1000);slider(Label::CameraZ,c.offset.z,-1000,1000);
    const auto limits=c.pitch_limits();
    slider(Label::CameraYaw,c.yaw,-180,180);slider(Label::CameraPitch,c.pitch,float(limits[0]),float(limits[1]));slider(Label::CameraZoom,c.zoom,.05f,20);c.limit_angles();
    bool reset=ImGui::Button(tr(Label::CameraReset));bool off=ImGui::Button(tr(Label::CameraOff));
    ImGui::TextUnformatted(state.last_patch&&GetTickCount64()-state.last_patch<1500?tr(Label::CameraLive):tr(Label::CameraWaiting));
    ImGui::TextWrapped("%s",tr(Label::CameraLimits));ImGui::PopTextWrapPos();
    auto pos=gui_vec2(imgui_function_table_instance()->GetWindowPos),extent=gui_vec2(imgui_function_table_instance()->GetWindowSize);
    over_ui|=in_rect(mouse,pos,ImVec2(pos.x+extent.x,pos.y+extent.y));ImGui::End();
    if(preferences_changed){free_camera_bridge::preferences(c.speed,c.sensitivity,c.invert_x,c.invert_y);camera_preferences_dirty=true;camera_preferences_due=GetTickCount64()+500;}
    if(off){free_camera_bridge::enable(false,camera_plane());status();return;}
    if(reset){free_camera_bridge::enable(true,camera_plane());return;}
    free_camera_bridge::change(c);
}
void camera_navigation(effect_runtime*rt,bool foreground,bool over_ui){
    auto state=free_camera_bridge::snapshot();if(!state.camera.enabled||!foreground)return;auto c=state.camera;
    const bool chord=rt->is_key_down(VK_CONTROL)||rt->is_key_down(VK_MENU);
    // Hovering a panel must not disable keyboard navigation. Only an active
    // widget (including Ctrl-click numeric entry) owns these keys.
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
        if(editing&&foreground&&!reshade_menu) {
            controls(rt,size,mouse,over_ui);
            drawing_settings(size,mouse,over_ui);
        }
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
        if(visible&&foreground&&!reshade_menu)draw_plane_hint(draw,size);
        if(visible){for(const auto&s:page())draw_stroke(draw,*s,size);if(dragging)draw_stroke(draw,pending,size);}
        if(editing&&foreground&&!over_ui&&!reshade_menu){if(ground_marking)draw_ground_preview(draw,size);else draw_grid_preview(draw,mouse,size,rt->is_key_down(VK_SHIFT));}
        if(native_ground_debug&&visible&&foreground&&!reshade_menu)draw_native_ground_debug(draw,mouse,size,over_ui);else native_ground_visible=0;
        if(foreground&&!reshade_menu&&(doc.cursor||editing||free_camera_bridge::snapshot().camera.enabled)&&mouse.x>=0&&mouse.y>=0&&mouse.x<size.x&&mouse.y<size.y)cursor(ImGui::GetForegroundDrawList(),mouse);
        if(!editing&&!reshade_menu){ImDrawList*d=ImGui::GetForegroundDrawList();float right=std::min(size.x-18.f,42.f+gui_text_size(tr(Label::Hud)).x);d->AddRectFilled(ImVec2(18,size.y-47),ImVec2(right,size.y-14),IM_COL32(18,27,41,205),7);d->AddText(ImVec2(30,size.y-41),colors[0],tr(Label::Hud));}
        if(GetTickCount64()<toast_until){ImGui::SetNextWindowPos(ImVec2(size.x*.5f,22),ImGuiCond_Always,ImVec2(.5f,0));ImGui::Begin("##SketchToast",nullptr,ImGuiWindowFlags_NoDecoration|ImGuiWindowFlags_AlwaysAutoResize|ImGuiWindowFlags_NoInputs|ImGuiWindowFlags_NoSavedSettings);ImGui::TextUnformatted(toast.c_str());ImGui::End();}
        if(dirty&&(!dragging||shape_gesture.waiting())&&!erasing&&GetTickCount64()>=save_due)save();
        if(camera_preferences_dirty&&GetTickCount64()>=camera_preferences_due)save_camera_preferences();
        if(drawing_preferences_dirty&&GetTickCount64()>=drawing_preferences_due)save_drawing_preferences();
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
    log("SinkingStarSketchPad 3.4.5 loaded (E: draw, G: cursor, B: visibility; T: experimental camera).");return true;
}
extern "C" __declspec(dllexport) void AddonUninit(HMODULE addon,HMODULE) {
    {std::lock_guard<std::recursive_mutex> lock(state_mutex);settle_drawing();if(text_open)finish_text();save();save_camera_preferences();save_drawing_preferences();}
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
