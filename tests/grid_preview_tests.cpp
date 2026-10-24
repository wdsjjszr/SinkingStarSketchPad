#include "../src/grid_visibility.hpp"
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
static_assert(preview_gray==190&&cross_arm==.12&&cross_width>border_width,"Gray crosses follow the grid axes and are slightly thicker than borders");
constexpr bool clipping(){
    Polygon p;p.count=4;p.points={ClipPoint{-2,-2,1},ClipPoint{2,-2,1},ClipPoint{2,2,1},ClipPoint{-2,2,1}};
    auto out=clip(p);if(out.count!=4)return false;
    for(unsigned n=0;n<out.count;++n)for(unsigned side=0;side<5;++side)if(clip_distance(out.points[n],side)<0)return false;
    p.points={ClipPoint{-1,-1,-1},ClipPoint{1,-1,-1},ClipPoint{1,1,-1},ClipPoint{-1,1,-1}};
    return clip(p).count==0;
}
static_assert(clipping(),"Checker polygons clip to the viewport and reject the back of the camera");
int main(){
    PlaneHint hint;assert(!hint.active(0,0,0));
    CellMask mask(-1,-1,2,2,false);mask.cells[0]=1;
    assert(mask.at(-1,-1)&&!mask.at(0,0)&&!mask.at(-2,-1));
    assert(mask.horizontal(-1,-1)&&mask.horizontal(-1,0)&&!mask.horizontal(0,0));
    assert(mask.vertical(-1,-1)&&mask.vertical(0,-1)&&!mask.vertical(0,0));
    Camera camera;camera.valid=true;camera.eye={0,0,10};camera.forward={0,0,-1};
    camera.matrix={.1,0,0,0,0,.1,0,0,0,0,-.1,1,0,0,0,1};
    camera.inverse={10,0,0,0,0,10,0,0,0,0,-10,10,0,0,0,1};
    sketch::native_ground::Scene scene;sketch::native_ground::add_wall(scene,{0,0,0},{1,1,2},{3},1);
    auto terrain=std::make_shared<sketch::native_ground::Snapshot>();terrain->set_geometry(scene);
    assert(visible_from(*terrain,camera,{0,0,2})&&!visible_from(*terrain,camera,{0,0,0}));
    assert(visible_from(*terrain,camera,{0,0,3})&&visible_from(*terrain,camera,{2,0,-.25}));
    auto reverse=camera;reverse.matrix[10]=.1;reverse.matrix[11]=0;reverse.inverse[10]=10;reverse.inverse[11]=0;
    assert(visible_from(*terrain,reverse,{0,0,2})&&!visible_from(*terrain,reverse,{0,0,0}));
    Lattice plane{true,{0,0,0},{1,0,0},{0,-1,0}};VisibilityCache cache;
    assert(cache.update(camera,plane,terrain,-64,-64)&&!cache.mask.at(0,0)&&cache.mask.at(2,0));
    camera.frame++;assert(!cache.update(camera,plane,terrain,-64,-64));
    camera.eye.z=11;assert(cache.update(camera,plane,terrain,-64,-64));
    plane.origin.z=2;assert(cache.update(camera,plane,terrain,-64,-64)&&cache.mask.at(0,0));
    assert(cache.update(camera,plane,nullptr,-64,-64)&&cache.mask.at(0,0));
    assert(cache.update(camera,plane,nullptr,-63,-64));
}
