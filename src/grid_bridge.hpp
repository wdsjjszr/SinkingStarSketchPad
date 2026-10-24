#pragma once
#include "grid_detect.hpp"
#include <d3d12.h>
#include <reshade.hpp>
#include <mutex>
#include <thread>
#include <condition_variable>
#include <memory>

namespace sketch::grid_bridge {
struct Region {uint64_t resource,offset;ULONGLONG tick;std::vector<grid::V> centers;};
struct Snapshot {
    std::vector<grid::Detection> layers;
    std::map<int,std::vector<grid::V>> points;
    size_t input_count;
    explicit Snapshot(const std::vector<grid::V> &input):layers(grid::detect_layers(input)),points(grid::group_heights(input)),input_count(input.size()){}
};
inline std::mutex mutex;
inline std::vector<Region> regions;
inline std::shared_ptr<const Snapshot> result;
inline std::condition_variable wake;
inline std::thread worker;
inline bool running=false;
// Observe CPU upload data already populated for the game's CopyBufferRegion.
// No GPU resource is modified, read back, or kept alive by this observer.
inline bool copied(reshade::api::command_list *cmd,reshade::api::resource source,uint64_t source_offset,
                   reshade::api::resource dest,uint64_t dest_offset,uint64_t bytes){
    if(cmd->get_device()->get_api()!=reshade::api::device_api::d3d12||!source.handle||bytes<48*8||bytes>16*1024*1024||bytes%48)return false;
    std::lock_guard<std::mutex> lock(mutex);auto tick=GetTickCount64();
    auto old=std::find_if(regions.begin(),regions.end(),[&](const Region &r){return r.resource==dest.handle&&r.offset==dest_offset;});
    if(old!=regions.end()&&tick-old->tick<500)return false;
    auto resource=reinterpret_cast<ID3D12Resource*>(source.handle);auto desc=resource->GetDesc();
    if(desc.Dimension!=D3D12_RESOURCE_DIMENSION_BUFFER||source_offset>desc.Width||bytes>desc.Width-source_offset)return false;
    D3D12_HEAP_PROPERTIES heap{};D3D12_HEAP_FLAGS flags{};
    if(FAILED(resource->GetHeapProperties(&heap,&flags))||(heap.Type!=D3D12_HEAP_TYPE_UPLOAD&&int(heap.Type)!=5))return false;
    void *data=nullptr;D3D12_RANGE range{SIZE_T(source_offset),SIZE_T(source_offset+bytes)};
    if(FAILED(resource->Map(0,&range,&data))||!data)return false;
    auto points=grid::instance_centers(static_cast<const char*>(data)+source_offset,size_t(bytes));
    D3D12_RANGE written{0,0};resource->Unmap(0,&written);
    if(old!=regions.end()){old->tick=tick;old->centers=std::move(points);}
    else if(points.size()>=4){if(regions.size()>=32)regions.erase(regions.begin());regions.push_back({dest.handle,dest_offset,tick,std::move(points)});}
    return false;
}
inline void destroyed(reshade::api::device *,reshade::api::resource resource){
    std::lock_guard<std::mutex> lock(mutex);
    regions.erase(std::remove_if(regions.begin(),regions.end(),[&](const Region &r){return r.resource==resource.handle;}),regions.end());
}
inline void start(){
    std::lock_guard<std::mutex> lock(mutex);if(running)return;running=true;
    worker=std::thread([]{
        std::unique_lock<std::mutex> lock(mutex);
        while(running){
            if(wake.wait_for(lock,std::chrono::seconds(1),[]{return !running;}))break;
            std::vector<grid::V> all;for(const auto &region:regions)all.insert(all.end(),region.centers.begin(),region.centers.end());
            lock.unlock();std::shared_ptr<const Snapshot> detected;
            try{detected=std::make_shared<const Snapshot>(all);}catch(...){}
            lock.lock();if(detected)result=std::move(detected);
        }
    });
}
inline std::shared_ptr<const Snapshot> snapshot(){std::lock_guard<std::mutex> lock(mutex);return result;}
inline void cleanup(){
    {std::lock_guard<std::mutex> lock(mutex);running=false;}wake.notify_all();if(worker.joinable())worker.join();
    std::lock_guard<std::mutex> lock(mutex);regions.clear();result={};
}
}
