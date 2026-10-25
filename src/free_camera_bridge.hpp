#pragma once
#include "free_camera.hpp"
#include <d3d12.h>
#include <reshade.hpp>
#include <map>
#include <memory>
#include <mutex>
#include <vector>
#include <set>

// Override validated camera constants referenced by the submitted command
// list, before submission. Never scan/write arbitrary game memory or patch
// old camera buffers just because they were observed in an earlier frame.
namespace sketch::free_camera_bridge {
namespace api=reshade::api;
struct Key {uint64_t heap;uint32_t index;bool operator<(const Key&b)const{return heap<b.heap||(heap==b.heap&&index<b.index);}};
struct Buffer {api::resource resource;uint64_t offset,size;};
struct Held {std::shared_ptr<ID3D12Resource> resource;uint64_t offset,size;uint32_t rank=1;};
struct Copy {std::shared_ptr<ID3D12Resource> source,dest;uint64_t from,to,size;};
struct Slot {uint32_t param,binding;bool table;};
// Preserve the engine's bytes: cached camera buffers may be submitted again
// without a CPU rewrite. Reapply from that baseline, never from our own output.
struct Stamp {std::array<float,240> original,output;};
inline std::recursive_mutex mutex;
inline std::map<Key,Buffer> descriptors;
inline std::map<uint64_t,std::vector<Slot>> layouts;
inline std::map<api::command_list*,std::vector<Held>> commands;
inline std::map<api::command_list*,std::set<Key>> command_tables;
inline std::map<api::command_list*,std::vector<Copy>> command_copies;
inline std::map<std::pair<uint64_t,uint64_t>,Stamp> stamps;
inline free_camera::Controller control;
inline free_camera::Frame source;
inline free_camera::Frame effective;
inline uint32_t source_rank=0;
inline ULONGLONG source_tick=0,patch_tick=0;
inline uint64_t patched=0;
inline uint32_t viewport_width=0,viewport_height=0;
inline void viewport(uint32_t w,uint32_t h){std::lock_guard<std::recursive_mutex> lock(mutex);viewport_width=w;viewport_height=h;}
inline Key key(api::device*d,api::descriptor_table table,uint32_t binding,uint32_t array=0){api::descriptor_heap heap{};uint32_t offset=0;d->get_descriptor_heap_offset(table,binding,array,&heap,&offset);return {heap.handle,offset};}
inline void layout_init(api::device*d,uint32_t count,const api::pipeline_layout_param*params,api::pipeline_layout layout){
    if(d->get_api()!=api::device_api::d3d12)return;std::lock_guard<std::recursive_mutex> lock(mutex);auto &slots=layouts[layout.handle];slots.clear();
    auto add=[&](uint32_t i,const api::descriptor_range&r,bool table){if(r.type==api::descriptor_type::constant_buffer&&r.dx_register_space==0&&r.dx_register_index<=2&&r.count>2-r.dx_register_index)slots.push_back({i,r.binding+2-r.dx_register_index,table});};
    for(uint32_t i=0;i<count;++i){auto&p=params[i];switch(p.type){
        case api::pipeline_layout_param_type::push_descriptors:add(i,p.push_descriptors,false);break;
        case api::pipeline_layout_param_type::descriptor_table:case api::pipeline_layout_param_type::push_descriptors_with_ranges:
            for(uint32_t j=0;j<p.descriptor_table.count;++j)add(i,p.descriptor_table.ranges[j],p.type==api::pipeline_layout_param_type::descriptor_table);break;
        case api::pipeline_layout_param_type::descriptor_table_with_flags:case api::pipeline_layout_param_type::push_descriptors_with_ranges_and_flags:
            for(uint32_t j=0;j<p.descriptor_table_with_flags.count;++j)add(i,p.descriptor_table_with_flags.ranges[j],p.type==api::pipeline_layout_param_type::descriptor_table_with_flags);break;
        default:break;
    }}
}
inline void layout_destroy(api::device*,api::pipeline_layout layout){std::lock_guard<std::recursive_mutex> lock(mutex);layouts.erase(layout.handle);}
inline bool updated(api::device*d,uint32_t count,const api::descriptor_table_update*updates){
    if(d->get_api()!=api::device_api::d3d12)return false;std::lock_guard<std::recursive_mutex> lock(mutex);
    for(uint32_t n=0;n<count;++n){auto&u=updates[n];if(u.count>4096)continue;
        for(uint32_t i=0;i<u.count;++i){auto k=key(d,u.table,u.binding+i,u.array_offset);descriptors.erase(k);
            if(u.type!=api::descriptor_type::constant_buffer)continue;auto&r=static_cast<const api::buffer_range*>(u.descriptors)[i];
            if(r.buffer.handle)descriptors[k]={r.buffer,r.offset,r.size};
        }}
    if(descriptors.size()>16384)descriptors.clear();return false;
}
inline bool copied(api::device*d,uint32_t count,const api::descriptor_table_copy*copies){
    if(d->get_api()!=api::device_api::d3d12)return false;std::lock_guard<std::recursive_mutex> lock(mutex);
    for(uint32_t n=0;n<count;++n){auto&c=copies[n];if(!c.count)continue;
        auto from=key(d,c.source_table,c.source_binding,c.source_array_offset),to=key(d,c.dest_table,c.dest_binding,c.dest_array_offset);
        std::vector<std::pair<Key,Buffer>> found;
        for(auto it=descriptors.lower_bound(from);it!=descriptors.end()&&it->first.heap==from.heap&&uint64_t(it->first.index)-from.index<c.count;++it){
            uint64_t index=uint64_t(to.index)+it->first.index-from.index;if(index<=UINT32_MAX)found.push_back({{to.heap,uint32_t(index)},it->second});
        }
        for(auto it=descriptors.lower_bound(to);it!=descriptors.end()&&it->first.heap==to.heap&&uint64_t(it->first.index)-to.index<c.count;)it=descriptors.erase(it);
        for(auto&v:found)descriptors[v.first]=v.second;
    }return false;
}
inline void remember(api::command_list*cmd,Buffer b){
    auto &list=commands[cmd];for(auto&x:list)if(x.resource.get()==reinterpret_cast<ID3D12Resource*>(b.resource.handle)&&x.offset==b.offset)return;
    if(list.size()>=128)return;auto*r=reinterpret_cast<ID3D12Resource*>(b.resource.handle);r->AddRef();list.push_back({std::shared_ptr<ID3D12Resource>(r,[](ID3D12Resource*r){r->Release();}),b.offset,b.size});
}
inline void bound(api::command_list*cmd,api::shader_stage,api::pipeline_layout layout,uint32_t first,uint32_t count,const api::descriptor_table*tables,uint32_t,const uint32_t*){
    auto*d=cmd->get_device();if(d->get_api()!=api::device_api::d3d12)return;std::lock_guard<std::recursive_mutex> lock(mutex);
    // Descriptor tables hold addresses, not snapshots of descriptors. Games may
    // fill/copy the descriptors after recording the bind (or even the draw).
    auto found=layouts.find(layout.handle);if(found==layouts.end())return;for(auto s:found->second)if(s.table&&s.param>=first&&s.param-first<count)command_tables[cmd].insert(key(d,tables[s.param-first],s.binding));
}
inline void pushed(api::command_list*cmd,api::shader_stage,api::pipeline_layout layout,uint32_t param,const api::descriptor_table_update&u){
    if(cmd->get_device()->get_api()!=api::device_api::d3d12||u.type!=api::descriptor_type::constant_buffer)return;std::lock_guard<std::recursive_mutex> lock(mutex);
    auto found=layouts.find(layout.handle);if(found==layouts.end())return;for(auto s:found->second)if(!s.table&&s.param==param&&s.binding>=u.binding&&s.binding-u.binding<u.count){auto&r=static_cast<const api::buffer_range*>(u.descriptors)[s.binding-u.binding];if(r.buffer.handle)remember(cmd,{r.buffer,r.offset,r.size});}
}
inline void reset(api::command_list*cmd){std::lock_guard<std::recursive_mutex> lock(mutex);commands.erase(cmd);command_tables.erase(cmd);command_copies.erase(cmd);}
inline bool buffer_copy(api::command_list*cmd,api::resource src,uint64_t from,api::resource dst,uint64_t to,uint64_t size){
    if(cmd->get_device()->get_api()!=api::device_api::d3d12||!src.handle||!dst.handle||size<960)return false;
    auto*s=reinterpret_cast<ID3D12Resource*>(src.handle),*d=reinterpret_cast<ID3D12Resource*>(dst.handle);
    D3D12_HEAP_PROPERTIES heap{};D3D12_HEAP_FLAGS flags{};if(FAILED(s->GetHeapProperties(&heap,&flags))||(heap.Type!=D3D12_HEAP_TYPE_UPLOAD&&int(heap.Type)!=5))return false;
    auto sd=s->GetDesc(),dd=d->GetDesc();if(sd.Dimension!=D3D12_RESOURCE_DIMENSION_BUFFER||dd.Dimension!=D3D12_RESOURCE_DIMENSION_BUFFER||from>sd.Width||to>dd.Width||size>sd.Width-from||size>dd.Width-to)return false;
    std::lock_guard<std::recursive_mutex> lock(mutex);auto&list=command_copies[cmd];if(list.size()>=1024)return false;
    s->AddRef();d->AddRef();auto release=[](ID3D12Resource*r){r->Release();};list.push_back({{s,release},{d,release},from,to,size});return false;
}
inline bool resource_copy(api::command_list*cmd,api::resource src,api::resource dst){if(!src.handle)return false;auto d=reinterpret_cast<ID3D12Resource*>(src.handle)->GetDesc();if(d.Dimension==D3D12_RESOURCE_DIMENSION_BUFFER)return buffer_copy(cmd,src,0,dst,0,d.Width);return false;}
inline void resource_destroy(api::device*,api::resource r){std::lock_guard<std::recursive_mutex> lock(mutex);for(auto it=descriptors.begin();it!=descriptors.end();)if(it->second.resource.handle==r.handle)it=descriptors.erase(it);else++it;for(auto it=stamps.begin();it!=stamps.end();)if(it->first.first==r.handle)it=stamps.erase(it);else++it;}
inline void execute(api::command_queue*queue,api::command_list*cmd){
    if(queue->get_device()->get_api()!=api::device_api::d3d12)return;std::lock_guard<std::recursive_mutex> lock(mutex);
    auto direct=commands.find(cmd);std::vector<Held> submitted=direct==commands.end()?std::vector<Held>{}:direct->second;
    auto tables=command_tables.find(cmd);
    if(tables!=command_tables.end())for(auto k:tables->second){auto descriptor=descriptors.find(k);if(descriptor==descriptors.end()){continue;}
        auto b=descriptor->second;auto*r=reinterpret_cast<ID3D12Resource*>(b.resource.handle);
        if(std::any_of(submitted.begin(),submitted.end(),[&](const Held&h){return h.resource.get()==r&&h.offset==b.offset;}))continue;
        r->AddRef();submitted.push_back({std::shared_ptr<ID3D12Resource>(r,[](ID3D12Resource*r){r->Release();}),b.offset,b.size});
    }
    // A DEFAULT-heap CBV cannot be mapped. Follow only recorded CPU-upload
    // copies covering a known CBV's validated camera prefix, before submission.
    auto copies=command_copies.find(cmd);
    if(copies!=command_copies.end())for(auto&copy:copies->second)for(auto&[key,b]:descriptors){
        if(b.resource.handle!=reinterpret_cast<uint64_t>(copy.dest.get())||b.size<960||b.offset<copy.to||b.offset-copy.to>copy.size-960)continue;
        uint64_t offset=copy.from+b.offset-copy.to;
        if(std::any_of(submitted.begin(),submitted.end(),[&](const Held&h){return h.resource.get()==copy.source.get()&&h.offset==offset;}))continue;
        submitted.push_back({copy.source,offset,std::min(b.size,copy.size-(b.offset-copy.to)),2});
    }
    for(auto&b:submitted){auto*r=b.resource.get();auto desc=r->GetDesc();D3D12_HEAP_PROPERTIES heap{};D3D12_HEAP_FLAGS flags{};
        if(b.size<960||desc.Dimension!=D3D12_RESOURCE_DIMENSION_BUFFER||b.offset>desc.Width||desc.Width-b.offset<960||FAILED(r->GetHeapProperties(&heap,&flags)))continue;
        if(heap.Type!=D3D12_HEAP_TYPE_UPLOAD&&int(heap.Type)!=5)continue;
        void*data=nullptr;D3D12_RANGE read{SIZE_T(b.offset),SIZE_T(b.offset+960)};if(FAILED(r->Map(0,&read,&data))||!data)continue;
        auto*p=reinterpret_cast<float*>(static_cast<char*>(data)+b.offset);std::array<float,240> original;std::memcpy(original.data(),p,960);
        auto stamp_key=std::make_pair(reinterpret_cast<uint64_t>(r),b.offset);auto stamp=stamps.find(stamp_key);bool wrote=false;
        const bool cached=stamp!=stamps.end()&&std::memcmp(original.data(),stamp->second.output.data(),960)==0;
        if(cached)original=stamp->second.original;
        {auto frame=free_camera::decode(original.data());
            if(frame.valid){
                if((!viewport_width||(frame.width==viewport_width&&frame.height==viewport_height))&&(!source.valid||(b.rank>=source_rank&&int32_t(frame.frame-source.frame)>=0)||GetTickCount64()-source_tick>=2000)){
                    if(control.active_view()&&source.valid&&(grid::length(source.eye-frame.eye)>150||grid::dot(source.forward,frame.forward)<.5))control.restore();
                    source=frame;source_rank=b.rank;source_tick=GetTickCount64();effective=frame;
                }
                auto output=original;const bool same_view=source.valid&&grid::length(source.eye-frame.eye)<.1&&grid::dot(source.forward,frame.forward)>.999;
                if(same_view&&(stamp!=stamps.end()||stamps.size()<4096)&&control.apply(output.data())){std::memcpy(p,output.data(),960);stamps[stamp_key]={original,output};wrote=true;patch_tick=GetTickCount64();++patched;if(b.rank>=source_rank&&frame.width==viewport_width&&frame.height==viewport_height)effective=free_camera::decode(output.data());}
                else if(cached){std::memcpy(p,original.data(),960);stamps.erase(stamp_key);wrote=true;}
            }}
        D3D12_RANGE write=wrote?D3D12_RANGE{SIZE_T(b.offset),SIZE_T(b.offset+960)}:D3D12_RANGE{0,0};r->Unmap(0,&write);
    }
}
struct State {free_camera::Controller camera;bool ready=false;ULONGLONG last_patch=0;uint64_t patches=0;};
inline State snapshot(){std::lock_guard<std::recursive_mutex> lock(mutex);return {control,source.valid&&GetTickCount64()-source_tick<2000,patch_tick,patched};}
inline bool enable(bool value,double plane_z){std::lock_guard<std::recursive_mutex> lock(mutex);if(!value){control.restore();return true;}if(!source.valid||GetTickCount64()-source_tick>=2000)return false;if(control.holding){control.enabled=true;control.holding=false;return true;}return control.enable(source,plane_z);}
inline void hold(){std::lock_guard<std::recursive_mutex> lock(mutex);control.hold();}
inline void restore(){std::lock_guard<std::recursive_mutex> lock(mutex);control.restore();}
inline bool reset_view(double plane_z){std::lock_guard<std::recursive_mutex> lock(mutex);if(!source.valid||GetTickCount64()-source_tick>=2000)return false;return control.enable(source,plane_z);}
inline void change(const free_camera::Controller&value){std::lock_guard<std::recursive_mutex> lock(mutex);if(control.enabled)control=value;}
inline void preferences(double speed,double sensitivity,bool invert_x,bool invert_y){std::lock_guard<std::recursive_mutex> lock(mutex);control.speed=std::clamp(speed,.1,30.);control.sensitivity=std::clamp(sensitivity,.01,.3);control.invert_x=invert_x;control.invert_y=invert_y;}
inline grid::Camera projection(uint32_t width,uint32_t height){std::lock_guard<std::recursive_mutex> lock(mutex);if(!effective.valid||effective.width!=width||effective.height!=height||GetTickCount64()-source_tick>=2000)return {};return {effective.vp,effective.inv,effective.frame,true,effective.eye,effective.forward};}
inline void cleanup(){std::lock_guard<std::recursive_mutex> lock(mutex);commands.clear();command_tables.clear();command_copies.clear();source_rank=0;effective={};descriptors.clear();layouts.clear();stamps.clear();control={};source={};source_tick=patch_tick=patched=0;viewport_width=viewport_height=0;}
}
