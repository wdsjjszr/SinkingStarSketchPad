#pragma once
#include "grid_math.hpp"
#include <vector>
#include <map>
#include <set>
#include <cstring>
#include <tuple>

namespace sketch::grid {
// Only retain rigid, upright, quarter-turn instances. This rejects skinning
// bones, particle data, vertex/index uploads and most decorative transforms.
inline bool instance_center(const float *m,V &p){
    for(unsigned i=0;i<12;++i)if(!std::isfinite(m[i])||std::abs(m[i])>1e5)return false;
    for(unsigned i:{2u,6u,8u,9u})if(std::abs(m[i])>.002)return false;
    if(std::abs(m[10]-1)>.002)return false;
    for(unsigned i:{0u,1u,4u,5u})if(std::abs(m[i]-std::round(m[i]))>.002||std::abs(m[i])>1.002)return false;
    if(std::abs(m[0]*m[0]+m[1]*m[1]-1)>.004||std::abs(m[4]*m[4]+m[5]*m[5]-1)>.004
        ||std::abs(m[0]*m[4]+m[1]*m[5])>.004)return false;
    p={m[3],m[7],m[11]};return true;
}
inline std::vector<V> instance_centers(const char *data,size_t bytes){
    std::vector<V> points;const size_t count=bytes/48,step=std::max(size_t(1),(count+8191)/8192);
    for(size_t n=0;n<count;n+=step){float m[12];std::memcpy(m,data+n*48,48);V p;if(instance_center(m,p))points.push_back(p);}
    return points;
}
struct Detection {Lattice lattice;unsigned centers=0,edges=0;std::set<std::pair<int,int>> cells;};
inline std::vector<Detection> detect_layers(const std::vector<V> &input){
    // Quantization removes repeated draw instances and tiny animation errors.
    std::map<int,std::set<std::pair<int,int>>> layers;
    for(V p:input)if(finite(p)&&std::abs(p.x)<1e5&&std::abs(p.y)<1e5&&std::abs(p.z)<1e5)
        layers[int(std::round(p.z*100))].insert({int(std::round(p.x*1000)),int(std::round(p.y*1000))});
    std::vector<Detection> detected;
    for(const auto &[height,cells]:layers){
        if(cells.size()<6)continue;
        Detection best;unsigned best_score=0;
        std::vector<std::pair<int,int>> sample;size_t index=0,step=std::max(size_t(1),(cells.size()+95)/96);
        for(auto p:cells)if(index++%step==0)sample.push_back(p);
        std::map<int,unsigned> intervals;
        // Preserve nearest neighbours before sampling. Uniform sampling alone
        // can alias a dense board into an apparent 2x or 3x larger lattice.
        for(unsigned axis=0;axis<2;++axis){
            std::vector<std::pair<int,int>> sorted;
            for(auto p:cells)sorted.push_back(axis?std::make_pair(p.second,p.first):p);
            std::sort(sorted.begin(),sorted.end());
            for(size_t n=1;n<sorted.size();++n){int delta=sorted[n].second-sorted[n-1].second;
                if(std::abs(sorted[n].first-sorted[n-1].first)<=4&&delta>=50&&delta<=100000)intervals[delta]+=4;}
        }
        for(size_t a=0;a<sample.size();++a)for(size_t b=a+1;b<sample.size();++b){
            int dx=std::abs(sample[a].first-sample[b].first),dy=std::abs(sample[a].second-sample[b].second);
            if(dy<=4&&dx>=50&&dx<=100000)++intervals[dx];
            if(dx<=4&&dy>=50&&dy<=100000)++intervals[dy];
        }
        std::vector<std::pair<int,unsigned>> ranked(intervals.begin(),intervals.end());
        std::sort(ranked.begin(),ranked.end(),[](auto a,auto b){return a.second>b.second;});if(ranked.size()>16)ranked.resize(16);
        for(auto [spacing,votes]:ranked){
            if(votes<2)continue;
            for(size_t anchor=0;anchor<std::min(sample.size(),size_t(32));++anchor){
                auto origin=sample[anchor];std::set<std::pair<int,int>> inliers;
                for(auto p:cells){double x=double(p.first-origin.first)/spacing,y=double(p.second-origin.second)/spacing;
                    if(std::abs(x-std::round(x))<=.006&&std::abs(y-std::round(y))<=.006)inliers.insert({int(std::round(x)),int(std::round(y))});}
                if(inliers.size()<6)continue;unsigned horizontal=0,vertical=0;
                for(auto p:inliers){horizontal+=inliers.count({p.first+1,p.second});vertical+=inliers.count({p.first,p.second+1});}
                if(!horizontal||!vertical||horizontal+vertical<4)continue;
                unsigned score=unsigned(inliers.size())+2*(horizontal+vertical);
                if(score<=best_score)continue;
                double size=spacing/1000.;double x=origin.first/1000.,y=origin.second/1000.;
                x-=std::round(x/size)*size;y-=std::round(y/size)*size;
                best={{true,{x,y,height/100.},{size,0,0},{0,-size,0}},unsigned(inliers.size()),horizontal+vertical,{}};best_score=score;
                for(auto [i,j]:inliers)best.cells.emplace(origin.first+i*spacing,origin.second+j*spacing);
            }
        }
        if(best.lattice.ready)detected.push_back(std::move(best));
    }
    return detected;
}
inline const Detection *best_detection(const std::vector<Detection> &layers){
    const Detection *best=nullptr;
    for(const auto &layer:layers)if(!best||layer.centers+2*layer.edges>best->centers+2*best->edges)best=&layer;
    return best;
}
inline Detection detect(const std::vector<V> &input){
    auto layers=detect_layers(input);auto best=best_detection(layers);return best?*best:Detection{};
}
}
