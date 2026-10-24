#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <imgui.h>
#include <reshade.hpp>
#include "imgui_abi.hpp"
#include "model.hpp"
#include "i18n.hpp"
#include "cursor_style.hpp"
#include "camera_bridge.hpp"
#include "grid_bridge.hpp"
#include "native_ground_bridge.hpp"
#include "native_ground_pick.hpp"
#include "input_policy.hpp"
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
std::filesystem::path data_path;
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
Tool selected=Tool::Pen;
unsigned selected_color=0;
float width=3;
Stroke pending;
grid::Camera camera;
std::array<native_ground::TargetState,3> ground_state{};
native_ground::CharacterSample ground_character;
uintptr_t ground_scene=0;
std::string ground_scene_name;
bool ground_marking=false;
bool native_ground_debug=false;
unsigned native_ground_visible=0;
std::shared_ptr<const native_ground::Snapshot> native_ground_snapshot;
double native_ground_height=std::numeric_limits<double>::quiet_NaN();
native_ground::Pick ground_hit;
bool path_active=false, grid_preview=true;
double camera_plane(){const auto &g=doc.grids[doc.page];return g.ready?g.origin.z:0.;}
input::Focus input_focus;
void finish_path(){path_active=false;}
void select_tool(Tool t){ground_marking=false;ground_hit={};selected=t;finish_path();}
Page erase_before;
char text_buffer[2049]={};
Point text_position;
ULONGLONG save_due=0, toast_until=0, frame_count=0;
bool camera_preferences_dirty=false;
ULONGLONG camera_preferences_due=0;
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
void status() {
    if (data_path.empty()) return;
    std::ofstream f(data_path.parent_path()/L"status.txt");
    auto free=free_camera_bridge::snapshot();
    f << "camera_offset=" << free.camera.offset.x << ',' << free.camera.offset.y << ',' << free.camera.offset.z
      << "\ncamera_angles=" << free.camera.yaw << ',' << free.camera.pitch << "\ncamera_zoom=" << free.camera.zoom << '\n';
    f << "version=3.4.5\nprocess=" << GetCurrentProcessId() << "\nrenderer=DirectX12\nediting=" << editing
      << "\npage=" << doc.page+1 << "\nstrokes=" << page().size() << "\ncursor=" << doc.cursor
      << "\nlanguage=" << i18n::language_code(language) << "\nui_language=" << (chinese()?"zh-CN":"en") << "\nvisible=" << visible << "\nforeground=" << input_focus.active << "\nframes=" << frame_count << "\ncamera=" << camera.valid << "\ncamera_frame=" << camera.frame << "\ngrid=" << doc.grids[doc.page].ready << "\ngrid_centers=" << (native_ground_snapshot?native_ground_snapshot->cells.size():0) << "\nfree_camera=" << free.camera.enabled << "\nfree_camera_ready=" << free.ready << "\nfree_camera_patches=" << free.patches << "\n";
    f << "ground_marking=" << ground_marking << "\nground_mode=" << (ground_state[doc.page].manual?"manual":"auto")
      << "\nground_follow=" << ground_state[doc.page].follow << "\nground_locked=" << (doc.grids[doc.page].ready&&!ground_state[doc.page].needs_update())
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
        dirty=false; return true;
    } catch(const std::exception &e) { note(std::string(tr(Label::SaveFailed))+e.what(),15000); log(toast); save_due=GetTickCount64()+15000; return false; }
}
void load() {
    wchar_t path[32768]; DWORD n=GetModuleFileNameW(module_handle,path,32768);
    if(!n || n>=32768) throw std::runtime_error("Cannot locate addon directory");
    auto dir=std::filesystem::path(path).parent_path()/L"SinkingStarSketch-data";
    std::filesystem::create_directories(dir); data_path=dir/L"drafts.txt";load_language();load_camera_preferences();
    auto read=[](const std::filesystem::path &p) {
        if(std::filesystem::file_size(p)>16*1024*1024) throw std::runtime_error("Draft file too large");
        std::ifstream in(p,std::ios::binary); return Document::read(in);
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
    if(dragging) {
        if(page().size()<2000 && doc.point_count()+pending.points.size()<=120000) {
            history[doc.page].record(page()); page().push_back(std::make_shared<const Stroke>(std::move(pending))); changed();
        } else note(tr(Label::Full));
        dragging=false;
    }
    if(erasing) { if(page().size()!=erase_before.size()) { history[doc.page].record(erase_before); changed(); } erasing=false; erase_before.clear(); }
}
void cancel() { dragging=false; if(erasing) {page()=std::move(erase_before); erasing=false;} }
void finish_text() {
    if(text_buffer[0] && page().size()<2000 && doc.point_count()<120000) {
        Stroke s; s.tool=Tool::Text;s.color=selected_color;s.width=width;s.points={text_position};s.text=text_buffer;
        history[doc.page].record(page());page().push_back(std::make_shared<const Stroke>(std::move(s)));changed();
    }
    text_open=false; text_buffer[0]=0;
}
void set_edit(bool value) { commit(); if(text_open) finish_text(); editing=value;if(value)visible=true;else{ground_marking=false;ground_hit={};}status();if(!value)save(); }
void undo() { finish_path(); cancel(); if(history[doc.page].back(page()))changed(); }
void redo() { finish_path(); cancel(); if(history[doc.page].forward(page()))changed(); }
void clear() { finish_path(); commit(); if(!page().empty()){history[doc.page].record(page());page().clear();changed();note(tr(Label::Cleared));} }
ImVec2 v(Point p) {return ImVec2(p.x,p.y);}
void polyline(ImDrawList *draw,const std::vector<Point>&p,ImU32 color,float w) {
    if(p.empty())return;
    if(p.size()==1){draw->AddCircleFilled(v(p[0]),w*.5f,color);return;}
    std::vector<ImVec2> vertices;vertices.reserve(p.size());for(auto a:p)vertices.push_back(v(a));
    draw->AddPolyline(vertices.data(),static_cast<int>(vertices.size()),color,ImDrawFlags_None,w);
}
std::vector<Point> projected(const Stroke &s,Point size){
    if(s.tool!=Tool::Path)return outline(s,size);
    std::vector<Point> result;if(!camera.valid)return result;
    for(const auto &p:s.points){double x,y;if(!camera.project(s.lattice.world(p.x,p.y),x,y))return {};result.push_back({float(x*size.x),float(y*size.y)});}
    return result;
}
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
        auto last=stroke->points.back();int dx=int(target.x-last.x),dy=int(target.y-last.y);
        if(!dx&&!dy)return;
        if(dx&&dy){note(tr(Label::NeedCorner));return;}
        extra=size_t(std::abs(dx)+std::abs(dy));
        if(extra>1000){note(tr(Label::TooFar));return;}
        if(stroke->points.size()+extra>10000||doc.point_count()+extra>120000){note(tr(Label::PathFull));return;}
        for(size_t n=1;n<=extra;++n)stroke->points.push_back({last.x+float((dx>0)-(dx<0))*float(n),last.y+float((dy>0)-(dy<0))*float(n)});
    }else stroke->points.push_back(target);
    if((!append&&page().size()>=2000)||doc.point_count()+extra>120000){note(tr(Label::Full));return;}
    history[doc.page].record(page());if(append)page().back()=stroke;else page().push_back(stroke);
    path_active=true;changed();
}
void draw_grid_preview(ImDrawList *draw,Point mouse,Point size){
    if(selected!=Tool::Path||!camera.valid)return;
    const auto &g=doc.grids[doc.page];double i,j;if(!g.snap(camera,mouse.x/size.x,mouse.y/size.y,i,j))return;
    if(grid_preview)for(int a=-5;a<=5;++a)for(int b=-5;b<=5;++b){double x,y;if(camera.project(g.world(i+a,j+b),x,y))draw->AddCircleFilled(ImVec2(float(x*size.x),float(y*size.y)),2,IM_COL32(109,227,255,115));}
    double x,y;if(camera.project(g.world(i,j),x,y))draw->AddCircle(ImVec2(float(x*size.x),float(y*size.y)),9,colors[selected_color],24,2.5f);
}
bool ground_ready(){
    return ground_scene&&ground_character.scene_id==ground_scene&&native_ground_snapshot&&native_ground_snapshot->ready&&native_ground_snapshot->scene_id==ground_scene;
}
void set_ground(double z){
    auto &g=doc.grids[doc.page];
    if(!g.ready||g.origin.z!=z||g.origin.x!=0||g.origin.y!=0||g.u.x!=1||g.v.y!=-1){
        g=native_ground::lattice(z);finish_path();changed();
    }
    ground_state[doc.page].pending=false;
}
bool update_automatic_ground(){
    if(!ground_ready()||!ground_character.valid||ground_character.scene_id!=ground_scene)return false;
    auto cell=native_ground::nearest(*native_ground_snapshot,ground_character.position);if(!cell)return false;
    set_ground(cell->center.z);return true;
}
void toggle_ground_follow(){
    auto &state=ground_state[doc.page];state.follow=!state.follow;
    if(state.follow&&!update_automatic_ground())note(tr(Label::GroundCharacterWait));
    else note(tr(state.follow?Label::GroundFollowing:Label::GroundFrozen),2500);
    status();
}
void update_ground_pick(Point mouse,Point size){
    ground_hit=ground_ready()?native_ground::pick(*native_ground_snapshot,camera,mouse.x/size.x,mouse.y/size.y):native_ground::Pick{};
}
void anchor_ground(bool quick=false){
    if(ground_hit.result==native_ground::PickResult::Ground){set_ground(ground_hit.center.z);note(tr(Label::GroundAnchored),2500);}
    else note(tr(!camera.valid?Label::NoCamera:!ground_ready()?Label::GroundLoading:
        ground_hit.result==native_ground::PickResult::Wall?Label::GroundWall:Label::GroundNoHit));
    ground_marking=native_ground::keep_marking(quick,ground_hit.result);status();
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
    auto a=pixel(s.points.front(),size),b=pixel(s.points.back(),size);
    ImU32 c=colors[s.color],shadow=IM_COL32(9,14,23,210);
    if(s.tool==Tool::Text) {
        float fs=std::max(20.f,s.width*4+12);
        draw->AddText(ImGui::GetFont(),fs,ImVec2(a.x+1.5f,a.y+1.5f),shadow,s.text.c_str());
        draw->AddText(ImGui::GetFont(),fs,v(a),c,s.text.c_str());return;
    }
    auto p=projected(s,size);polyline(draw,p,shadow,s.width+3);polyline(draw,p,c,s.width);
    if(s.tool==Tool::Path){
        for(size_t i=0;i<p.size();++i){draw->AddCircleFilled(v(p[i]),5,shadow);draw->AddCircleFilled(v(p[i]),3, c);}
        if(!p.empty()){draw->AddCircle(v(p.front()),8,c,24,2);draw->AddCircle(v(p.back()),6,c,24,2);}
    }
    if(s.tool==Tool::Arrow) {
        float length=distance(a,b);if(length<2)return;
        float dx=(b.x-a.x)/length,dy=(b.y-a.y)/length,head=std::min(std::max(13.f,s.width*4),length*.6f);
        std::vector<Point> tips={{b.x-dx*head-dy*head*.5f,b.y-dy*head+dx*head*.5f},b,{b.x-dx*head+dy*head*.5f,b.y-dy*head-dx*head*.5f}};
        polyline(draw,tips,shadow,s.width+3);polyline(draw,tips,c,s.width);
    }
}
void erase_at(Point p,Point size) {
    auto &marks=page();
    marks.erase(std::remove_if(marks.begin(),marks.end(),[&](const auto&s){
        if(s->tool==Tool::Text) {
            Point a=pixel(s->points[0],size);ImVec2 measure=gui_text_size(s->text.c_str());float scale=std::max(20.f,s->width*4+12)/ImGui::GetFontSize();
            return p.x>=a.x-8&&p.y>=a.y-8&&p.x<=a.x+measure.x*scale+8&&p.y<=a.y+measure.y*scale+8;
        }
        if(s->tool==Tool::Path){auto pts=projected(*s,size);if(pts.size()==1)return distance(pts[0],p)<=s->width*.5f+9;
            for(size_t n=1;n<pts.size();++n)if(segment_distance(p,pts[n-1],pts[n])<=s->width*.5f+9)return true;return false;}
        return hit(*s,p,size);
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
                if(s->tool==Tool::Arrow&&p.size()==2){Point a=p[0],b=p[1];float len=distance(a,b);if(len>2){float dx=(b.x-a.x)/len,dy=(b.y-a.y)/len,head=std::min(std::max(13.f,s->width*4),len*.6f);out<<"<polyline fill=\"none\" stroke=\""<<c<<"\" stroke-width=\""<<s->width<<"\" points=\""<<b.x-dx*head-dy*head*.5f<<','<<b.y-dy*head+dx*head*.5f<<' '<<b.x<<','<<b.y<<' '<<b.x-dx*head+dy*head*.5f<<','<<b.y-dy*head-dx*head*.5f<<"\"/>\n";}}
            }
        }
        out<<"</svg>\n";out.flush();if(!out)throw std::runtime_error("Export failed");note(tr(Label::Exported));
    }catch(const std::exception&e){note(std::string(tr(Label::ExportFailed))+e.what());}
}
bool in_rect(Point p,ImVec2 a,ImVec2 b){return p.x>=a.x&&p.y>=a.y&&p.x<=b.x&&p.y<=b.y;}
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
    const char *labels[]={tr(Label::Pen),tr(Label::Line),tr(Label::Arrow),tr(Label::Rectangle),tr(Label::Ellipse),tr(Label::Text),tr(Label::Eraser),tr(Label::Path)};
    for(unsigned i=0;i<8;++i){if(i%4)ImGui::SameLine();const bool active=selected==Tool(i);if(active)ImGui::PushStyleColor(ImGuiCol_Button,ImVec4(.46f,.36f,.18f,1));if(ImGui::Button(labels[i])){commit();select_tool(Tool(i));}if(active)ImGui::PopStyleColor();}
    for(unsigned i=0;i<5;++i){if(i)ImGui::SameLine();ImGui::PushID(int(i));auto c=ImGui::ColorConvertU32ToFloat4(colors[i]);if(ImGui::ColorButton(tr(Label::Color),c,0,ImVec2(28,25)))selected_color=i;ImGui::PopID();}
    ImGui::SameLine();ImGui::SetNextItemWidth(145);ImGui::SliderFloat(tr(Label::Width),&width,1,16,"%.0f");
    ImGui::SameLine();if(ImGui::Button(tr(Label::Undo)))undo();ImGui::SameLine();if(ImGui::Button(tr(Label::Redo)))redo();ImGui::SameLine();if(ImGui::Button(tr(Label::Clear)))clear();
    for(unsigned i=0;i<3;++i){if(i)ImGui::SameLine();std::string label=tr(Label::Page)+std::to_string(i+1);if(ImGui::RadioButton(label.c_str(),doc.page==i)){commit();finish_path();ground_marking=false;ground_hit={};doc.page=i;changed();}}
    ImGui::SameLine();if(ImGui::Button(tr(Label::Save))){commit();if(save())note(tr(Label::Saved));}
    ImGui::SameLine();if(ImGui::Button(tr(Label::Export))){commit();export_svg(size);}
    ImGui::SameLine();if(ImGui::Button(tr(Label::Return)))set_edit(false);
    if(ImGui::Button(tr(Label::CameraToggle))){auto c=free_camera_bridge::snapshot();if(!free_camera_bridge::enable(!c.camera.enabled,camera_plane()))note(tr(Label::CameraUnavailable));}
    ImGui::TextUnformatted(tr(Label::Help));
    ImGui::TextUnformatted(tr(Label::KeyboardHelp));
    ImGui::Text(tr(Label::Stats),doc.page+1,page().size(),dirty?tr(Label::Saving):tr(Label::Saved));
    if(selected==Tool::Path){
        if(ImGui::Button(tr(Label::NewPath)))finish_path();
        ImGui::SameLine();ImGui::Checkbox(tr(Label::GridPreview),&grid_preview);
        if(doc.grids[doc.page].ready)ImGui::TextUnformatted(tr(Label::PathHelp));
        else ImGui::TextUnformatted(tr(Label::GridWait));
        ImGui::TextDisabled("%s",camera.valid?tr(Label::CameraReady):tr(Label::CameraWait));
    }else ImGui::TextDisabled("%s",tr(Label::Anchoring));
    ImGui::Separator();
    auto &ground=ground_state[doc.page];
    if(ImGui::RadioButton(tr(Label::GroundAuto),!ground.manual)){
        commit();ground.mode(false);ground_marking=false;ground_hit={};finish_path();update_automatic_ground();status();
    }
    ImGui::SameLine();
    if(ImGui::RadioButton(tr(Label::GroundManual),ground.manual)){
        commit();ground.mode(true);ground_marking=false;ground_hit={};finish_path();status();
    }
    if(!ground.manual){
        bool follow=ground.follow;if(ImGui::Checkbox(tr(Label::GroundFollow),&follow))toggle_ground_follow();
        if(!ground_character.valid)ImGui::TextWrapped("%s",tr(Label::GroundCharacterWait));
    }else{
        if(ImGui::Button(tr(ground_marking?Label::GroundFinish:Label::GroundMark))){
            commit();if(text_open)finish_text();
            if(ground_marking)ground_marking=false;else{select_tool(Tool::Path);ground_marking=true;}
            ground_hit={};status();
        }
        if(ground_marking)ImGui::TextWrapped("%s",tr(Label::GroundHelp));
    }
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
    const bool ctrl=rt->is_key_down(VK_CONTROL),shift=rt->is_key_down(VK_SHIFT),alt=rt->is_key_down(VK_MENU);
    if(ground_marking&&rt->is_key_pressed(VK_ESCAPE)){ground_marking=false;ground_hit={};status();return;}
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
        for(unsigned i=0;i<3;++i)if(rt->is_key_pressed('1'+i)){commit();finish_path();ground_marking=false;ground_hit={};doc.page=i;changed();}
    }else if(input::tool_shortcuts(ctrl,shift,alt)){
        const int codes[]={'P','L','A','R','O','T','K','H'};
        for(unsigned i=0;i<8;++i)if(rt->is_key_pressed(codes[i])){commit();select_tool(Tool(i));}
        for(unsigned i=0;i<5;++i)if(rt->is_key_pressed('1'+i))selected_color=i;
    }
}
void canvas(effect_runtime*rt,Point size,Point mouse,bool over_ui) {
    if(!text_open&&!over_ui&&!ImGui::IsAnyItemActive()&&mouse.x>=0&&mouse.y>=0&&mouse.x<size.x&&mouse.y<size.y
       &&ImGui::IsMouseClicked(ImGuiMouseButton_Middle)){
        commit();
        if(ground_state[doc.page].manual){update_ground_pick(mouse,size);anchor_ground(true);}else toggle_ground_follow();
        return;
    }
    if(ground_marking){if(!over_ui&&ImGui::IsMouseClicked(ImGuiMouseButton_Left))anchor_ground();return;}
    bool left=ImGui::IsMouseDown(ImGuiMouseButton_Left);
    if(!text_open && !free_camera_bridge::snapshot().camera.enabled && ImGui::IsMouseClicked(ImGuiMouseButton_Right) && !over_ui){undo();return;}
    if(!text_open&&!over_ui&&ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
        if(selected==Tool::Path){path_click(mouse,size);return;}
        if(selected==Tool::Text){text_position=normal(mouse,size);text_buffer[0]=0;text_open=true;text_focus=true;return;}
        if(selected==Tool::Eraser){erase_before=page();erasing=true;}
        else {pending=Stroke();pending.tool=selected;pending.color=selected_color;pending.width=width;pending.points.push_back(normal(mouse,size));if(selected!=Tool::Pen)pending.points.push_back(normal(mouse,size));dragging=true;}
    }
    if(erasing)erase_at(mouse,size);
    if(dragging) {
        Point p=mouse;
        if(pending.tool==Tool::Pen){if(distance(pixel(pending.points.back(),size),p)>=1.4f&&pending.points.size()<10000)pending.points.push_back(normal(p,size));}
        else {if(rt->is_key_down(VK_SHIFT))p=snapped(pixel(pending.points[0],size),p,pending.tool==Tool::Rectangle||pending.tool==Tool::Ellipse);pending.points[1]=normal(p,size);}
    }
    if(!left&&(dragging||erasing))commit();
}
void cursor(ImDrawList*draw,Point p) {
    const auto mode=!editing?cursor_style::Mode::Pointer:(selected==Tool::Eraser?cursor_style::Mode::Eraser:cursor_style::Mode::Drawing);
    float scale=std::clamp(ImGui::GetIO().DisplaySize.y/1440.f,1.f,1.5f);
    cursor_style::draw(draw,v(p),mode,colors[selected_color],scale);
}
void camera_controls(effect_runtime*rt,Point size,Point mouse,bool foreground,bool &over_ui){
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
    const bool chord=rt->is_key_down(VK_CONTROL)||rt->is_key_down(VK_MENU);
    // Hovering a panel must not disable keyboard navigation. Only an active
    // widget (including Ctrl-click numeric entry) owns these keys.
    if(!text_open&&!chord&&!ImGui::IsAnyItemActive()){
        c.move((rt->is_key_down('D')?1:0)-(rt->is_key_down('A')?1:0),(rt->is_key_down('W')?1:0)-(rt->is_key_down('S')?1:0),
               (rt->is_key_down('Q')?1:0)-(rt->is_key_down('Z')?1:0),ImGui::GetIO().DeltaTime,rt->is_key_down(VK_SHIFT));
        if(!over_ui&&input_focus.canvas_ready()){
            if(ImGui::IsMouseDown(ImGuiMouseButton_Right))c.mouse_rotate(ImGui::GetIO().MouseDelta.x,ImGui::GetIO().MouseDelta.y);
            c.magnify(ImGui::GetIO().MouseWheel);
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
        ++frame_count;if(frame_count==30)status();
        ImGuiIO &io=ImGui::GetIO();Point size={io.DisplaySize.x,io.DisplaySize.y};if(size.x<1||size.y<1)return;
        free_camera_bridge::viewport(uint32_t(size.x),uint32_t(size.y));
        camera=free_camera_bridge::projection(uint32_t(size.x),uint32_t(size.y));if(!camera.valid)camera=camera_bridge::snapshot(size.x,size.y);
        ground_character=native_ground_bridge::character_snapshot();
        auto native=native_ground_bridge::snapshot();
        const uintptr_t scene=ground_character.scene_id?ground_character.scene_id:(native?native->scene_id:0);
        if(scene&&(scene!=ground_scene||(native&&native->scene_id==scene&&native->ready&&native->scene!=ground_scene_name))){
            ground_scene=scene;ground_scene_name=native&&native->scene_id==scene&&native->ready?native->scene:"";
            for(size_t n=0;n<ground_state.size();++n){ground_state[n].scene_reset();doc.grids[n]={};}
            ground_marking=false;ground_hit={};finish_path();native_ground_height=std::numeric_limits<double>::quiet_NaN();
        }
        if(native!=native_ground_snapshot){
            if(!native||(!std::isnan(native_ground_height)&&!native->heights.count(native_ground_height)))native_ground_height=std::numeric_limits<double>::quiet_NaN();
            native_ground_snapshot=std::move(native);
        }
        if(ground_state[doc.page].needs_update())update_automatic_ground();
        if(frame_count%120==0)status();
        HWND window=static_cast<HWND>(rt->get_hwnd());bool foreground=GetForegroundWindow()==window;
        // ReShade virtualizes Win32 GetCursorPos while it blocks game input.
        // Its ImGui input has the real client position, already scaled to this back buffer.
        Point mouse={io.MousePos.x,io.MousePos.y};
        const bool mouse_down=ImGui::IsMouseDown(ImGuiMouseButton_Left)||ImGui::IsMouseDown(ImGuiMouseButton_Right)||ImGui::IsMouseDown(ImGuiMouseButton_Middle);
        const bool was_foreground=input_focus.active;
        if(input_focus.update(foreground,mouse_down)&&editing){commit();save();status();}
        if(foreground&&!was_foreground)status();
        // Ignore activation-frame keys (including an Alt+Tab chord), and keep
        // an unfinished text dialog intact while another application has focus.
        if(foreground&&was_foreground&&!reshade_menu)keys(rt);
        bool over_ui=false;
        if(editing&&foreground&&!reshade_menu) {
            controls(rt,size,mouse,over_ui);
        }
        if(!reshade_menu)camera_controls(rt,size,mouse,foreground,over_ui);
        if(ground_marking&&editing&&foreground&&!over_ui&&!reshade_menu&&input_focus.canvas_ready())update_ground_pick(mouse,size);
        else ground_hit={};
        if(editing&&foreground&&!reshade_menu&&input_focus.canvas_ready())canvas(rt,size,mouse,over_ui);
        if(!reshade_menu){
            const bool free=free_camera_bridge::snapshot().camera.enabled;
            const bool capture_mouse=(editing||free)&&foreground;
            const bool capture_keys=capture_mouse&&(free||input::capture_keyboard(text_open,rt->is_key_down(VK_CONTROL),rt->is_key_down(VK_MENU)));
            // InputProcessing=1 lets ReShade honor the two capture flags
            // separately. The block request enables add-on input routing even
            // when ReShade's own Home overlay is closed. Set current AND next
            // frame flags so a previously focused widget cannot eat movement.
            if(capture_mouse)rt->block_input_next_frame();
            io.WantCaptureMouse=capture_mouse;io.WantCaptureKeyboard=capture_keys;
            ImGui::SetNextFrameWantCaptureMouse(capture_mouse);ImGui::SetNextFrameWantCaptureKeyboard(capture_keys);
        }
        ImDrawList *draw=ImGui::GetBackgroundDrawList();
        if(visible){for(const auto&s:page())draw_stroke(draw,*s,size);if(dragging)draw_stroke(draw,pending,size);}
        if(editing&&foreground&&!over_ui&&!reshade_menu){if(ground_marking)draw_ground_preview(draw,size);else draw_grid_preview(draw,mouse,size);}
        if(native_ground_debug&&visible&&foreground&&!reshade_menu)draw_native_ground_debug(draw,mouse,size,over_ui);else native_ground_visible=0;
        if(foreground&&!reshade_menu&&(doc.cursor||editing||free_camera_bridge::snapshot().camera.enabled)&&mouse.x>=0&&mouse.y>=0&&mouse.x<size.x&&mouse.y<size.y)cursor(ImGui::GetForegroundDrawList(),mouse);
        if(!editing&&!reshade_menu){ImDrawList*d=ImGui::GetForegroundDrawList();float right=std::min(size.x-18.f,42.f+gui_text_size(tr(Label::Hud)).x);d->AddRectFilled(ImVec2(18,size.y-47),ImVec2(right,size.y-14),IM_COL32(18,27,41,205),7);d->AddText(ImVec2(30,size.y-41),colors[0],tr(Label::Hud));}
        if(GetTickCount64()<toast_until){ImGui::SetNextWindowPos(ImVec2(size.x*.5f,22),ImGuiCond_Always,ImVec2(.5f,0));ImGui::Begin("##SketchToast",nullptr,ImGuiWindowFlags_NoDecoration|ImGuiWindowFlags_AlwaysAutoResize|ImGuiWindowFlags_NoInputs|ImGuiWindowFlags_NoSavedSettings);ImGui::TextUnformatted(toast.c_str());ImGui::End();}
        if(dirty&&!dragging&&!erasing&&GetTickCount64()>=save_due)save();
        if(camera_preferences_dirty&&GetTickCount64()>=camera_preferences_due)save_camera_preferences();
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
    if(rt==primary){commit();save();primary=nullptr;reshade_menu=false;input_focus={};if(text_open)text_focus=true;}
}
}
extern "C" __declspec(dllexport) const char *NAME="SinkingStarSketchPad";
extern "C" __declspec(dllexport) const char *DESCRIPTION="In-game drawing, scratch pages and cell-snapped paths for Order of the Sinking Star. English and Simplified Chinese.";
extern "C" __declspec(dllexport) bool AddonInit(HMODULE addon,HMODULE reshade_module) {
    wchar_t process[32768];GetModuleFileNameW(nullptr,process,32768);const auto name=std::filesystem::path(process).filename().wstring();
    if(_wcsicmp(name.c_str(),L"sinking_star.exe")!=0&&_wcsicmp(name.c_str(),L"SketchDX12Test.exe")!=0)return false;
    module_handle=addon;
    if(!reshade::register_addon(addon,reshade_module))return false;
    grid_bridge::start();
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
    reshade::register_event<reshade::addon_event::destroy_resource>(grid_bridge::destroyed);
    reshade::register_event<reshade::addon_event::execute_command_list>(free_camera_bridge::execute);
    reshade::register_event<reshade::addon_event::copy_resource>(free_camera_bridge::resource_copy);
    reshade::register_event<reshade::addon_event::copy_buffer_region>(free_camera_bridge::buffer_copy);
    reshade::register_event<reshade::addon_event::copy_buffer_region>(grid_bridge::copied);
    reshade::register_event<reshade::addon_event::update_descriptor_tables>(camera_bridge::updated);
    reshade::register_event<reshade::addon_event::reshade_overlay>(overlay);
    reshade::register_event<reshade::addon_event::reshade_open_overlay>(opened);
    reshade::register_event<reshade::addon_event::destroy_effect_runtime>(destroyed);
    log("SinkingStarSketchPad 3.4.5 loaded (E: draw, G: cursor, B: visibility; T: experimental camera).");return true;
}
extern "C" __declspec(dllexport) void AddonUninit(HMODULE addon,HMODULE) {
    {std::lock_guard<std::recursive_mutex> lock(state_mutex);commit();if(text_open)finish_text();save();save_camera_preferences();}
    reshade::unregister_event<reshade::addon_event::init_pipeline_layout>(free_camera_bridge::layout_init);
    reshade::unregister_event<reshade::addon_event::destroy_pipeline_layout>(free_camera_bridge::layout_destroy);
    reshade::unregister_event<reshade::addon_event::update_descriptor_tables>(free_camera_bridge::updated);
    reshade::unregister_event<reshade::addon_event::copy_descriptor_tables>(free_camera_bridge::copied);
    reshade::unregister_event<reshade::addon_event::bind_descriptor_tables>(free_camera_bridge::bound);
    reshade::unregister_event<reshade::addon_event::push_descriptors>(free_camera_bridge::pushed);
    reshade::unregister_event<reshade::addon_event::reset_command_list>(free_camera_bridge::reset);
    reshade::unregister_event<reshade::addon_event::destroy_command_list>(free_camera_bridge::reset);
    reshade::unregister_event<reshade::addon_event::destroy_resource>(free_camera_bridge::resource_destroy);
    reshade::unregister_event<reshade::addon_event::destroy_resource>(grid_bridge::destroyed);
    reshade::unregister_event<reshade::addon_event::execute_command_list>(free_camera_bridge::execute);
    reshade::unregister_event<reshade::addon_event::reshade_overlay>(overlay);
    reshade::unregister_event<reshade::addon_event::reshade_open_overlay>(opened);
    reshade::unregister_event<reshade::addon_event::destroy_effect_runtime>(destroyed);
    reshade::unregister_event<reshade::addon_event::update_descriptor_tables>(camera_bridge::updated);
    reshade::unregister_event<reshade::addon_event::copy_resource>(free_camera_bridge::resource_copy);
    reshade::unregister_event<reshade::addon_event::copy_buffer_region>(free_camera_bridge::buffer_copy);
    reshade::unregister_event<reshade::addon_event::copy_buffer_region>(grid_bridge::copied);
    grid_bridge::cleanup();
    native_ground_bridge::cleanup();
    free_camera_bridge::cleanup();
    camera_bridge::cleanup();
    reshade::unregister_addon(addon);
}
