#include "../src/i18n.hpp"
#include <iostream>
#include <string>
#include <stdexcept>
using namespace sketch::i18n;
static_assert(std::string_view(catalog[static_cast<size_t>(Text::Hud)].en).find("E Draw | G Cursor | B Marks | T Camera")!=std::string_view::npos&&std::string_view(catalog[static_cast<size_t>(Text::Hud)].zh).find("E绘图 G指针 B草稿 T相机")!=std::string_view::npos,"Compact HUD retains all four shortcut hints in both languages");
constexpr bool english_is_ascii(){
    for(const auto &entry:catalog)for(unsigned char ch:std::string_view(entry.en))if(ch>=128)return false;
    return true;
}
static_assert(english_is_ascii(),"Line icons are drawn separately; English labels contain plain ASCII text");
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
template<size_t N> constexpr bool valid_help(const ShortcutEntry (&rows)[N]){
    for(const auto &row:rows)if(!row.key[0]||!row.en[0]||!row.zh[0]||!same_formats(row.en,row.zh))return false;
    return true;
}
static_assert(valid_help(general_shortcuts)&&valid_help(tool_shortcuts)&&valid_help(drawing_shortcuts)&&valid_help(camera_shortcuts),"All key badges have complete bilingual descriptions and compatible formats");
constexpr bool has_shortcut(std::string_view key){
    for(const auto &row:general_shortcuts)if(key==row.key)return true;
    for(const auto &row:tool_shortcuts)if(key==row.key)return true;
    for(const auto &row:drawing_shortcuts)if(std::string_view(row.key).find(key)!=std::string_view::npos)return true;
    for(const auto &row:camera_shortcuts)if(std::string_view(row.key).find(key)!=std::string_view::npos)return true;
    return false;
}
constexpr bool all_shortcuts(){
    for(auto key:{"E","G","B","T","Esc","Alt+Tab","Alt+P","Alt+L","Alt+A","Alt+R","Alt+O","Alt+T","Alt+K","Alt+H","Alt+1-5","Shift","Ctrl+Z","Ctrl+Y","Ctrl+Shift+Z","Ctrl+1","Ctrl+2","Ctrl+3","Ctrl+S","Ctrl+Shift+Delete","Ctrl+Enter","W/A/S/D","Q / Z"})if(!has_shortcut(key))return false;
    return true;
}
static_assert(all_shortcuts(),"The rendered help tables retain all production shortcuts");
static_assert(std::string_view(shortcut_key(drawing_shortcuts[0],true))=="鼠标中键"&&std::string_view(shortcut_key(drawing_shortcuts[1],true))=="滚轮"&&std::string_view(shortcut_key(drawing_shortcuts[2],true))=="Shift + 滚轮"&&std::string_view(shortcut_key(drawing_shortcuts[4],true))=="鼠标右键"&&std::string_view(shortcut_key(camera_shortcuts[2],true))=="右键拖动","Chinese help translates mouse key badges and retains modifier names");
static_assert(std::string_view(shortcut_key(drawing_shortcuts[0],false))=="Middle click"&&std::string_view(shortcut_key(tool_shortcuts[0],true))=="Alt+P","English mouse badges and physical keyboard shortcuts remain unchanged");
constexpr bool stable_windows(){
    for(auto label:{Text::Title,Text::ShortcutsTitle,Text::SettingsTitle,Text::CameraTitle,Text::TextTitle}){
        std::string_view en=catalog[size_t(label)].en,zh=catalog[size_t(label)].zh;auto a=en.find("###"),b=zh.find("###");
        if(a==en.npos||b==zh.npos||en.substr(a)!=zh.substr(b))return false;
    }
    return true;
}
static_assert(stable_windows(),"Language changes preserve all window identities");
static_assert(std::string_view(camera_shortcuts[4].en)=="Restore game view"&&std::string_view(camera_shortcuts[4].zh)=="恢复游戏视角","T/Esc always restore; camera holding remains an explicit button action");
int main(){try{
    check(!use_chinese(Language::System,false)&&use_chinese(Language::System,true),"System language selects English or Chinese");
    check(!use_chinese(Language::English,true)&&use_chinese(Language::Chinese,false),"Explicit language overrides system locale");
    check(parse_language("en")==Language::English&&parse_language("zh-CN")==Language::Chinese&&parse_language("bad")==Language::System,"Settings codes round-trip and unknown values fall back safely");
    std::cout<<"ALL LANGUAGE TESTS PASSED\n";return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
