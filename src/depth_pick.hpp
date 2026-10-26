#pragma once
#include "grid_math.hpp"

namespace sketch::grid {
inline bool depth_world(const Camera &camera,const std::array<float,4> &sample,V &world){
    for(float value:sample)if(!std::isfinite(value))return false;
    if(sample[3]<.5f||sample[0]<=0||sample[0]>=1||sample[1]<0||sample[1]>=1||sample[2]<0||sample[2]>=1)return false;
    return camera.unproject(sample[1],sample[2],sample[0],world);
}
inline Lattice surface_grid(V point,const Lattice &reference){
    auto g=reference.ready&&reference.valid()?reference:Lattice{true,{0,0,0},{1,0,0},{0,-1,0}};
    g.origin.z=point.z;return g;
}
}
