#include "../src/grid_visibility.hpp"
#include "../src/preview_mesh.hpp"
#include <cassert>
using namespace sketch::grid;
constexpr bool hints(){
    HintPreferences preferences;preferences.duration_ms=1000;PlaneHint hint;Lattice a{true,{0,0,2},{1,0,0},{0,-1,0}},b=a;b.origin.z=3;
    hint.show(a,10,0,1,HintSource::Initialize,preferences);if(hint.showing)return false;
    hint.show(a,10,0,1,HintSource::Mark,preferences);
    if(!hint.active(1309,0,1,preferences)||hint.active(1310,0,1,preferences)||hint.active(20,1,1,preferences))return false;
    hint.show(b,100,0,1,HintSource::Wheel,preferences);if(hint.lattice.origin.z!=3||!hint.active(1099,0,1,preferences))return false;
    hint.show(b,200,0,1,HintSource::Mark,preferences);if(!hint.active(1199,0,1,preferences)||hint.active(201,0,2,preferences))return false;
    if(!hint.active(1499,0,1,preferences)||hint.active(1500,0,1,preferences))return false;
    preferences.duration_ms=5000;if(!hint.active(6699,0,1,preferences)||hint.active(6700,0,1,preferences))return false;
    preferences.mode=HintMode::WheelOnly;hint.expire(201,0,1,preferences);if(hint.showing)return false;
    preferences.mode=HintMode::On;if(hint.active(202,0,1,preferences))return false;
    hint.show(a,300,0,1,HintSource::Mark,preferences);preferences.duration_ms=0;hint.expire(301,0,1,preferences);
    preferences.duration_ms=1000;if(hint.active(302,0,1,preferences))return false;
    hint.show(a,400,0,1,HintSource::Mark,preferences);hint.expire(1700,0,1,preferences);
    preferences.duration_ms=5000;return !hint.active(1701,0,1,preferences);
}
static_assert(hints(),"Initialization stays silent; hints use the newest mark/page/scene, live duration and policy, without reviving expired hints");
static_assert(!HintPreferences{}.style.centers&&!HintPreferences{}.style.intersections&&!HintPreferences{}.style.borders&&HintPreferences{}.style.checker&&HintPreferences{}.style.opacity==15&&HintPreferences{}.duration_ms==750&&Preview{}.enabled,"Hint defaults enable checker only at 15 percent for a 750ms hold; stored borders still override the default");
static_assert(fade_weight(750,750)==1&&fade_weight(975,750)==0&&fade_weight(1150,1000)==.125&&fade_weight(0,0)==0,"Hold is followed by a 30 percent cubic ease-out fade");
static_assert(preview_cells==17&&radial_weight(0)==1&&radial_weight(4.25)==.5&&radial_weight(8.5)==0&&checker_shade(-1,0)==255&&checker_shade(-2,0)==0,"17-cell smooth radial falloff and black/white parity are stable for negative cells");
constexpr bool policies(){
    HintPreferences p;if(!p.permits(HintSource::Mark)||!p.permits(HintSource::Wheel)||p.permits(HintSource::Initialize))return false;
    p.mode=HintMode::WheelOnly;if(p.permits(HintSource::Mark)||!p.permits(HintSource::Wheel))return false;
    p.mode=HintMode::Off;return !p.permits(HintSource::Mark)&&!p.permits(HintSource::Wheel);
}
static_assert(policies(),"On, off and wheel-only policies exclude automatic level initialization");
static_assert(Preview{}.centers&&!Preview{}.intersections&&!Preview{}.borders&&!Preview{}.checker&&Preview{}.opacity==45,"Preview defaults preserve center dots");
static_assert(Preview{}.alpha()==115&&Preview{true,true,false,false,false,100}.alpha()==255&&Preview{true,true,false,false,false,0}.alpha()==0,"Opacity includes exact transparent and opaque endpoints");
static_assert(preview_foreground==255&&preview_outline==0&&cross_arm==.12&&cross_width>border_width,"White crosses with black outlines follow the grid axes and retain their core size");
static_assert(dot_radius==2.5&&dot_outer_radius==4&&dot_outer_radius-dot_radius==1.5,"Smaller white center dots retain the 1.5-pixel black outline thickness");
constexpr bool clipping(){
    Polygon p;p.count=4;p.points={ClipPoint{-2,-2,1},ClipPoint{2,-2,1},ClipPoint{2,2,1},ClipPoint{-2,2,1}};
    auto out=clip(p);if(out.count!=4)return false;
    for(unsigned n=0;n<out.count;++n)for(unsigned side=0;side<5;++side)if(clip_distance(out.points[n],side)<0)return false;
    p.points={ClipPoint{-1,-1,-1},ClipPoint{1,-1,-1},ClipPoint{1,1,-1},ClipPoint{-1,1,-1}};
    return clip(p).count==0;
}
static_assert(clipping(),"Checker polygons clip to the viewport and reject the back of the camera");
int main(){
    PlaneHint hint;assert(!hint.active(0,0,0,HintPreferences{}));
    CellMask mask(-1,-1,2,2,false);mask.cells[0]=1;
    assert(mask.at(-1,-1)&&!mask.at(0,0)&&!mask.at(-2,-1));
    assert(mask.horizontal(-1,-1)&&mask.horizontal(-1,0)&&!mask.horizontal(0,0));
    assert(mask.vertical(-1,-1)&&mask.vertical(0,-1)&&!mask.vertical(0,0));
    Camera camera;camera.valid=true;camera.eye={0,0,10};camera.forward={0,0,-1};
    camera.matrix={.1,0,0,0,0,.1,0,0,0,0,-.1,1,0,0,0,1};
    camera.inverse={10,0,0,0,0,10,0,0,0,0,-10,10,0,0,0,1};
    Lattice plane{true,{0,0,2},{1,0,0},{0,-1,0}};
    PreviewPainter painter{camera,plane,1000,1000,.45,0,0,true,{}};painter.layers(Preview{true,true,true,true,true,45},CellMask(-preview_radius,-preview_radius,preview_cells,preview_cells));
    assert(painter.mesh.triangles.size()*3>65536);for(auto triangle:painter.mesh.triangles)for(auto vertex:triangle.points)assert(vertex.alpha>=0&&vertex.alpha<=.45);
    PreviewMesh border;border.line_outline({0,0,.4},{20,0,.4},2);
    for(auto triangle:border.triangles){assert(triangle.shade==0);for(auto p:triangle.points)assert(std::abs(p.y)>=1&&std::abs(p.y)<=2&&p.alpha==.4);}
    PreviewMesh dot;dot.dot_outline({0,0,.4});for(auto triangle:dot.triangles){assert(triangle.shade==0);for(auto p:triangle.points)assert(std::hypot(p.x,p.y)>2.49&&std::hypot(p.x,p.y)<4.01);}
    PreviewMesh body;body.line({0,0,.4},{20,0,.4},2);body.dot({0,0,.4});for(auto triangle:body.triangles)assert(triangle.shade==255);
}
