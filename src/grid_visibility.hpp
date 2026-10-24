#pragma once
#include "grid_preview.hpp"
#include "native_ground_pick.hpp"
#include <memory>

namespace sketch::grid {
constexpr int preview_gray=190;
constexpr double cross_arm=.12;
constexpr float border_width=2,cross_width=3;
struct CellMask {
    int x=0,y=0,width=0,height=0;
    std::vector<uint8_t> cells;
    CellMask()=default;
    CellMask(int first_x,int first_y,int columns,int rows,bool visible=true):x(first_x),y(first_y),width(columns),height(rows),cells(size_t(columns)*rows,uint8_t(visible)){}
    bool at(int i,int j)const{return i>=x&&j>=y&&i<x+width&&j<y+height&&cells[size_t(j-y)*width+i-x];}
    bool horizontal(int i,int j)const{return at(i,j)||at(i,j-1);}
    bool vertical(int i,int j)const{return at(i,j)||at(i-1,j);}
};
constexpr bool same(V a,V b){return a.x==b.x&&a.y==b.y&&a.z==b.z;}
constexpr bool same(const Lattice &a,const Lattice &b){return a.ready==b.ready&&same(a.origin,b.origin)&&same(a.u,b.u)&&same(a.v,b.v);}
inline bool same_view(const Camera &a,const Camera &b){return a.valid==b.valid&&a.matrix==b.matrix&&a.inverse==b.inverse&&same(a.eye,b.eye)&&same(a.forward,b.forward);}
inline bool visible_from(const native_ground::Snapshot &terrain,const Camera &camera,V center){
    double x,y;Ray ray;if(!camera.project(center,x,y)||!camera.ray(x,y,ray))return false;
    double distance=dot(center-ray.origin,ray.direction);if(distance<=0)return false;
    return !native_ground::occludes(native_ground::wall_distance(terrain,ray,distance),distance);
}
struct VisibilityCache {
    Camera view;
    Lattice plane;
    std::shared_ptr<const native_ground::Snapshot> terrain;
    CellMask mask;
    bool ready=false;
    bool update(const Camera &camera,const Lattice &lattice,std::shared_ptr<const native_ground::Snapshot> snapshot,int x,int y){
        if(ready&&same_view(view,camera)&&same(plane,lattice)&&terrain==snapshot&&mask.x==x&&mask.y==y)return false;
        view=camera;plane=lattice;terrain=std::move(snapshot);mask=CellMask(x,y,128,128,false);ready=true;
        for(int j=y;j<y+128;++j)for(int i=x;i<x+128;++i){
            if(cell_polygon(camera,lattice,i,j).count<3)continue;
            if(terrain&&!visible_from(*terrain,camera,lattice.world(i,j)))continue;
            mask.cells[size_t(j-y)*128+i-x]=1;
        }
        return true;
    }
};
}
