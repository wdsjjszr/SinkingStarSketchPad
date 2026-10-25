#pragma once
#include "grid_preview.hpp"
#include <vector>

namespace sketch::grid {
constexpr unsigned preview_foreground=255,preview_outline=0;
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
}
