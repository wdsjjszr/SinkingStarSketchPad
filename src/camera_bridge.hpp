#pragma once
#include <d3d12.h>
#include <reshade.hpp>
#include <mutex>
#include <vector>
#include <cstring>
#include "grid_math.hpp"

// The camera prefix is 960 bytes; the containing CBV can be padded or include
// additional fields. Observe descriptors, then read upload memory
// at the overlay callback, after the game has populated the buffer.
namespace sketch::camera_bridge {
struct Buffer {ID3D12Resource *resource;uint64_t offset;};
inline std::mutex mutex;
inline std::vector<Buffer> buffers;
inline bool updated(reshade::api::device *device,uint32_t count,const reshade::api::descriptor_table_update *updates){
    if(device->get_api()!=reshade::api::device_api::d3d12)return false;
    std::lock_guard<std::mutex> lock(mutex);
    for(unsigned k=0;k<count;++k){
        const auto &u=updates[k];if(u.type!=reshade::api::descriptor_type::constant_buffer)continue;
        auto ranges=static_cast<const reshade::api::buffer_range*>(u.descriptors);
        for(unsigned i=0;i<u.count;++i){
            if(ranges[i].size<960||ranges[i].size>65536||!ranges[i].buffer.handle)continue;
            auto r=reinterpret_cast<ID3D12Resource*>(ranges[i].buffer.handle);
            auto at=std::find_if(buffers.begin(),buffers.end(),[&](const Buffer &b){return b.resource==r&&b.offset==ranges[i].offset;});
            if(at!=buffers.end()){auto b=*at;buffers.erase(at);buffers.push_back(b);continue;}
            auto d=r->GetDesc();D3D12_HEAP_PROPERTIES heap{};D3D12_HEAP_FLAGS flags{};
            if(d.Dimension!=D3D12_RESOURCE_DIMENSION_BUFFER||ranges[i].offset>d.Width||d.Width-ranges[i].offset<960
                ||FAILED(r->GetHeapProperties(&heap,&flags))||(heap.Type!=D3D12_HEAP_TYPE_UPLOAD&&int(heap.Type)!=5))continue;
            r->AddRef();buffers.push_back({r,ranges[i].offset});
            if(buffers.size()>192){buffers.front().resource->Release();buffers.erase(buffers.begin());}
        }
    }
    return false;
}
inline bool decode(const float *p,double width,double height,grid::Camera &c){
    for(unsigned i=0;i<32;++i)if(!std::isfinite(p[i]))return false;
    for(unsigned i=208;i<219;++i)if(!std::isfinite(p[i]))return false;
    double norm=p[212]*p[212]+p[213]*p[213]+p[214]*p[214];
    if(norm<.98||norm>1.02||p[216]<=0||p[217]<=0||std::abs(1.0/p[216]-width)>4||std::abs(1.0/p[217]-height)>4)return false;
    double error=0;
    for(unsigned r=0;r<4;++r)for(unsigned col=0;col<4;++col){
        double sum=0;for(unsigned j=0;j<4;++j)sum+=double(p[r*4+j])*p[16+j*4+col];
        error=std::max(error,std::abs(sum-(r==col?1:0)));
    }
    if(error>.03)return false;
    for(unsigned i=0;i<16;++i){c.matrix[i]=p[i];c.inverse[i]=p[16+i];}
    c.eye={p[208],p[209],p[210]};c.forward={p[212],p[213],p[214]};
    std::memcpy(&c.frame,p+225,4);c.valid=true;return true;
}
inline grid::Camera snapshot(double width,double height){
    std::lock_guard<std::mutex> lock(mutex);
    grid::Camera best;
    for(const auto &b:buffers){
        void *data=nullptr;D3D12_RANGE read{SIZE_T(b.offset),SIZE_T(b.offset+960)};
        if(FAILED(b.resource->Map(0,&read,&data))||!data)continue;
        float p[240];std::memcpy(p,static_cast<const char*>(data)+b.offset,sizeof(p));
        D3D12_RANGE written{0,0};b.resource->Unmap(0,&written);
        grid::Camera candidate;
        if(decode(p,width,height,candidate)&&(!best.valid||int32_t(candidate.frame-best.frame)>=0))best=candidate;
    }
    return best;
}
inline void cleanup(){std::lock_guard<std::mutex> lock(mutex);for(auto &b:buffers)b.resource->Release();buffers.clear();}
}
