#pragma once
#include "native_ground.hpp"

namespace sketch::native_ground {
enum class PickResult {None,Ground,Wall};
struct Pick {PickResult result=PickResult::None;grid::V center;};
constexpr bool keep_marking(bool quick,PickResult result){return !quick&&result!=PickResult::Ground;}
constexpr bool occludes(double wall,double ground){return wall<ground-1e-6;}
constexpr bool slab(double origin,double direction,double low,double high,double &enter,double &exit){
    if(direction==0)return origin>=low&&origin<=high;
    double a=(low-origin)/direction,b=(high-origin)/direction;
    if(a>b){double swap=a;a=b;b=swap;}
    if(a>enter)enter=a;if(b<exit)exit=b;return enter<=exit;
}
inline double wall_distance(const Snapshot &ground,grid::Ray ray,double limit){
    const double infinity=std::numeric_limits<double>::infinity();
    if(ground.voxels.empty())return infinity;
    double enter=0,exit=limit;
    if(!slab(ray.origin.x,ray.direction.x,ground.wall_min.x-.5,ground.wall_max.x+.5,enter,exit)
       ||!slab(ray.origin.y,ray.direction.y,ground.wall_min.y-.5,ground.wall_max.y+.5,enter,exit)
       ||!slab(ray.origin.z,ray.direction.z,ground.wall_min.z,ground.wall_max.z+1.,enter,exit))return infinity;
    // Voxel X/Y are cell centers; Z is the bottom face. Nudge into the entry cell.
    auto start=ray.origin+ray.direction*(enter+1e-7);
    int cell[]={coordinate(start.x),coordinate(start.y),int(std::floor(start.z))};
    const double origin[]={ray.origin.x+.5,ray.origin.y+.5,ray.origin.z};
    const double direction[]={ray.direction.x,ray.direction.y,ray.direction.z};
    int step[3];double next[3],delta[3];
    for(unsigned axis=0;axis<3;++axis){
        step[axis]=(direction[axis]>0)-(direction[axis]<0);
        next[axis]=step[axis]?(cell[axis]+(step[axis]>0)-origin[axis])/direction[axis]:infinity;
        delta[axis]=step[axis]?std::abs(1/direction[axis]):infinity;
    }
    while(enter<=exit){
        if(ground.voxels.count({cell[0],cell[1],cell[2]}))return enter;
        double crossing=std::min({next[0],next[1],next[2]});
        if(!std::isfinite(crossing))break;
        for(unsigned axis=0;axis<3;++axis)if(next[axis]<=crossing+1e-9){cell[axis]+=step[axis];next[axis]+=delta[axis];}
        enter=crossing;
    }
    return infinity;
}
inline Pick pick(const Snapshot &ground,grid::Ray ray){
    if(!ground.ready||!grid::finite(ray.origin)||!grid::finite(ray.direction)||grid::length(ray.direction)<1e-8)return {};
    const Cell *best=nullptr;double distance=std::numeric_limits<double>::infinity();
    if(ray.direction.z< -1e-8)for(double z:ground.heights){
        double t=(z-ray.origin.z)/ray.direction.z;if(t<0||t>=distance)continue;
        grid::V point;if(!grid::plane(ray,z,point))continue;
        if(auto cell=cell_at(ground,coordinate(point.x),coordinate(point.y),z)){best=cell;distance=t;}
    }
    if(occludes(wall_distance(ground,ray,distance),distance))return {PickResult::Wall,{}};
    return best?Pick{PickResult::Ground,best->center}:Pick{};
}
inline Pick pick(const Snapshot &ground,const grid::Camera &camera,double x,double y){
    grid::Ray ray;if(x<0||x>=1||y<0||y>=1||!camera.ray(x,y,ray))return {};return pick(ground,ray);
}
}
