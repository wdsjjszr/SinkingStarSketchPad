#pragma once
#include "model.hpp"

namespace sketch {
constexpr double grid_round(double value){return value>=0?int(value+.5):int(value-.5);}
constexpr double grid_floor(double value){int whole=int(value);return value<whole?whole-1:whole;}
constexpr double magnitude(double value){return value<0?-value:value;}
constexpr Point nearest_grid_point(Point p){
    Point center{float(grid_round(p.x)),float(grid_round(p.y))},corner{float(grid_floor(p.x)+.5),float(grid_floor(p.y)+.5)};
    double a=(p.x-center.x)*(p.x-center.x)+(p.y-center.y)*(p.y-center.y);
    double b=(p.x-corner.x)*(p.x-corner.x)+(p.y-corner.y)*(p.y-corner.y);
    return a<=b?center:corner;
}
constexpr bool snaps_to_grid(Tool tool){return tool==Tool::Line||tool==Tool::Arrow||tool==Tool::Rectangle;}
constexpr Point grid_shape_end(Tool tool,Point a,Point b,bool shift){
    if(!shift)return snaps_to_grid(tool)?nearest_grid_point(b):b;
    double dx=b.x-a.x,dy=b.y-a.y,x=magnitude(dx),y=magnitude(dy),sx=dx<0?-1:1,sy=dy<0?-1:1;
    if(tool==Tool::Ellipse){double length=x>y?x:y;return {float(a.x+sx*length),float(a.y+sy*length)};}
    if(tool==Tool::Rectangle){double length=grid_round((x>y?x:y)*2)*.5;return {float(a.x+sx*length),float(a.y+sy*length)};}
    constexpr double tangent=.414213562373095;
    if(y<=x*tangent)return {float(a.x+grid_round(dx)),a.y};
    if(x<=y*tangent)return {a.x,float(a.y+grid_round(dy))};
    double length=grid_round(x+y)*.5;return {float(a.x+sx*length),float(a.y+sy*length)};
}
inline bool screen_point(const grid::Lattice &g,Point p,const grid::Camera &camera,Point size,Point &out){
    if(!g.ready){out=pixel(p,size);return true;}
    double x,y;if(!camera.project(g.world(p.x,p.y),x,y))return false;out={float(x*size.x),float(y*size.y)};return true;
}
inline bool plane_point(const grid::Lattice &g,const grid::Camera &camera,Point mouse,Point size,Point &out){
    if(!g.ready){out=normal(mouse,size);return true;}
    double i,j;if(!g.coordinates(camera,mouse.x/size.x,mouse.y/size.y,i,j))return false;out={float(i),float(j)};return true;
}
inline std::vector<Point> project_stroke(const Stroke &stroke,const grid::Camera &camera,Point size){
    if(!stroke.lattice.ready)return outline(stroke,size);
    std::vector<Point> result;
    for(auto local:outline(stroke,{1,1})){Point point;if(!screen_point(stroke.lattice,local,camera,size,point))return {};result.push_back(point);}
    return result;
}
inline bool projected_hit(const std::vector<Point> &points,Point mouse,float width){
    const float radius=width*.5f+9;
    if(points.size()==1)return distance(points[0],mouse)<=radius;
    for(size_t n=1;n<points.size();++n)if(segment_distance(mouse,points[n-1],points[n])<=radius)return true;
    return false;
}
inline std::vector<Point> arrow_head(const std::vector<Point> &points,float width){
    if(points.size()!=2)return {};
    Point a=points.front(),b=points.back();float length=distance(a,b);if(length<2)return {};
    float dx=(b.x-a.x)/length,dy=(b.y-a.y)/length,head=std::min(std::max(13.f,width*4),length*.6f);
    return {{b.x-dx*head-dy*head*.5f,b.y-dy*head+dx*head*.5f},b,{b.x-dx*head+dy*head*.5f,b.y-dy*head-dx*head*.5f}};
}
}
