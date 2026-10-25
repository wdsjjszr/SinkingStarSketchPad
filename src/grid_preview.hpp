#pragma once
#include "grid_math.hpp"
#include <cstdint>

namespace sketch::grid {
struct Preview {
    bool enabled=true,centers=true,intersections=false,borders=false,checker=false;
    int opacity=45;
    constexpr int alpha()const{return (opacity*255+50)/100;}
    constexpr bool visible()const{return opacity>0&&(centers||intersections||borders||checker);}
};
enum class HintMode {On,Off,WheelOnly};
enum class HintSource {Initialize,Mark,Wheel};
struct HintPreferences {
    Preview style{true,false,false,true,true,15};
    HintMode mode=HintMode::On;
    unsigned duration_ms=750;
    constexpr bool permits(HintSource source)const{
        return duration_ms&&source!=HintSource::Initialize&&mode!=HintMode::Off&&(mode!=HintMode::WheelOnly||source==HintSource::Wheel);
    }
};
constexpr double fade_weight(double elapsed,unsigned hold){
    if(!hold||elapsed<0)return 0;
    if(elapsed<=hold)return 1;
    double remaining=1-(elapsed-hold)/(hold*.3);return remaining>0?remaining*remaining*remaining:0;
}
constexpr int preview_radius=8,preview_cells=2*preview_radius+1;
constexpr double radial_weight(double distance){double t=distance/(preview_radius+.5);return t>=1?0:t<=0?1:1-t*t*(3-2*t);}
constexpr unsigned checker_shade(int i,int j){return (i+j)%2?255:0;}
struct PlaneHint {
    Lattice lattice;
    uint64_t started=0,generation=0;
    unsigned page=0;
    HintSource source=HintSource::Initialize;
    bool showing=false;
    constexpr void clear(){showing=false;}
    constexpr void show(Lattice plane,uint64_t now,unsigned current_page,uint64_t scene,HintSource reason,const HintPreferences &preferences){
        clear();if(!preferences.permits(reason))return;
        lattice=plane;started=now;page=current_page;generation=scene;source=reason;showing=true;
    }
    constexpr bool active(uint64_t now,unsigned current_page,uint64_t scene,const HintPreferences &preferences)const{
        return showing&&preferences.permits(source)&&page==current_page&&generation==scene&&now>=started&&fade_weight(double(now-started),preferences.duration_ms)>0;
    }
    constexpr void expire(uint64_t now,unsigned current_page,uint64_t scene,const HintPreferences &preferences){if(!active(now,current_page,scene,preferences))clear();}
};
struct ClipPoint {double x=0,y=0,w=0;};
constexpr ClipPoint mix(ClipPoint a,ClipPoint b,double t){return {a.x+(b.x-a.x)*t,a.y+(b.y-a.y)*t,a.w+(b.w-a.w)*t};}
constexpr double clip_distance(ClipPoint p,unsigned side){return side==0?p.w-1e-6:side==1?p.x+p.w:side==2?p.w-p.x:side==3?p.y+p.w:p.w-p.y;}
struct Polygon {std::array<ClipPoint,12> points{};unsigned count=0;};
constexpr Polygon clip(Polygon polygon){
    for(unsigned side=0;side<5&&polygon.count;++side){
        Polygon out;
        for(unsigned n=0;n<polygon.count;++n){
            auto a=polygon.points[n],b=polygon.points[(n+1)%polygon.count];double da=clip_distance(a,side),db=clip_distance(b,side);
            if(da>=0)out.points[out.count++]=a;
            if((da>0&&db<0)||(da<0&&db>0))out.points[out.count++]=mix(a,b,da/(da-db));
        }
        polygon=out;
    }
    return polygon;
}
inline ClipPoint clip_point(const Camera &camera,V p){
    return {camera.matrix[0]*p.x+camera.matrix[1]*p.y+camera.matrix[2]*p.z+camera.matrix[3],
            camera.matrix[4]*p.x+camera.matrix[5]*p.y+camera.matrix[6]*p.z+camera.matrix[7],
            camera.matrix[12]*p.x+camera.matrix[13]*p.y+camera.matrix[14]*p.z+camera.matrix[15]};
}
inline Polygon cell_polygon(const Camera &camera,const Lattice &g,double i,double j){
    Polygon p;p.count=4;
    p.points={clip_point(camera,g.world(i-.5,j-.5)),clip_point(camera,g.world(i+.5,j-.5)),
              clip_point(camera,g.world(i+.5,j+.5)),clip_point(camera,g.world(i-.5,j+.5))};return clip(p);
}
inline bool clip_line(const Camera &camera,V first,V second,ClipPoint &a,ClipPoint &b){
    a=clip_point(camera,first);b=clip_point(camera,second);
    for(unsigned side=0;side<5;++side){
        double da=clip_distance(a,side),db=clip_distance(b,side);if(da<0&&db<0)return false;
        if((da>=0)!=(db>=0)){auto point=mix(a,b,da/(da-db));if(da<0)a=point;else b=point;}
    }
    return true;
}
inline bool preview_focus(const Camera &camera,const Lattice &g,double &i,double &j){
    if(g.coordinates(camera,.5,.5,i,j))return true;
    bool found=false;double best=0;
    for(auto pixel:std::array<std::array<double,2>,4>{{{0,0},{1,0},{0,1},{1,1}}}){
        double x,y;if(!g.coordinates(camera,pixel[0],pixel[1],x,y))continue;
        auto delta=g.world(x,y)-camera.eye;double distance=dot(delta,delta);
        if(!found||distance<best){found=true;best=distance;i=x;j=y;}
    }
    return found;
}
}
