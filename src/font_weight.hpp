#pragma once
#include <cstddef>
#include <cstdint>

namespace sketch::ui {
constexpr unsigned font_u16(const unsigned char *p){return unsigned(p[0])*256+p[1];}
constexpr uint32_t font_u32(const unsigned char *p){return uint32_t(p[0])<<24|uint32_t(p[1])<<16|uint32_t(p[2])<<8|p[3];}
constexpr unsigned font_weight(const unsigned char *data,size_t size){
    if(!data||size<12)return 0;
    auto version=font_u32(data);
    if(version!=0x4f54544f&&version!=0x00010000)return 0;
    unsigned tables=font_u16(data+4);if(tables>(size-12)/16)return 0;
    for(unsigned i=0;i<tables;++i){
        auto table=data+12+i*16;if(font_u32(table)!=0x4f532f32)continue;
        size_t offset=font_u32(table+8),length=font_u32(table+12);
        if(offset>size||length<6||length>size-offset)return 0;
        return font_u16(data+offset+4);
    }
    return 0;
}
}
