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
struct EntitySlot {
    uint32_t hash,padding;
    uintptr_t entity;
    constexpr bool live()const{return hash>=2&&entity!=0;}
};
static_assert(sizeof(EntitySlot)==16&&offsetof(EntitySlot,entity)==8,"Entity ID table slots have a 16-byte stride");
static_assert(!EntitySlot{0,0,1}.live()&&!EntitySlot{1,0,1}.live()&&EntitySlot{2,0,1}.live(),"Empty and deleted slots are not entities");
struct Field {size_t offset;uintptr_t type;};
class Reader {
    uintptr_t base=0,slot=0;
    std::vector<uint8_t> image;
    struct Type {std::string name;size_t size;};
    std::map<uintptr_t,Type> types;
    struct Guy {uintptr_t entity,type;uint32_t id;Field position,status,flags,destroyed;uint16_t active,observed=0;};
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
    uint16_t active_mask(uintptr_t type)const{
        if(meta<uint32_t>(type)!=11||meta<uint64_t>(type+8)!=2||name(type+16)!="Status_Flags")
            throw std::runtime_error("Unsupported character status flags.");
        const auto names=meta<Array>(type+40),values=meta<Array>(type+56);uint16_t active=0;
        if(names.count<=0||names.count>128||names.count!=values.count)throw std::runtime_error("Unsupported character selection metadata.");
        for(int64_t i=0;i<names.count;++i)if(name(names.data+size_t(i)*16)=="ACTIVE"){
            auto mask=meta<uint64_t>(values.data+size_t(i)*8);
            if(active||!mask||mask>0xffff||(mask&(mask-1)))throw std::runtime_error("Unsupported ACTIVE selection bit.");
            active=uint16_t(mask);
        }
        if(!active)throw std::runtime_error("ACTIVE selection flag is unavailable.");return active;
    }
    Guy character(uintptr_t entity,uintptr_t type)const{
        auto position=path(type,{"entity","undoable_data","position"}),status=path(type,{"entity","undoable_data","status_flags"});
        if(meta<uint64_t>(position.type+8)!=12||meta<uint32_t>(position.type)!=7||name(position.type+16)!="Vector3")
            throw std::runtime_error("Unsupported character position field.");
        const char *axes[]={"x","y","z"};
        for(unsigned i=0;i<3;++i){auto axis=field(position.type,axes[i]);if(axis.offset!=i*4||meta<uint32_t>(axis.type)!=1||meta<uint64_t>(axis.type+8)!=4)
            throw std::runtime_error("Unsupported character position axes.");}
        auto flags=path(type,{"entity","undoable_data","entity_flags"}),destroyed=path(type,{"entity","scheduled_for_destruction"});
        if(meta<uint64_t>(flags.type+8)!=8||meta<uint64_t>(destroyed.type+8)!=1)throw std::runtime_error("Unsupported character lifetime fields.");
        return {entity,type,read<uint32_t>(entity+0x100),position,status,flags,destroyed,active_mask(status.type)};
    }
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
    SceneIdentity probe(){
        SceneIdentity out;
        try{
            if(image.empty())initialize();
            if(!slot)throw std::runtime_error("Current-scene signature is unavailable for this game version.");
            out.id=read<uintptr_t>(slot);auto text=read<Array>(out.id);
            if(text.count<=0||text.count>128)throw std::runtime_error("Current scene is not ready.");
            out.name.resize(size_t(text.count));bytes(text.data,out.name.data(),out.name.size());
            auto after=read<Array>(out.id);
            if(read<uintptr_t>(slot)!=out.id||after.count!=text.count||after.data!=text.data)throw std::runtime_error("Scene changed while reading its name.");
            out.ready=true;
        }catch(const std::exception &e){out.error=e.what();}
        return out;
    }
private:
    void require_scene(const SceneIdentity &scene){
        auto current=probe();if(!current.ready||!current.same(scene))throw std::runtime_error("Scene changed while reading native data.");
    }
    static std::vector<EntitySlot> entities(uintptr_t manager){
        auto objects=read<Array>(manager+0x3c0);
        if(objects.count<=0||objects.count>262144)throw std::runtime_error("Current scene entity table is not ready.");
        std::vector<EntitySlot> out(size_t(objects.count));bytes(objects.data,out.data(),out.size()*sizeof(EntitySlot));
        auto after=read<Array>(manager+0x3c0);
        if(after.count!=objects.count||after.data!=objects.data)throw std::runtime_error("Scene entity table changed while reading.");
        return out;
    }
public:
    CharacterSample sample_character(const SceneIdentity &scene){
        CharacterSample out;
        out.scene_id=scene.id;
        try{
            require_scene(scene);std::vector<Guy> guys;
            for(auto entry:entities(scene.id)){
                if(!entry.live())continue;
                if(read<uintptr_t>(entry.entity+0xf0)!=scene.id)throw std::runtime_error("Scene changed while reading characters.");
                auto type=read<uintptr_t>(entry.entity+0x108);
                if(type_info(type).name=="Guy")guys.push_back(character(entry.entity,type));
            }
            for(auto &g:guys){
                if(read<uintptr_t>(g.entity+0xf0)!=out.scene_id||read<uintptr_t>(g.entity+0x108)!=g.type||read<uint32_t>(g.entity+0x100)!=g.id)
                    throw std::runtime_error("Character identity changed; waiting for refresh.");
                g.observed=read<uint16_t>(g.entity+g.status.offset);
                auto flags=read<uint64_t>(g.entity+g.flags.offset);
                if(read<uint8_t>(g.entity+g.destroyed.offset)||((flags&(1ull<<25))&&!(flags&(1ull<<26)))||!controlled(g.observed,g.active))continue;
                auto position=read<std::array<float,3>>(g.entity+g.position.offset);
                if(!finite_position(position))throw std::runtime_error("Invalid character position.");
                grid::V p{position[0],position[1],position[2]};
                if(!out.controlled_count||precedes(p,out.position,g.id,out.entity_id)){out.position=p;out.entity_id=g.id;}
                ++out.controlled_count;
            }
            for(const auto &g:guys)if(read<uintptr_t>(g.entity+0xf0)!=out.scene_id||read<uintptr_t>(g.entity+0x108)!=g.type
                ||read<uint32_t>(g.entity+0x100)!=g.id||controlled(read<uint16_t>(g.entity+g.status.offset),g.active)!=controlled(g.observed,g.active))
                throw std::runtime_error("Character selection changed while reading.");
            require_scene(scene);
            if(!out.controlled_count)throw std::runtime_error("No currently controlled character in this scene.");
            out.valid=true;
        }catch(const std::exception &e){out.error=e.what();}
        return out;
    }
    Snapshot sample(const SceneIdentity &identity){
        Snapshot out;out.scene_id=identity.id;out.scene=identity.name;
        try{
            require_scene(identity);
            // The ID table includes static entities that the character-oriented list at +0x370 omits.
            const auto manager=identity.id;Scene scene;bool have_settings=false;
            for(auto entry:entities(manager)){
                if(!entry.live())continue;
                auto entity=entry.entity;
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
            if(!have_settings)throw std::runtime_error("No Settings entity in this scene.");
            out.set_geometry(std::move(scene));require_scene(identity);
        }catch(const std::exception &e){out.cells.clear();out.heights.clear();out.voxels.clear();out.ready=false;out.error=e.what();}
        return out;
    }
};
inline std::mutex mutex;
inline std::condition_variable wake;
inline std::thread worker;
inline bool running=false;
enum class Read {None,Ground,Character};
struct State {
    SceneIdentity scene;
    std::shared_ptr<const Snapshot> ground;
    CharacterSample character;
    uint64_t generation=0,completed=0;
    bool initialized=false;
    double initial_z=0;
};
struct Request {uint64_t id=0,generation=0;Read kind=Read::None;};
inline State result;
inline Request pending;
inline uint64_t serial=0;
inline void start(){
    std::lock_guard<std::mutex> lock(mutex);if(running)return;running=true;
    worker=std::thread([]{
        Reader reader;std::unique_lock<std::mutex> lock(mutex);ULONGLONG next_probe=0;
        while(running){
            auto now=GetTickCount64();
            if(now<next_probe&&pending.kind==Read::None){wake.wait_for(lock,std::chrono::milliseconds(next_probe-now),[]{return !running||pending.kind!=Read::None;});continue;}
            auto request=pending;pending={};auto next=result;lock.unlock();
            auto scene=reader.probe();const bool changed=scene.ready&&(!next.generation||!scene.same(next.scene));
            if(changed){
                auto generation=next.generation+1;next={};next.generation=generation;next.scene=scene;
                next.ground=std::make_shared<const Snapshot>(reader.sample(scene));
            }else if(scene.ready)next.scene=scene;
            else{next.scene.ready=false;next.scene.error=scene.error;}
            if(next.scene.ready){
                const bool requested=request.id&&request.generation==next.generation;
                if((!next.initialized&&(changed||now>=next_probe))||(requested&&request.kind==Read::Character)){
                    next.character=reader.sample_character(next.scene);
                    if(!next.initialized&&next.character.valid){next.initial_z=next.character.position.z;next.initialized=true;}
                }
                if(requested){
                    if(request.kind==Read::Ground)next.ground=std::make_shared<const Snapshot>(reader.sample(next.scene));
                    next.completed=request.id;
                }
            }else if(request.id&&request.generation==next.generation){
                if(request.kind==Read::Character){next.character={};next.character.error=next.scene.error;}
                else{auto failed=std::make_shared<Snapshot>();failed->error=next.scene.error;next.ground=std::move(failed);}
                next.completed=request.id;
            }
            if(changed||now>=next_probe)next_probe=GetTickCount64()+1000;
            lock.lock();result=std::move(next);
        }
    });
}
inline State snapshot(){std::lock_guard<std::mutex> lock(mutex);return result;}
inline uint64_t request(Read kind,uint64_t generation){
    uint64_t id;{std::lock_guard<std::mutex> lock(mutex);id=++serial;pending={id,generation,kind};}wake.notify_all();return id;
}
inline void cancel(uint64_t id){std::lock_guard<std::mutex> lock(mutex);if(pending.id==id)pending={};}
inline void cleanup(){
    {std::lock_guard<std::mutex> lock(mutex);running=false;}wake.notify_all();if(worker.joinable())worker.join();
    std::lock_guard<std::mutex> lock(mutex);result={};pending={};serial=0;
}
}
