#pragma once
#include "ui_scale.hpp"
#include "imgui_abi.hpp"
#include <cmath>

namespace sketch::ui {
inline constexpr ImU32 ink=IM_COL32(232,234,240,255),muted=IM_COL32(154,163,181,255),gold=IM_COL32(255,181,71,255),teal=IM_COL32(45,212,191,255),border=IM_COL32(51,56,71,255);
inline ImVec2 vec(float x,float y){return ImVec2(x,y);}
inline const char *plain(const char *label){return static_cast<unsigned char>(label[0])==0xef&&label[3]==' '?label+4:label;}
inline Rect window_rect(){auto p=gui_vec2(imgui_function_table_instance()->GetWindowPos),s=gui_vec2(imgui_function_table_instance()->GetWindowSize);return {p.x,p.y,s.x,s.y};}
inline Rect item_rect(){auto a=gui_vec2(imgui_function_table_instance()->GetItemRectMin),b=gui_vec2(imgui_function_table_instance()->GetItemRectMax);return {a.x,a.y,b.x-a.x,b.y-a.y};}
struct FontScope {
    explicit FontScope(float ratio){ImGui::PushFont(nullptr,ImGui::GetStyle().FontSizeBase*ratio);}
    ~FontScope(){ImGui::PopFont();}
};
struct Style {
    int colors=0,vars=0;
    void color(ImGuiCol id,ImU32 value){ImGui::PushStyleColor(id,value);++colors;}
    void var(ImGuiStyleVar id,float value){ImGui::PushStyleVar(id,value);++vars;}
    void var(ImGuiStyleVar id,ImVec2 value){ImGui::PushStyleVar(id,value);++vars;}
    explicit Style(UIScale s){
        color(ImGuiCol_Text,ink);color(ImGuiCol_TextDisabled,muted);color(ImGuiCol_WindowBg,IM_COL32(27,30,38,250));
        color(ImGuiCol_ChildBg,0);color(ImGuiCol_PopupBg,IM_COL32(27,30,38,255));color(ImGuiCol_Border,border);
        color(ImGuiCol_FrameBg,IM_COL32(34,38,49,255));color(ImGuiCol_FrameBgHovered,IM_COL32(44,49,62,255));color(ImGuiCol_FrameBgActive,IM_COL32(51,56,71,255));
        color(ImGuiCol_Button,IM_COL32(34,38,49,255));color(ImGuiCol_ButtonHovered,IM_COL32(47,53,66,255));color(ImGuiCol_ButtonActive,IM_COL32(57,64,79,255));
        color(ImGuiCol_Header,IM_COL32(18,48,45,255));color(ImGuiCol_HeaderHovered,IM_COL32(29,70,63,255));color(ImGuiCol_HeaderActive,IM_COL32(29,85,75,255));
        color(ImGuiCol_CheckMark,teal);color(ImGuiCol_SliderGrab,ink);color(ImGuiCol_SliderGrabActive,teal);color(ImGuiCol_Separator,IM_COL32(42,46,58,255));
        color(ImGuiCol_ScrollbarBg,0);color(ImGuiCol_ScrollbarGrab,border);color(ImGuiCol_ScrollbarGrabHovered,muted);color(ImGuiCol_ScrollbarGrabActive,teal);
        var(ImGuiStyleVar_WindowPadding,vec(s.px(10),s.px(8)));var(ImGuiStyleVar_WindowRounding,s.px(10));var(ImGuiStyleVar_WindowBorderSize,s.px(1));
        var(ImGuiStyleVar_FramePadding,vec(s.px(7),s.px(5)));var(ImGuiStyleVar_FrameRounding,s.px(6));var(ImGuiStyleVar_FrameBorderSize,s.px(1));
        var(ImGuiStyleVar_ItemSpacing,vec(s.px(6),s.px(6)));var(ImGuiStyleVar_ItemInnerSpacing,vec(s.px(6),s.px(4)));
        var(ImGuiStyleVar_PopupRounding,s.px(8));var(ImGuiStyleVar_ScrollbarSize,s.px(8));var(ImGuiStyleVar_GrabMinSize,s.px(12));var(ImGuiStyleVar_GrabRounding,s.px(6));
        var(ImGuiStyleVar_CellPadding,vec(s.px(4),s.px(4)));
    }
    ~Style(){ImGui::PopStyleVar(vars);ImGui::PopStyleColor(colors);}
};
// The legacy camera keeps the host palette and only scales its existing metrics.
struct LegacyStyle {
    explicit LegacyStyle(UIScale s){
        const auto &v=ImGui::GetStyle();float r=s.ratio();
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,vec(v.WindowPadding.x*r,v.WindowPadding.y*r));
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding,vec(v.FramePadding.x*r,v.FramePadding.y*r));
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing,vec(v.ItemSpacing.x*r,v.ItemSpacing.y*r));
        ImGui::PushStyleVar(ImGuiStyleVar_ItemInnerSpacing,vec(v.ItemInnerSpacing.x*r,v.ItemInnerSpacing.y*r));
        ImGui::PushStyleVar(ImGuiStyleVar_ScrollbarSize,v.ScrollbarSize*r);ImGui::PushStyleVar(ImGuiStyleVar_GrabMinSize,v.GrabMinSize*r);
    }
    ~LegacyStyle(){ImGui::PopStyleVar(6);}
};
enum class Icon {Pen,Line,Arrow,Rectangle,Ellipse,Text,Eraser,Path,Undo,Redo,Clear,Save,Export,Camera,Help,Settings,Return,Grid,Eye,Character,Target,Close,Chevron};
inline void icon(Icon id,ImVec2 p,float size,ImU32 c){
    auto *d=ImGui::GetWindowDrawList();float k=size/24,t=size/14;
    auto at=[&](float x,float y){return vec(p.x+x*k,p.y+y*k);};
    auto line=[&](float x,float y,float a,float b){d->AddLine(at(x,y),at(a,b),c,t);};
    auto rect=[&](float x,float y,float a,float b,float round=0){d->AddRect(at(x,y),at(a,b),c,round*k,0,t);};
    auto circle=[&](float x,float y,float r){d->AddCircle(at(x,y),r*k,c,20,t);};
    switch(id){
    case Icon::Pen:line(5,15,15,5);line(15,5,19,9);line(19,9,9,19);line(9,19,4,20);line(4,20,5,15);line(6,14,10,18);break;
    case Icon::Line:line(5,19,19,5);break;
    case Icon::Arrow:line(5,19,19,5);line(9,5,19,5);line(19,5,19,15);break;
    case Icon::Rectangle:rect(4,6,20,18,1);break;
    case Icon::Ellipse:for(int n=0;n<32;++n){float a=n*6.2831853f/32,b=(n+1)*6.2831853f/32;line(12+8*std::cos(a),12+6*std::sin(a),12+8*std::cos(b),12+6*std::sin(b));}break;
    case Icon::Text:line(5,6,19,6);line(12,6,12,19);line(9,19,15,19);break;
    case Icon::Eraser:line(5,14,13,6);line(13,6,19,12);line(19,12,12,19);line(12,19,9,19);line(9,19,5,14);line(9,10,15,16);line(8,20,20,20);break;
    case Icon::Path:line(4,19,10,19);line(10,19,10,12);line(10,12,16,12);line(16,12,16,5);line(16,5,21,5);circle(4,19,1.2f);circle(21,5,1.2f);break;
    case Icon::Undo:case Icon::Redo:{bool flip=id==Icon::Redo;auto x=[&](float v){return flip?24-v:v;};line(x(9),14,x(4),9);line(x(4),9,x(9),4);line(x(4),9,x(14),9);line(x(14),9,x(19),13);line(x(19),13,x(18),18);line(x(18),18,x(14),20);line(x(14),20,x(10),20);break;}
    case Icon::Clear:line(5,7,19,7);rect(9,4,15,7);line(7,7,8,20);line(8,20,16,20);line(16,20,17,7);line(10,10,10,17);line(14,10,14,17);break;
    case Icon::Save:rect(5,4,19,20,1);rect(8,4,15,9);rect(8,14,16,20);break;
    case Icon::Export:line(12,4,12,15);line(7,10,12,15);line(12,15,17,10);line(5,18,5,21);line(5,21,19,21);line(19,21,19,18);break;
    case Icon::Camera:rect(3,7,21,20,2);rect(8,4,15,7,1);circle(12,13,3.5f);break;
    case Icon::Help:circle(12,12,9);line(9,8,11,6.5f);line(11,6.5f,14,7);line(14,7,15,10);line(15,10,12,13);line(12,13,12,14);circle(12,17.5f,.5f);break;
    case Icon::Settings:line(4,7,12,7);line(18,7,21,7);circle(15,7,2.5f);line(4,17,6,17);line(12,17,21,17);circle(9,17,2.5f);break;
    case Icon::Return:line(10,5,5,5);line(5,5,5,20);line(5,20,10,20);line(10,12,21,12);line(16,7,21,12);line(21,12,16,17);break;
    case Icon::Grid:rect(4,4,20,20);line(4,12,20,12);line(12,4,12,20);break;
    case Icon::Eye:line(2,12,7,6);line(7,6,17,6);line(17,6,22,12);line(22,12,17,18);line(17,18,7,18);line(7,18,2,12);circle(12,12,3);break;
    case Icon::Character:circle(12,6,3);line(6,21,6,17);line(6,17,9,13);line(9,13,15,13);line(15,13,18,17);line(18,17,18,21);break;
    case Icon::Target:circle(12,12,6);line(12,2,12,7);line(12,17,12,22);line(2,12,7,12);line(17,12,22,12);break;
    case Icon::Close:line(6,6,18,18);line(18,6,6,18);break;
    case Icon::Chevron:line(9,6,15,12);line(15,12,9,18);break;
    }
}
inline void text(ImVec2 p,const char *label,UIScale s,float size=12,ImU32 c=ink){ImGui::GetWindowDrawList()->AddText(ImGui::GetFont(),s.px(size),p,c,label);}
inline void hint(const char *label,const char *keys=nullptr){
    if(!ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort))return;
    ImGui::BeginTooltip();ImGui::TextUnformatted(plain(label));if(keys&&*keys)ImGui::TextDisabled("%s",keys);ImGui::EndTooltip();
}
enum class Tone {Plain,Gold,Teal};
inline bool button(const char *id,Icon glyph,const char *label,float w,float h,UIScale s,bool active=false,Tone tone=Tone::Plain,bool stacked=false){
    ImGui::PushStyleColor(ImGuiCol_Button,0);ImGui::PushStyleColor(ImGuiCol_ButtonHovered,0);ImGui::PushStyleColor(ImGuiCol_ButtonActive,0);
    bool clicked=ImGui::Button(id,vec(w,h));ImGui::PopStyleColor(3);auto r=item_rect();auto *d=ImGui::GetWindowDrawList();
    bool hover=ImGui::IsItemHovered();ImU32 fill=hover?IM_COL32(44,49,62,255):IM_COL32(34,38,49,255),edge=border,fg=ink;
    if(active&&tone==Tone::Gold){fill=gold;edge=gold;fg=IM_COL32(26,20,5,255);}
    if(active&&tone==Tone::Teal){fill=IM_COL32(18,48,45,255);edge=teal;}
    if(label||active||hover)d->AddRectFilled(vec(r.x,r.y),vec(r.x+r.w,r.y+r.h),fill,s.px(7));
    if(label)d->AddRect(vec(r.x,r.y),vec(r.x+r.w,r.y+r.h),edge,s.px(7),0,s.px(1));
    float sz=s.px(18);const char *caption=label?plain(label):nullptr;
    float ix=r.x+(r.w-sz)*.5f,iy=r.y+(r.h-sz)*.5f;
    if(caption){
        if(stacked){iy=r.y+s.px(7);auto m=gui_text_size(caption);float k=s.px(11)/ImGui::GetFontSize();text(vec(r.x+(r.w-m.x*k)*.5f,r.y+r.h-s.px(17)),caption,s,11,fg);}
        else{ix=r.x+s.px(8);text(vec(ix+sz+s.px(6),r.y+(r.h-s.px(22))*.5f),caption,s,11,fg);}
    }
    icon(glyph,vec(ix,iy),sz,active&&tone==Tone::Teal?teal:fg);return clicked;
}
inline bool checkbox(const char *label,bool &value,UIScale s){
    ImGui::PushID(label);ImGui::PushStyleColor(ImGuiCol_Button,0);ImGui::PushStyleColor(ImGuiCol_ButtonHovered,0);ImGui::PushStyleColor(ImGuiCol_ButtonActive,0);
    bool clicked=ImGui::Button("##check",vec(gui_vec2(imgui_function_table_instance()->GetContentRegionAvail).x,s.px(22)));ImGui::PopStyleColor(3);ImGui::PopID();
    if(clicked)value=!value;auto r=item_rect();auto *d=ImGui::GetWindowDrawList();float y=r.y+(r.h-s.px(14))*.5f;
    if(value)d->AddRectFilled(vec(r.x,y),vec(r.x+s.px(14),y+s.px(14)),teal,s.px(3));
    else d->AddRect(vec(r.x,y),vec(r.x+s.px(14),y+s.px(14)),ImGui::IsItemHovered()?teal:muted,s.px(3),0,s.px(1));
    text(vec(r.x+s.px(22),r.y+(r.h-s.px(12))*.5f),label,s);return clicked;
}
struct SliderSkin {
    bool input=ImGui::GetIO().KeyCtrl||ImGui::GetIO().WantTextInput;
    SliderSkin(){ImGui::PushStyleColor(ImGuiCol_FrameBg,input?IM_COL32(34,38,49,255):0);ImGui::PushStyleColor(ImGuiCol_FrameBgHovered,input?IM_COL32(44,49,62,255):0);ImGui::PushStyleColor(ImGuiCol_FrameBgActive,input?IM_COL32(51,56,71,255):0);ImGui::PushStyleColor(ImGuiCol_SliderGrab,0);ImGui::PushStyleColor(ImGuiCol_SliderGrabActive,0);ImGui::PushStyleColor(ImGuiCol_Text,input?ink:0);ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize,0);}
    ~SliderSkin(){ImGui::PopStyleVar();ImGui::PopStyleColor(6);}
    void draw(float fraction,UIScale s,ImU32 accent)const{
        if(input)return;auto r=item_rect();auto *d=ImGui::GetWindowDrawList();float lo=r.x+s.px(7),hi=r.x+r.w-s.px(7),x=lo+(hi-lo)*std::clamp(fraction,0.f,1.f),y=r.y+r.h*.5f;
        d->AddRectFilled(vec(lo,y-s.px(2)),vec(hi,y+s.px(2)),border,s.px(2));d->AddRectFilled(vec(lo,y-s.px(2)),vec(x,y+s.px(2)),accent,s.px(2));d->AddCircleFilled(vec(x,y),s.px(7),ink,20);
    }
};
inline bool slider(const char *id,int &value,int lo,int hi,float width,UIScale s,const char *format="%d%%",ImU32 accent=teal){
    ImGui::SetNextItemWidth(width);SliderSkin skin;bool changed=ImGui::SliderInt(id,&value,lo,hi,format,ImGuiSliderFlags_AlwaysClamp);skin.draw(float(value-lo)/(hi-lo),s,accent);return changed;
}
inline bool slider(const char *id,float &value,float lo,float hi,float width,UIScale s,const char *format="%.0f",ImU32 accent=gold){
    ImGui::SetNextItemWidth(width);SliderSkin skin;bool changed=ImGui::SliderFloat(id,&value,lo,hi,format,ImGuiSliderFlags_AlwaysClamp);skin.draw((value-lo)/(hi-lo),s,accent);return changed;
}
inline void header(const char *title,bool *open,UIScale s){
    float available=gui_vec2(imgui_function_table_instance()->GetContentRegionAvail).x;
    ImGui::InvisibleButton("##title",vec(std::max(s.px(1),available-(open?s.px(34):0)),s.px(28)));auto r=item_rect();text(vec(r.x,r.y+s.px(6)),title,s,13);
    if(ImGui::IsItemActive()&&ImGui::IsMouseDragging(ImGuiMouseButton_Left)){auto w=window_rect();auto delta=ImGui::GetIO().MouseDelta;ImGui::SetWindowPos(vec(w.x+delta.x,w.y+delta.y));}
    if(open){ImGui::SameLine();if(button("##close",Icon::Close,nullptr,s.px(28),s.px(28),s))*open=false;}
    ImGui::Separator();
}
inline void panel_size(float width,float max_height){
    ImGui::SetNextWindowSize(vec(width,0),ImGuiCond_Always);ImGui::SetNextWindowSizeConstraints(vec(width,0),vec(width,max_height));
}
inline constexpr ImGuiWindowFlags panel_flags=ImGuiWindowFlags_NoTitleBar|ImGuiWindowFlags_NoResize|ImGuiWindowFlags_NoCollapse|ImGuiWindowFlags_NoSavedSettings|ImGuiWindowFlags_NoNavInputs|ImGuiWindowFlags_NoDocking|ImGuiWindowFlags_AlwaysAutoResize;
inline Rect finish_panel(float viewport_width,float viewport_height,UIScale s){
    auto r=window_rect(),f=fit(r,viewport_width,viewport_height,s.px(8));
    if(r.x!=f.x||r.y!=f.y)ImGui::SetWindowPos(vec(f.x,f.y));ImGui::End();return f;
}
}
