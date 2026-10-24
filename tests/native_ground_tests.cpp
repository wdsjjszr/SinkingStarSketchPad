#include "../src/native_ground.hpp"
#include <cassert>
using namespace sketch::native_ground;
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
}
