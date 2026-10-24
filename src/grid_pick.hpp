#pragma once
#include "grid_detect.hpp"

namespace sketch::grid {
struct PlaneHit {size_t layer;V center;double depth;};
inline std::vector<PlaneHit> pick_planes(const std::vector<Detection> &layers,const Camera &camera,double x,double y){
    std::vector<PlaneHit> hits;Ray ray;
    if(x<0||x>=1||y<0||y>=1||!camera.ray(x,y,ray))return hits;
    for(size_t n=0;n<layers.size();++n){
        const auto &layer=layers[n];const auto &g=layer.lattice;double i,j;V point;
        if(!g.snap(camera,x,y,i,j)||!plane(ray,g.origin.z,point))continue;
        V center=g.world(i,j);double sx,sy;
        if(!layer.cells.count({int(std::round(center.x*1000)),int(std::round(center.y*1000))})||!camera.project(point,sx,sy))continue;
        double depth=camera.matrix[12]*point.x+camera.matrix[13]*point.y+camera.matrix[14]*point.z+camera.matrix[15];
        hits.push_back({n,center,depth});
    }
    // shortcut: candidate cells do not include wall occlusion; add scene depth when accurate visible-surface picking is needed.
    std::stable_sort(hits.begin(),hits.end(),[](const PlaneHit &a,const PlaneHit &b){return a.depth<b.depth;});
    return hits;
}
}
