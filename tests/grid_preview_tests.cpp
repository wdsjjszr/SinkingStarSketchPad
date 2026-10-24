#include "../src/grid_preview.hpp"
#include <cassert>
using namespace sketch::grid;
constexpr bool hints(){
    PlaneHint hint;Lattice a{true,{0,0,2},{1,0,0},{0,-1,0}},b=a;b.origin.z=3;
    hint.show(a,10,0,1);if(!hint.active(2009,0,1)||hint.active(2010,0,1)||hint.active(20,1,1))return false;
    hint.show(b,100,0,1);if(hint.lattice.origin.z!=3||!hint.active(2099,0,1))return false;
    hint.show(b,200,0,1);if(!hint.active(2199,0,1)||hint.active(201,0,2))return false;
    hint.clear();return !hint.active(201,0,1);
}
static_assert(hints(),"Only the newest plane/page/scene hint lasts two seconds, including reselecting the same plane");
static_assert(Preview{}.centers&&!Preview{}.intersections&&!Preview{}.borders&&!Preview{}.checker&&Preview{}.opacity==45,"Preview defaults preserve center dots");
static_assert(Preview{}.alpha()==115&&Preview{true,true,false,false,false,100}.alpha()==255&&Preview{true,true,false,false,false,0}.alpha()==0,"Opacity includes exact transparent and opaque endpoints");
static_assert(inside_height(.5,0)&&inside_height(0,0)&&!inside_height(1,0)&&inside_height(-.5,-1),"Cut solid interiors, retain voxel tops and handle negative heights");
constexpr bool clipping(){
    Polygon p;p.count=4;p.points={ClipPoint{-2,-2,1},ClipPoint{2,-2,1},ClipPoint{2,2,1},ClipPoint{-2,2,1}};
    auto out=clip(p);if(out.count!=4)return false;
    for(unsigned n=0;n<out.count;++n)for(unsigned side=0;side<5;++side)if(clip_distance(out.points[n],side)<0)return false;
    p.points={ClipPoint{-1,-1,-1},ClipPoint{1,-1,-1},ClipPoint{1,1,-1},ClipPoint{-1,1,-1}};
    return clip(p).count==0;
}
static_assert(clipping(),"Checker polygons clip to the viewport and reject the back of the camera");
int main(){PlaneHint hint;assert(!hint.active(0,0,0));}
