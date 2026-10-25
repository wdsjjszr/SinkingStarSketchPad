#pragma once
#include <imgui.h>
#include <cmath>
#include <algorithm>

namespace sketch::cursor_style {
enum class Mode { Pointer, Drawing, Eraser };
inline constexpr ImU32 ink=IM_COL32(28,22,21,245), bronze=IM_COL32(136,88,36,255),
    gold=IM_COL32(240,191,79,255), light=IM_COL32(255,237,170,255), ivory=IM_COL32(255,251,219,255);

// Vector artwork stays sharp at the game's back-buffer resolution. The tip
// (pointer) or empty central gap (drawing/eraser) is the true mouse position.
inline void draw(ImDrawList *d,ImVec2 mouse,Mode mode,ImU32 paint,float scale=1){
    auto p=[&](float x,float y){return ImVec2(mouse.x+x*scale,mouse.y+y*scale);};
    auto line=[&](float ax,float ay,float bx,float by,ImU32 color,float width){d->AddLine(p(ax,ay),p(bx,by),color,width*scale);};
    if(mode==Mode::Pointer){
        ImVec2 silhouette[]={p(0,0),p(23,16),p(14,18),p(19,30),p(12,34),p(7,22),p(2,27)};
        // A dark cut edge, warm metal body and pale crystal facet mirror the
        // game's gold star emblems while preserving an unambiguous arrow tip.
        d->AddConcavePolyFilled(silhouette,7,gold);
        d->AddTriangleFilled(p(0,0),p(23,16),p(11,16),light);
        d->AddTriangleFilled(p(0,0),p(11,16),p(2,27),ivory);
        ImVec2 handle[]={p(7,20),p(14,18),p(19,30),p(12,34)};
        d->AddConvexPolyFilled(handle,4,bronze);
        d->AddPolyline(silhouette,7,ink,ImDrawFlags_Closed,2.3f*scale);
        line(2.4f,5,10.8f,15.9f,light,1);
        // Small eight-point star engraved in the grip, with an ivory core.
        ImVec2 star[16];
        for(int n=0;n<16;++n){float angle=-1.570796327f+n*.392699082f;
            float radius=n%2?1.55f:(n%4==0?5.f:3.7f);
            star[n]=p(13.3f+std::cos(angle)*radius,26.f+std::sin(angle)*radius);}
        d->AddConcavePolyFilled(star,16,light);
        d->AddCircleFilled(p(13.3f,26),1.05f*scale,ivory,12);
        return;
    }
    if(mode==Mode::Eraser){
        d->AddCircle(mouse,12*scale,ink,40,4*scale);
        d->AddCircle(mouse,12*scale,gold,40,1.5f*scale);
        for(int n=0;n<4;++n){float a=n*1.570796327f,c=std::cos(a),s=std::sin(a);
            line(c*10,s*10,c*16,s*16,ink,4);line(c*10,s*10,c*16,s*16,light,1.5f);}
        // The working radius stays open so underlying marks remain visible.
        return;
    }
    for(int n=0;n<4;++n){float a=n*1.570796327f,c=std::cos(a),s=std::sin(a);
        auto arm=[&](float u,float v){return p(c*u-s*v,s*u+c*v);};
        ImVec2 ray[]={arm(4.5f,0),arm(10,-2),arm(16,0),arm(10,2)};
        d->AddConvexPolyFilled(ray,4,paint);d->AddPolyline(ray,4,ink,ImDrawFlags_Closed,1.7f*scale);
        d->AddCircleFilled(arm(20,0),1.3f*scale,paint,12);
    }
    // A tiny diamond reports the selected ink colour at the exact hotspot.
    ImVec2 center[]={p(0,-2.1f),p(2.1f,0),p(0,2.1f),p(-2.1f,0)};
    d->AddConvexPolyFilled(center,4,paint);d->AddPolyline(center,4,ink,ImDrawFlags_Closed,scale);
}
}
