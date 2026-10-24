#include "../src/free_camera.hpp"
#include "../src/native_ground_pick.hpp"
#include <cassert>
#include <iostream>
using namespace sketch;
std::array<float,240> fixture(double eye_x=0){
    using namespace free_camera;std::array<float,240> p{};
    Mat view=translation({-eye_x,0,-30}),proj={1.2,0,0,0,0,1.8,0,0,0,0,-1.001,-.1001,0,0,-1,0},vp=mul(proj,view),inv;
    store(p.data(),vp);assert(inverse(vp,inv));store(p.data()+16,inv);store(p.data()+160,view);store(p.data()+192,proj);assert(inverse(proj,inv));store(p.data()+176,inv);
    p[208]=float(eye_x);p[210]=30;p[214]=-1;p[216]=1.f/960;p[217]=1.f/640;
    uint32_t type=8,frame=10;std::memcpy(p.data()+226,&type,4);std::memcpy(p.data()+225,&frame,4);return p;
}
int main(){
    using namespace free_camera;
    auto original=fixture();auto frame=decode(original.data());assert(frame.valid);
    Controller c;assert(c.enable(frame,0));auto p=original;assert(c.apply(p.data()));for(int i=0;i<32;++i)assert(std::abs(p[i]-original[i])<.0001);
    c.move(1,1,1,.1,false);assert(std::abs(grid::length(c.offset)-.3)<1e-6);c.rotate(31,42);c.magnify(4);p=original;assert(c.apply(p.data()));
    auto changed=decode(p.data());assert(changed.valid&&pair_valid(changed.vp,changed.inv));
    assert(grid::length(changed.eye-frame.eye)>1&&grid::length(changed.forward-frame.forward)>.1);
    grid::Camera camera{changed.vp,changed.inv,0,true,changed.eye,changed.forward};grid::Lattice cells{true,{.5,.5,0},{1,0,0},{0,1,0}};
    native_ground::Scene terrain;terrain.bounds={-5,-5,11,11,false};native_ground::Snapshot ground;ground.set_geometry(terrain);
    double pick_x,pick_y;assert(camera.project({0,0,0},pick_x,pick_y));
    auto hit=native_ground::pick(ground,camera,pick_x,pick_y);
    assert(hit.result==native_ground::PickResult::Ground&&grid::length(hit.center)<1e-6);
    for(int i=-3;i<=3;++i)for(int j=-3;j<=3;++j){double x,y,a,b;assert(camera.project(cells.world(i,j),x,y));assert(cells.snap(camera,x,y,a,b)&&a==i&&b==j);}
    auto moved_game=fixture(25);assert(c.apply(moved_game.data()));for(int i=0;i<32;++i)assert(std::abs(moved_game[i]-p[i])<.001); // remains independent of gameplay follow camera
    auto reflected=original;auto mirror=identity();mirror[10]=-1;auto reflected_vp=mul(frame.vp,mirror);Mat inv;assert(inverse(reflected_vp,inv));store(reflected.data()+32,reflected_vp);store(reflected.data()+96,inv);
    assert(c.apply(reflected.data()));auto expected=mul(load(reflected.data()),mirror);for(int i=0;i<16;++i)assert(std::abs(reflected[32+i]-expected[i])<.001);assert(pair_valid(load(reflected.data()+32),load(reflected.data()+96)));
    c.enabled=false;p=original;assert(!c.apply(p.data())&&p==original);
    c.enable(frame,0);c.rotate(1e7,10000);assert(c.pitch==85&&std::abs(c.yaw)<=180);c.magnify(1e9);assert(c.zoom<=20);c.magnify(-1e9);assert(c.zoom>=.05);
    auto invalid=original;uint32_t type=1;std::memcpy(invalid.data()+226,&type,4);auto unchanged=invalid;assert(!c.apply(invalid.data())&&invalid==unchanged);
    invalid=original;invalid[0]=0;assert(!decode(invalid.data()).valid);invalid=original;invalid[160]=0;assert(!decode(invalid.data()).valid);
    // A game view already tilted 45 degrees must never pitch above the horizon
    // or cross the vertical pole. WASD directions must not depend on tilt.
    auto angled=frame;auto tilt=rotation(frame.right,3.141592653589793/4);
    angled.forward=direction(tilt,frame.forward);angled.up=direction(tilt,frame.up);
    Controller low,high;assert(low.enable(angled,0)&&high.enable(angled,0));
    low.rotate(0,-1000);high.rotate(0,1000);
    assert(std::abs(low.pitch+45)<1e-6&&std::abs(high.pitch-40)<1e-6);
    assert(direction(high.orient(),angled.forward).z<-.08);
    low.move(0,1,0,.1,false);high.move(0,1,0,.1,false);assert(grid::length(low.offset-high.offset)<1e-9);
    auto mirrored=angled;mirrored.right=mirrored.right*-1;
    Controller mirrored_low,mirrored_high;mirrored_low.enable(mirrored,0);mirrored_high.enable(mirrored,0);
    mirrored_low.rotate(0,-1000);mirrored_high.rotate(0,1000);
    assert(std::abs(mirrored_low.pitch+40)<1e-6&&std::abs(mirrored_high.pitch-45)<1e-6);
    assert(direction(mirrored_low.orient(),mirrored.forward).z<-.08);
    assert(direction(mirrored_high.orient(),mirrored.forward).z<-.999);
    Controller normal,x,y;normal.enable(angled,0);x=normal;y=normal;x.invert_x=true;y.invert_y=true;
    normal.mouse_rotate(10,10);x.mouse_rotate(10,10);y.mouse_rotate(10,10);
    assert(std::abs(normal.yaw+.8)<1e-9&&std::abs(normal.pitch-.8)<1e-9);
    assert(x.yaw==-normal.yaw&&x.pitch==normal.pitch);assert(y.yaw==normal.yaw&&y.pitch==-normal.pitch);
    x.sensitivity=.12;x.speed=2;x.enable(frame,0);assert(x.invert_x&&x.sensitivity==.12&&x.speed==2);
    std::cout<<"PASS: slower defaults, independent X/Y inversion, absolute tilt limits, stable ground navigation and retained preferences\n";
    std::cout<<"PASS: free camera pan, height, rotation, zoom, restore, matrix inverses, reflected camera and 49 world-grid snap points\n";
}
