#pragma once
#include "grid_visibility.hpp"

namespace sketch::grid {
constexpr unsigned preview_batch_triangles=20000;
constexpr double dot_radius=2.5,dot_outer_radius=5.5;
static_assert(preview_batch_triangles*3<65536,"Each primitive reservation fits 16-bit indices before the SDK applies its vertex offset");
struct PreviewVertex {double x=0,y=0,alpha=0;};
constexpr PreviewVertex interpolate(PreviewVertex a,PreviewVertex b,double t){return {a.x+(b.x-a.x)*t,a.y+(b.y-a.y)*t,a.alpha+(b.alpha-a.alpha)*t};}
struct PreviewTriangle {std::array<PreviewVertex,3> points;unsigned shade=preview_foreground;};
struct PreviewMesh {
    std::vector<PreviewTriangle> triangles;
    void polygon(const std::vector<PreviewVertex> &points,unsigned shade){
        for(size_t n=2;n<points.size();++n)triangles.push_back({{points[0],points[n-1],points[n]},shade});
    }
    void strip(PreviewVertex a,PreviewVertex b,double low,double high,unsigned shade){
        double length=std::hypot(b.x-a.x,b.y-a.y);if(length<1e-8)return;
        double x=(b.y-a.y)/length,y=(a.x-b.x)/length;
        polygon({{a.x+x*high,a.y+y*high,a.alpha},{b.x+x*high,b.y+y*high,b.alpha},{b.x+x*low,b.y+y*low,b.alpha},{a.x+x*low,a.y+y*low,a.alpha}},shade);
    }
    void line(PreviewVertex a,PreviewVertex b,double width){strip(a,b,-width*.5,width*.5,preview_foreground);}
    void line_outline(PreviewVertex a,PreviewVertex b,double width){strip(a,b,width*.5,width*.5+1,preview_outline);strip(a,b,-width*.5-1,-width*.5,preview_outline);}
    void dot(PreviewVertex center){
        for(unsigned n=0;n<24;++n){double a=n*6.283185307179586/24,b=(n+1)*6.283185307179586/24;
            triangles.push_back({{center,{center.x+dot_radius*std::cos(a),center.y+dot_radius*std::sin(a),center.alpha},{center.x+dot_radius*std::cos(b),center.y+dot_radius*std::sin(b),center.alpha}},preview_foreground});}
    }
    void dot_outline(PreviewVertex center){
        for(unsigned n=0;n<24;++n){double a=n*6.283185307179586/24,b=(n+1)*6.283185307179586/24;
            polygon({{center.x+dot_radius*std::cos(a),center.y+dot_radius*std::sin(a),center.alpha},{center.x+dot_outer_radius*std::cos(a),center.y+dot_outer_radius*std::sin(a),center.alpha},
                     {center.x+dot_outer_radius*std::cos(b),center.y+dot_outer_radius*std::sin(b),center.alpha},{center.x+dot_radius*std::cos(b),center.y+dot_radius*std::sin(b),center.alpha}},preview_outline);}
    }
};
struct PreviewPainter {
    const Camera &camera;
    const Lattice &plane;
    double width,height,opacity,focus_i=0,focus_j=0;
    bool radial=false;
    PreviewMesh mesh,outlines,cores;
    PreviewVertex vertex(ClipPoint p)const{
        double x=(p.x/p.w+1)*.5,y=(1-p.y/p.w)*.5,weight=opacity;
        if(radial){double i,j;weight=plane.coordinates(camera,x,y,i,j)?weight*radial_weight(std::hypot(i-focus_i,j-focus_j)):0;}
        return {x*width,y*height,weight};
    }
    void line(V a,V b,double thickness){ClipPoint first,last;if(clip_line(camera,a,b,first,last)){auto x=vertex(first),y=vertex(last);outlines.line_outline(x,y,thickness);cores.line(x,y,thickness);}}
    void cell(int i,int j){auto polygon=cell_polygon(camera,plane,i,j);std::vector<PreviewVertex> points;for(unsigned n=0;n<polygon.count;++n)points.push_back(vertex(polygon.points[n]));mesh.polygon(points,checker_shade(i,j));}
    void dot(int i,int j){auto p=clip_point(camera,plane.world(i,j));if(p.w>1e-6&&p.x>=-p.w&&p.x<=p.w&&p.y>=-p.w&&p.y<=p.w){auto center=vertex(p);outlines.dot_outline(center);cores.dot(center);}}
    void layers(const Preview &style,const CellMask &mask){
        if(!style.visible())return;
        if(style.checker||style.centers)for(int j=mask.y;j<mask.y+mask.height;++j)for(int i=mask.x;i<mask.x+mask.width;++i){
            if(!mask.at(i,j))continue;if(style.checker)cell(i,j);if(style.centers)dot(i,j);
        }
        for(int j=mask.y;j<=mask.y+mask.height;++j)for(int i=mask.x;i<=mask.x+mask.width;++i){
            auto p=plane.world(i-.5,j-.5);
            if(style.borders){if(mask.horizontal(i,j))line(p,plane.world(i+.5,j-.5),border_width);if(mask.vertical(i,j))line(p,plane.world(i-.5,j+.5),border_width);}
            if(style.intersections){
                if(mask.horizontal(i,j))line(p,p+plane.u*cross_arm,cross_width);
                if(mask.horizontal(i-1,j))line(p,p-plane.u*cross_arm,cross_width);
                if(mask.vertical(i,j))line(p,p+plane.v*cross_arm,cross_width);
                if(mask.vertical(i,j-1))line(p,p-plane.v*cross_arm,cross_width);
            }
        }
        mesh.triangles.reserve(mesh.triangles.size()+outlines.triangles.size()+cores.triangles.size());
        mesh.triangles.insert(mesh.triangles.end(),outlines.triangles.begin(),outlines.triangles.end());
        mesh.triangles.insert(mesh.triangles.end(),cores.triangles.begin(),cores.triangles.end());
    }
};
}
