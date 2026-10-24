#pragma once
#include "grid_math.hpp"
#include <cstdint>
#include <map>
#include <set>
#include <string>
#include <tuple>
#include <vector>

namespace sketch::native_ground {
struct I3 {
    int x=0,y=0,z=0;
    bool operator<(const I3 &b)const{return std::tie(x,y,z)<std::tie(b.x,b.y,b.z);}
};
constexpr size_t bit_index(int x,int y,int z,int width,int height){return size_t(x)+size_t(width)*(size_t(y)+size_t(height)*size_t(z));}
static_assert(bit_index(2,1,1,4,3)==18,"Wall_Data stores X first, then Y, then Z");
inline int coordinate(double value){return int(std::floor(value+.5));}
enum class Source {Voxel,Base,Fence};
struct Cell {grid::V center;Source source;uint32_t entity_id;};
struct Bounds {
    int x=0,y=0,width=0,height=0;
    bool wrapping=false;
    bool local(int &i,int &j)const{
        i-=x;j-=y;
        if(wrapping){i=(i%width+width)%width;j=(j%height+height)%height;}
        return i>=0&&j>=0&&i<width&&j<height;
    }
};
struct Ocean {Bounds bounds;double z;std::vector<uint8_t> values;};
struct Scene {
    Bounds bounds;
    double ground=0,underground=0,underground_barrier=0,sky_barrier=0;
    bool lower=false;
    std::map<I3,uint32_t> voxels;
    std::vector<Cell> floors;
    std::vector<Ocean> oceans;
    int layer(double z)const{return z<underground_barrier?1:z>=sky_barrier?2:0;}
    bool water(int x,int y,double z)const{
        for(const auto &o:oceans){
            if(layer(o.z)!=layer(z)||o.z>z+2.1)continue;
            int i=x,j=y;if(o.bounds.local(i,j)&&(o.values[size_t(i)+size_t(j)*o.bounds.width]&1))return true;
        }
        return false;
    }
};
inline bool bit(const std::vector<uint64_t> &slots,size_t index){return (slots[index/64]>>(index%64))&1;}
inline void add_wall(Scene &scene,I3 origin,I3 dimensions,const std::vector<uint64_t> &slots,uint32_t id){
    for(int z=0;z<dimensions.z;++z)for(int y=0;y<dimensions.y;++y)for(int x=0;x<dimensions.x;++x)
        if(bit(slots,bit_index(x,y,z,dimensions.x,dimensions.y)))scene.voxels[{origin.x+x,origin.y+y,origin.z+z}]=id;
}
inline std::vector<Cell> surfaces(const Scene &scene){
    std::map<std::tuple<double,double,double>,Cell> cells;
    auto put=[&](Cell cell){cells[{cell.center.x,cell.center.y,cell.center.z}]=cell;};
    // An implicit layer floor is a logical reference surface, not a terrain mesh footprint.
    auto base=[&](double z){
        for(int y=scene.bounds.y;y<scene.bounds.y+scene.bounds.height;++y)
            for(int x=scene.bounds.x;x<scene.bounds.x+scene.bounds.width;++x)
                if(!scene.voxels.count({x,y,coordinate(z)})&&!scene.water(x,y,z))put({{double(x),double(y),z},Source::Base,0});
    };
    base(scene.ground);if(scene.lower)base(scene.underground);
    for(const auto &[p,id]:scene.voxels)if(!scene.voxels.count({p.x,p.y,p.z+1}))
        put({{double(p.x),double(p.y),double(p.z+1)},Source::Voxel,id});
    for(auto cell:scene.floors)if(!scene.voxels.count({coordinate(cell.center.x),coordinate(cell.center.y),coordinate(cell.center.z)}))put(cell);
    std::vector<Cell> out;out.reserve(cells.size());for(const auto &[key,cell]:cells)out.push_back(cell);return out;
}
struct Snapshot {
    std::string scene,error;
    std::vector<Cell> cells;
    std::set<double> heights;
    size_t wall_sets=0,fences=0,oceans=0,voxel_count=0;
    bool ready=false;
};
}
