#pragma once
#include "native_ground.hpp"
#include <windows.h>
#include <cstring>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <thread>

namespace sketch::native_ground_bridge {
using namespace native_ground;
struct Array {int64_t count;uintptr_t data;};
struct Field {size_t offset;uintptr_t type;};
class Reader {
    uintptr_t base=0,slot=0;
    std::vector<uint8_t> image;
    struct Type {std::string name;size_t size;};
    std::map<uintptr_t,Type> types;
    static void bytes(uintptr_t at,void *out,size_t size){
        SIZE_T got=0;
        if(at<0x10000||!ReadProcessMemory(GetCurrentProcess(),reinterpret_cast<void*>(at),out,size,&got)||got!=size)
            throw std::runtime_error("Native memory read failed; wait for the scene to finish loading.");
    }
    template<class T> static T read(uintptr_t at){T out;bytes(at,&out,sizeof(out));return out;}
    template<class T> T meta(uintptr_t at)const{
        if(at<base||at-base>image.size()||sizeof(T)>image.size()-(at-base))throw std::runtime_error("Unsupported game type metadata.");
        T value;std::memcpy(&value,image.data()+(at-base),sizeof(value));return value;
    }
    std::string name(uintptr_t at)const{
        auto s=meta<Array>(at);
        if(s.count<0||s.count>128||s.data<base||s.data-base>image.size()||size_t(s.count)>image.size()-(s.data-base))
            throw std::runtime_error("Unsupported game type name.");
        return std::string(reinterpret_cast<const char*>(image.data()+(s.data-base)),size_t(s.count));
    }
    Field field(uintptr_t type,const char *wanted)const{
        if(meta<uint32_t>(type)!=7)throw std::runtime_error("Expected a game structure.");
        auto members=meta<Array>(type+64);const auto size=meta<uint64_t>(type+8);
        if(members.count<0||members.count>256)throw std::runtime_error("Unsupported game field list.");
        for(int64_t i=0;i<members.count;++i){
            auto at=members.data+size_t(i)*64;if(name(at)!=wanted)continue;
            auto child=meta<uintptr_t>(at+16);auto offset=meta<int64_t>(at+24);auto bytes=meta<uint64_t>(child+8);
            if((meta<uint32_t>(at+32)&1)||offset<0||uint64_t(offset)>size||bytes>size-uint64_t(offset))break;
            return {size_t(offset),child};
        }
        throw std::runtime_error(std::string("Unsupported game field: ")+wanted);
    }
    Field path(uintptr_t type,std::initializer_list<const char*> names)const{
        Field result{0,type};for(auto n:names){auto child=field(result.type,n);result.offset+=child.offset;result.type=child.type;}return result;
    }
    const Type &type_info(uintptr_t type){
        auto it=types.find(type);if(it!=types.end())return it->second;
        if(meta<uint32_t>(type)!=7)throw std::runtime_error("Unsupported entity type.");
        auto size=meta<uint64_t>(type+8);if(size<0x120||size>65536)throw std::runtime_error("Unsupported entity size.");
        return types.emplace(type,Type{name(type+16),size_t(size)}).first->second;
    }
    template<class T> T value(const std::vector<uint8_t> &data,Field f)const{
        if(meta<uint64_t>(f.type+8)!=sizeof(T)||f.offset>data.size()||sizeof(T)>data.size()-f.offset)
            throw std::runtime_error("Unsupported native field size.");
        T out;std::memcpy(&out,data.data()+f.offset,sizeof(out));return out;
    }
    template<class T> T value(const std::vector<uint8_t> &data,uintptr_t type,std::initializer_list<const char*> names)const{return value<T>(data,path(type,names));}
    static bool finite_position(const std::array<float,3> &p){for(auto n:p)if(!std::isfinite(n)||std::abs(n)>1000000)return false;return true;}
    Bounds bounds(const std::vector<uint8_t>&data,Field f,I3 position={})const{
        auto x=field(f.type,"squares_x"),y=field(f.type,"squares_y"),corner=field(f.type,"corner"),wrap=field(f.type,"wrapping");
        x.offset+=f.offset;y.offset+=f.offset;corner.offset+=f.offset;wrap.offset+=f.offset;
        auto width=value<int64_t>(data,x),height=value<int64_t>(data,y);auto origin=value<I3>(data,corner);
        if(width<=0||height<=0||width>4096||height>4096||width*height>1000000||std::abs(int64_t(origin.x))>1000000||std::abs(int64_t(origin.y))>1000000)
            throw std::runtime_error("Unsupported native grid bounds.");
        return {position.x+origin.x,position.y+origin.y,int(width),int(height),value<uint8_t>(data,wrap)!=0};
    }
    void initialize(){
        base=reinterpret_cast<uintptr_t>(GetModuleHandleW(nullptr));
        auto dos=read<IMAGE_DOS_HEADER>(base);auto pe=read<IMAGE_NT_HEADERS64>(base+dos.e_lfanew);
        if(dos.e_magic!=IMAGE_DOS_SIGNATURE||pe.Signature!=IMAGE_NT_SIGNATURE||pe.OptionalHeader.SizeOfImage>128*1024*1024)
            throw std::runtime_error("Unsupported game executable.");
        image.resize(pe.OptionalHeader.SizeOfImage);
        auto table=base+dos.e_lfanew+24+pe.FileHeader.SizeOfOptionalHeader;
        uintptr_t code=0;size_t length=0;
        for(unsigned i=0;i<pe.FileHeader.NumberOfSections;++i){
            auto section=read<IMAGE_SECTION_HEADER>(table+i*sizeof(IMAGE_SECTION_HEADER));
            if(std::memcmp(section.Name,".text",5)&&std::memcmp(section.Name,".rdata",6))continue;
            auto rva=section.VirtualAddress,size=section.Misc.VirtualSize;
            if(rva>image.size()||size>image.size()-rva)throw std::runtime_error("Unsupported game executable section.");
            bytes(base+rva,image.data()+rva,size);
            if(!std::memcmp(section.Name,".text",5)){code=base+rva;length=size;}
        }
        // Current-scene RIP reference used by ootss-cmd; no fixed process address.
        constexpr uint8_t signature[]={0x4c,0x8b,0x2d,0,0,0,0,0x49,0x8d,0x95,0x98,0x3d,0,0,0x4d,0x8d,0x85,0xb0,0x3d,0,0,0x31,0xc9,0xe8,0,0,0,0};
        unsigned matches=0;
        for(size_t i=0;i+sizeof(signature)<=length;++i){
            const auto *p=image.data()+code-base+i;bool match=true;
            for(size_t j=0;j<sizeof(signature);++j)if(!(j>=3&&j<7)&&j<24&&p[j]!=signature[j]){match=false;break;}
            if(!match)continue;
            int32_t displacement;std::memcpy(&displacement,p+3,4);slot=code+i+7+displacement;++matches;
        }
        if(matches!=1||slot<base||slot-base>image.size()-8){slot=0;throw std::runtime_error("Current-scene signature is unavailable for this game version.");}
    }
public:
    Snapshot sample(){
        Snapshot out;
        try{
            if(image.empty())initialize();
            if(!slot)throw std::runtime_error("Current-scene signature is unavailable for this game version.");
            auto manager=read<uintptr_t>(slot);auto objects=read<Array>(manager+0x370);auto scene_name=read<Array>(manager);
            if(objects.count<=0||objects.count>20000||scene_name.count<=0||scene_name.count>128)throw std::runtime_error("Current scene is not ready.");
            out.scene.resize(size_t(scene_name.count));bytes(scene_name.data,out.scene.data(),out.scene.size());
            std::vector<uintptr_t> entities(size_t(objects.count));bytes(objects.data,entities.data(),entities.size()*8);
            Scene scene;bool have_settings=false;
            for(auto entity:entities){
                auto owner=read<uintptr_t>(entity+0xf0);if(owner!=manager)throw std::runtime_error("Scene changed while reading entities.");
                auto type=read<uintptr_t>(entity+0x108);const auto &info=type_info(type);
                if(info.name!="Wall_Set"&&info.name!="Settings"&&info.name!="Fence"&&info.name!="Ocean_Map")continue;
                std::vector<uint8_t> data(info.size);bytes(entity,data.data(),data.size());
                auto flags=value<uint64_t>(data,type,{"entity","undoable_data","entity_flags"});
                if((flags&(1ull<<25))&&!(flags&(1ull<<26)))continue;
                if(value<uint8_t>(data,type,{"entity","scheduled_for_destruction"}))continue;
                auto position=value<std::array<float,3>>(data,type,{"entity","undoable_data","position"});
                if(!finite_position(position))throw std::runtime_error("Invalid native entity position.");
                auto id=value<uint32_t>(data,type,{"entity","entity_id"});
                if(info.name=="Settings"){
                    scene.bounds=bounds(data,path(type,{"grid"}));
                    scene.ground=value<float>(data,type,{"ground_z0"});scene.underground=value<float>(data,type,{"underground_z0"});
                    scene.underground_barrier=value<float>(data,type,{"underground_barrier"});scene.sky_barrier=value<float>(data,type,{"skyground_barrier"});
                    scene.lower=value<uint8_t>(data,type,{"has_lower_layer"})!=0;
                    if(!std::isfinite(scene.ground)||std::abs(scene.ground)>1000000||!std::isfinite(scene.underground)||std::abs(scene.underground)>1000000||!std::isfinite(scene.underground_barrier)||!std::isfinite(scene.sky_barrier))throw std::runtime_error("Invalid native layer heights.");
                    have_settings=true;
                }else if(info.name=="Wall_Set"){
                    if(value<uint8_t>(data,type,{"kind"})!=0)continue;
                    auto origin=value<I3>(data,type,{"wall_data","corner"});auto dimensions=value<I3>(data,type,{"wall_data","dimensions"});
                    if(std::abs(int64_t(origin.x))>1000000||std::abs(int64_t(origin.y))>1000000||std::abs(int64_t(origin.z))>1000000)throw std::runtime_error("Unsupported wall origin.");
                    auto slots=value<Array>(data,type,{"wall_data","bits","slots"});auto count=value<int64_t>(data,type,{"wall_data","bits","count"});
                    if(dimensions.x<0||dimensions.y<0||dimensions.z<0||dimensions.x>4096||dimensions.y>4096||dimensions.z>4096)throw std::runtime_error("Unsupported wall dimensions.");
                    auto volume=int64_t(dimensions.x)*dimensions.y*dimensions.z;
                    if(volume>2000000||count!=volume||slots.count<(count+63)/64||slots.count>31250)throw std::runtime_error("Unsupported wall voxel data.");
                    std::vector<uint64_t> bits(size_t((count+63)/64));if(!bits.empty())bytes(slots.data,bits.data(),bits.size()*8);
                    origin.x+=coordinate(position[0]);origin.y+=coordinate(position[1]);origin.z+=coordinate(position[2]);
                    add_wall(scene,origin,dimensions,bits,id);++out.wall_sets;
                    if(scene.voxels.size()>2000000)throw std::runtime_error("Native voxel preview exceeds its memory budget.");
                }else if(info.name=="Fence"){
                    if(value<uint8_t>(data,type,{"blocking"})&&value<uint8_t>(data,type,{"is_floor"})){
                        scene.floors.push_back({{double(coordinate(position[0])),double(coordinate(position[1])),position[2]},Source::Fence,id});++out.fences;
                    }
                }else{
                    Ocean ocean;auto view=value<Array>(data,type,{"ocean_values"});
                    ocean.bounds=bounds(data,path(type,{"grid"}),{coordinate(position[0]),coordinate(position[1]),0});ocean.z=position[2];
                    const auto count=size_t(ocean.bounds.width)*ocean.bounds.height;
                    if(view.count!=int64_t(count))throw std::runtime_error("Ocean grid is still loading.");
                    ocean.values.resize(count);bytes(view.data,ocean.values.data(),count);scene.oceans.push_back(std::move(ocean));++out.oceans;
                }
            }
            auto after=read<Array>(manager+0x370);
            if(read<uintptr_t>(slot)!=manager||after.count!=objects.count||after.data!=objects.data)throw std::runtime_error("Scene changed while reading ground.");
            if(!have_settings)throw std::runtime_error("No Settings entity in this scene.");
            out.voxel_count=scene.voxels.size();out.cells=surfaces(scene);for(const auto &cell:out.cells)out.heights.insert(cell.center.z);out.ready=true;
        }catch(const std::exception &e){out.cells.clear();out.heights.clear();out.error=e.what();}
        return out;
    }
};
inline std::mutex mutex;
inline std::condition_variable wake;
inline std::thread worker;
inline bool running=false,enabled=false;
inline std::shared_ptr<const Snapshot> result;
inline void start(){
    std::lock_guard<std::mutex> lock(mutex);if(running)return;running=true;
    worker=std::thread([]{
        Reader reader;std::unique_lock<std::mutex> lock(mutex);
        while(running){
            wake.wait(lock,[]{return !running||enabled;});if(!running)break;
            lock.unlock();auto next=std::make_shared<const Snapshot>(reader.sample());lock.lock();
            if(enabled)result=std::move(next);
            wake.wait_for(lock,std::chrono::seconds(1),[]{return !running||!enabled;});
        }
    });
}
inline void enable(bool value){
    {std::lock_guard<std::mutex> lock(mutex);enabled=value;if(!value)result={};}wake.notify_all();
}
inline std::shared_ptr<const Snapshot> snapshot(){std::lock_guard<std::mutex> lock(mutex);return result;}
inline void cleanup(){
    {std::lock_guard<std::mutex> lock(mutex);running=false;enabled=false;}wake.notify_all();if(worker.joinable())worker.join();
    std::lock_guard<std::mutex> lock(mutex);result={};
}
}
