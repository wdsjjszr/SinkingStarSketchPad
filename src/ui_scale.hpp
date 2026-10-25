#pragma once
#include <algorithm>

namespace sketch::ui {
struct TextStyle {float size;bool bold;};
namespace typography {
inline constexpr TextStyle title{13,true},section{11,true},body{12,false},note{11,false},caption{10.5f,false};
}
constexpr float pixel_round(float value){return float(value<0?int(value-.5f):int(value+.5f));}
struct UIScale {
    int percent;
    float font_pixels;
    constexpr UIScale(int value=100,float font=18):percent(std::clamp(value,75,200)),font_pixels(font){}
    constexpr float ratio()const{return percent*.01f;}
    // ImGui truncates auto-fit sizes; fractional padding can create a spurious scrollbar.
    constexpr float px(float value)const{return pixel_round(value*font_pixels/12*ratio());}
    constexpr float legacy(float value)const{return pixel_round(value*ratio());}
};
struct ScaleSetting {
    int applied,pending;
    constexpr ScaleSetting(int value=100):applied(UIScale(value).percent),pending(applied){}
    constexpr bool apply(){pending=UIScale(pending).percent;bool changed=applied!=pending;applied=pending;return changed;}
};
struct Rect {
    float x=0,y=0,w=0,h=0;
    constexpr bool contains(float px,float py)const{return px>=x&&py>=y&&px<x+w&&py<y+h;}
};
struct WindowPosition {
    Rect target;
    bool pending=false;
    constexpr void move(Rect current,float dx,float dy){target=current;target.x+=dx;target.y+=dy;pending=true;}
    constexpr bool take(Rect &next){if(!pending)return false;next=target;pending=false;return true;}
};
constexpr Rect fit(Rect r,float width,float height,float margin){
    r.w=std::min(r.w,std::max(1.f,width-2*margin));r.h=std::min(r.h,std::max(1.f,height-2*margin));
    r.x=std::clamp(r.x,margin,std::max(margin,width-margin-r.w));
    r.y=std::clamp(r.y,margin,std::max(margin,height-margin-r.h));return r;
}
constexpr Rect beside(Rect panel,float top,float width,float height,float viewport_width,float viewport_height,float gap){
    float x=panel.x+panel.w+gap;
    if(x+width>viewport_width-gap)x=panel.x-gap-width;
    return fit({x,top,width,height},viewport_width,viewport_height,gap);
}
struct DismissClick {
    bool held=false;
    constexpr void consume(){held=true;}
    constexpr bool block(bool mouse_down){bool blocked=held;if(!mouse_down)held=false;return blocked;}
};
}
