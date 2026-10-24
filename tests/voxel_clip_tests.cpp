#include "../src/voxel_clip.hpp"
#include <cassert>
using namespace sketch::grid;
static_assert(!in_front(2,2,1)&&!in_front(2.00005,2,1)&&in_front(2.001,2,1)&&in_front(1.999,2,-1),"World-space contact tolerance keeps coplanar surfaces stable on either viewing side");
static_assert(interpolate({0,0,0},{4,2,1},.25).alpha==.25,"Clipped vertices interpolate opacity");
int main(){
    PaintPolygon subject{{0,0,0},{10,0,1},{10,10,1},{0,10,0}},cover{{5,-1,1},{11,-1,1},{11,11,1},{5,11,1}};
    auto pieces=subtract(subject,cover);double remainder=0;for(auto p:pieces)remainder+=std::abs(area(p));assert(std::abs(remainder-50)<1e-6);
    assert(subtract(subject,{{-1,-1,1},{11,-1,1},{11,11,1},{-1,11,1}}).empty());
    ScreenCuts cuts;cuts.reset(100,100);cuts.add(cover);cuts.add(cover);PreviewMesh mesh;mesh.polygon(subject,255);auto cropped=cuts.cut(mesh);
    double total=0;for(auto t:cropped.triangles){total+=std::abs(edge(t.points[0],t.points[1],t.points[2]))*.5;for(auto p:t.points)assert(p.alpha<=.500001);}assert(std::abs(total-50)<1e-6);
    auto merged=merge_faces({{0,0},{0,1},{1,0},{1,1}});assert(merged.size()==1&&merged[0][2]==2&&merged[0][3]==2);
    Camera camera;camera.valid=true;camera.eye={0,0,10};camera.forward={0,0,-1};
    camera.matrix={.1,0,0,0,0,.1,0,0,0,0,-.1,1,0,0,0,1};camera.inverse={10,0,0,0,0,10,0,0,0,0,-10,10,0,0,0,1};
    sketch::native_ground::Scene scene;sketch::native_ground::add_wall(scene,{0,0,0},{1,1,2},{3},1);
    auto terrain=std::make_shared<sketch::native_ground::Snapshot>();terrain->set_geometry(scene);assert(exposed_faces(*terrain).size()==6);
    Lattice plane{true,{0,0,2},{1,0,0},{0,-1,0}};Preview style{true,true,true,true,true,45};OcclusionCache cache;
    assert(cache.update(camera,plane,terrain,-64,-64,1000,1000,style)&&cache.cuts.polygons.empty());
    camera.frame++;assert(!cache.update(camera,plane,terrain,-64,-64,1000,1000,style));
    camera.matrix[3]+=1e-9;camera.inverse[3]-=1e-8;assert(cache.update(camera,plane,terrain,-64,-64,1000,1000,style)&&cache.cuts.polygons.empty());
    plane.origin.z=0;assert(cache.update(camera,plane,terrain,-64,-64,1000,1000,style)&&!cache.cuts.polygons.empty());
    auto reverse=camera;reverse.matrix[10]=.1;reverse.matrix[11]=0;reverse.inverse[10]=10;reverse.inverse[11]=0;assert(cache.update(reverse,plane,terrain,-64,-64,1000,1000,style)&&!cache.cuts.polygons.empty());
    assert(cache.update(camera,plane,nullptr,-64,-64,1000,1000,style)&&cache.cuts.polygons.empty()&&!cache.mesh.triangles.empty());
    style.centers=false;assert(cache.update(camera,plane,nullptr,-64,-64,1000,1000,style));
}
