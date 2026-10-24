#include "../src/native_ground_pick.hpp"
#include <cassert>
using namespace sketch::native_ground;
constexpr bool viewing_ray_checks(){
    sketch::grid::Ray a,b,c;
    if(!sketch::grid::viewing_ray({0,0,9},{0,0,1},{0,0,10},{0,0,-1},a)
       ||!sketch::grid::viewing_ray({0,0,1},{0,0,9},{0,0,10},{0,0,-1},b)
       ||!sketch::grid::viewing_ray({2,3,9},{2,3,1},{0,0,10},{0,0,-1},c))return false;
    return a.origin.z==10&&b.origin.z==10&&a.direction.z<0&&b.direction.z<0&&c.origin.x==2&&c.origin.y==3&&c.origin.z==10;
}
static_assert(viewing_ray_checks(),"Perspective/reverse-depth and parallel rays start on the eye plane and face forward");
static_assert(!occludes(5,5)&&!occludes(5+1e-7,5)&&occludes(4,5),"A ground cell's own top is not an occluder; a wall in front is");
static_assert(controlled(4,4)&&controlled(12,4)&&!controlled(0,4)&&!controlled(8,4),"ACTIVE is re-evaluated independently of other status flags");
static_assert(precedes({-1,9,9},{0,0,0},20,1)&&precedes({0,-1,9},{0,0,0},20,1)
    &&precedes({0,0,-1},{0,0,0},20,1)&&precedes({0,0,0},{0,0,0},1,2),"Controlled characters sort by X, Y, Z, then entity ID");
static_assert(nearer(2,-2,2,2)&&!nearer(2,2,2,-2)&&nearer(1,8,2,0),"Distance wins first; equal distance prefers lower ground");
constexpr bool slab_checks(){
    double enter=0,exit=100;
    if(!slab(0,1,2,4,enter,exit)||enter!=2||exit!=4)return false;
    if(slab(0,0,2,4,enter,exit))return false;
    enter=0;exit=100;return slab(6,-1,2,4,enter,exit)&&enter==2&&exit==4;
}
static_assert(slab_checks(),"Ray bounds support parallel rays and either travel direction");
constexpr bool target_checks(){
    TargetState pages[3];
    if(pages[0].manual||pages[0].follow||!pages[0].needs_update())return false;
    pages[0].pending=false;if(pages[0].needs_update())return false;
    pages[0].follow=true;if(!pages[0].needs_update()||pages[1].follow)return false;
    pages[0].mode(true);if(pages[0].needs_update()||!pages[0].follow)return false;
    pages[0].scene_reset();if(!pages[0].manual||!pages[0].follow)return false;
    pages[0].mode(false);if(!pages[0].needs_update())return false;
    pages[0].follow=false;pages[0].pending=false;return !pages[0].needs_update()&&pages[1].needs_update();
}
static_assert(target_checks(),"Each page initializes automatically, freezes without following, retains preferences and resets on a new scene");
static_assert(!keep_marking(false,PickResult::Ground)&&keep_marking(false,PickResult::Wall)&&keep_marking(false,PickResult::None)
    &&!keep_marking(true,PickResult::Ground)&&!keep_marking(true,PickResult::Wall)&&!keep_marking(true,PickResult::None),
    "Left-click failures keep marking; every middle-click attempt exits");
int main(){
    Scene scene;scene.bounds={0,0,2,1,false};scene.underground_barrier=-4;scene.sky_barrier=10;
    add_wall(scene,{0,0,0},{1,1,2},{3},17);
    add_wall(scene,{0,0,4},{1,1,1},{1},18);
    auto cells=surfaces(scene);
    assert(cells.size()==3);
    assert(cells[0].center.z==2&&cells[0].entity_id==17);
    assert(cells[1].center.z==5&&cells[1].entity_id==18);
    assert(cells[2].center.x==1&&cells[2].center.z==0&&cells[2].source==Source::Base);
    scene.oceans.push_back({{0,0,2,1,false},0,{0,1}});
    cells=surfaces(scene);assert(cells.size()==2);
    scene.floors.push_back({{1,0,3},Source::Fence,19});
    cells=surfaces(scene);assert(cells.size()==3&&cells.back().source==Source::Fence);
    assert(coordinate(-1.5)==-1&&coordinate(1.5)==2);
    Snapshot ground;ground.set_geometry(scene);
    assert(nearest(ground,{0,0,3})->center.z==2);
    assert(nearest(ground,{0,0,4.9})->center.z==5);
    assert(nearest(ground,{1,0,0})->center.z==3);
    assert(nearest(ground,{2,0,3})->center.x==1);
    assert(!nearest(Snapshot{},{}));
    Scene priorities;priorities.bounds={0,0,0,0,false};
    priorities.floors={{{0,0,20},Source::Fence,1},{{1,0,0},Source::Fence,2}};
    Snapshot column;column.set_geometry(priorities);
    assert(nearest(column,{0,0,0})->center.z==20);
    priorities.floors={{{-1,0,-1},Source::Fence,1},{{1,0,1},Source::Fence,2}};
    column=Snapshot{};column.set_geometry(priorities);
    assert(nearest(column,{0,0,0})->center.z==-1);
    assert(pick(ground,{{0,0,10},{0,0,-1}}).center.z==5);
    assert(pick(ground,{{0,0,3},{0,0,-1}}).center.z==2);
    assert(pick(ground,{{-2,0,1},{1,0,0}}).result==PickResult::Wall);
    assert(pick(ground,{{0,0,-2},{0,0,1}}).result==PickResult::Wall);
    assert(pick(ground,{{4,4,10},{0,0,-1}}).result==PickResult::None);
    assert(pick(ground,{{1,0,10},{0,0,-1}}).result==PickResult::Ground);
    Scene blocking;blocking.bounds={0,0,0,0,false};add_wall(blocking,{0,0,0},{1,1,3},{7},1);
    blocking.floors={{{2,0,0},Source::Fence,2},{{-1,0,2},Source::Fence,3}};
    Snapshot occlusion;occlusion.set_geometry(blocking);
    assert(pick(occlusion,{{-2,0,2},{1,0,-.5}}).result==PickResult::Wall);
    auto front=pick(occlusion,{{-2,0,3},{1,0,-1}});
    assert(front.result==PickResult::Ground&&front.center.x==-1);
    auto g=lattice(5);assert(g.origin.x==0&&g.origin.y==0&&g.origin.z==5&&g.u.x==1&&g.v.y==-1);
}
