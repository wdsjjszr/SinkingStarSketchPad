#include "../src/i18n.hpp"
#include <iostream>
#include <string>
#include <stdexcept>
using namespace sketch::i18n;
static_assert(std::string_view(catalog[static_cast<size_t>(Text::Hud)].en).find("E Draw | G Cursor | B Marks | T Camera")!=std::string_view::npos&&std::string_view(catalog[static_cast<size_t>(Text::Hud)].zh).find("E绘图 G指针 B草稿 T相机")!=std::string_view::npos,"Compact HUD retains all four shortcut hints in both languages");
void check(bool ok,const char *name){if(!ok)throw std::runtime_error(name);std::cout<<"PASS: "<<name<<'\n';}
constexpr bool same_formats(std::string_view a,std::string_view b){
    while(true){
        auto i=a.find('%'),j=b.find('%');
        if(i==a.npos||j==b.npos)return i==a.npos&&j==b.npos;
        a.remove_prefix(i);b.remove_prefix(j);
        i=a.find_first_of("diuoxXfFeEgGaAcspn%",1);j=b.find_first_of("diuoxXfFeEgGaAcspn%",1);
        if(i==a.npos||j==b.npos||a.substr(0,i+1)!=b.substr(0,j+1))return false;
        a.remove_prefix(i+1);b.remove_prefix(j+1);
    }
}
constexpr bool translations_valid(){
    for(const auto &entry:catalog)if(!entry.en[0]||!entry.zh[0]||!same_formats(entry.en,entry.zh))return false;
    return true;
}
static_assert(translations_valid(),"Every English/Chinese label must be nonempty and preserve all printf formats");
int main(){try{
    check(!use_chinese(Language::System,false)&&use_chinese(Language::System,true),"System language selects English or Chinese");
    check(!use_chinese(Language::English,true)&&use_chinese(Language::Chinese,false),"Explicit language overrides system locale");
    check(parse_language("en")==Language::English&&parse_language("zh-CN")==Language::Chinese&&parse_language("bad")==Language::System,"Settings codes round-trip and unknown values fall back safely");
    for(const auto &entry:catalog){
        for(unsigned char ch:std::string(entry.en))if(ch>=128)throw std::runtime_error("Non-ASCII English translation");}
    check(std::string(get(Text::Title,false)).find("###SinkingStarSketch")!=std::string::npos&&std::string(get(Text::Title,true)).find("###SinkingStarSketch")!=std::string::npos,"Window identity stays stable when language changes");
    std::cout<<"ALL LANGUAGE TESTS PASSED\n";return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
