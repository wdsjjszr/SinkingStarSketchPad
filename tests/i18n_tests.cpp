#include "../src/i18n.hpp"
#include <iostream>
#include <string>
#include <stdexcept>
using namespace sketch::i18n;
static_assert(std::string_view(catalog[static_cast<size_t>(Text::Hud)].en).find("E Draw | G Cursor | B Marks | T Camera")!=std::string_view::npos&&std::string_view(catalog[static_cast<size_t>(Text::Hud)].zh).find("E绘图 G指针 B草稿 T相机")!=std::string_view::npos,"Compact HUD retains all four shortcut hints in both languages");
constexpr bool valid_control_icons(){
    constexpr Text controls[]={Text::Pen,Text::Line,Text::Arrow,Text::Rectangle,Text::Ellipse,Text::Text,Text::Eraser,Text::Path,Text::Undo,Text::Redo,Text::Clear,Text::Save,Text::Export,Text::Return,Text::CameraToggle};
    for(size_t i=0;i<catalog.size();++i){
        bool has_icon=false;for(Text control:controls)if(i==static_cast<size_t>(control))has_icon=true;
        std::string_view en=catalog[i].en,zh=catalog[i].zh;
        if(has_icon){
            if(en.size()<5||zh.size()<5||en.substr(0,4)!=zh.substr(0,4)||en[3]!=' ')return false;
            if(static_cast<unsigned char>(en[0])!=0xef||static_cast<unsigned char>(en[1])<0x80||static_cast<unsigned char>(en[1])>0x8b||static_cast<unsigned char>(en[2])<0x80||static_cast<unsigned char>(en[2])>0xbf)return false;
            en.remove_prefix(4);
        }
        for(unsigned char ch:en)if(ch>=128)return false;
    }
    return true;
}
static_assert(valid_control_icons(),"All 15 controls need matching Fork Awesome icons and a space before their text; other English text stays ASCII");
static_assert(std::string_view(catalog[static_cast<size_t>(Text::Return)].en).substr(4)=="Return to game E"&&std::string_view(catalog[static_cast<size_t>(Text::Return)].zh).substr(4)=="返回游戏 E","Return button retains its shortcut after the icon");
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
    check(std::string(get(Text::Title,false)).find("###SinkingStarSketch")!=std::string::npos&&std::string(get(Text::Title,true)).find("###SinkingStarSketch")!=std::string::npos,"Window identity stays stable when language changes");
    for(bool chinese:{false,true}){
        check(std::string(get(Text::ShortcutsTitle,chinese)).find("###SketchShortcuts")!=std::string::npos,"Shortcut window identity stays stable when language changes");
        check(std::string(get(Text::Return,chinese)).back()=='E',"Return button keeps its shortcut");
        for(Text label:{Text::Pen,Text::Line,Text::Arrow,Text::Rectangle,Text::Ellipse,Text::Text,Text::Eraser,Text::Path,Text::CameraToggle,Text::Done,Text::Cancel,Text::Hud,Text::Anchoring,Text::Cleared}){
            std::string text=get(label,chinese);
            check(text.find("Alt+")==std::string::npos&&text.find("Ctrl+")==std::string::npos&&text.find("Esc")==std::string::npos,"Normal interface omits shortcut hints");
        }
        std::string help=std::string(get(Text::ShortcutGeneral,chinese))+get(Text::ShortcutTools,chinese)+get(Text::ShortcutDrawing,chinese)+get(Text::CameraHelp,chinese);
        for(const char *key:{"E:","G:","B:","T:","Esc","Alt+Tab","Alt+P","Alt+L","Alt+A","Alt+R","Alt+O","Alt+T","Alt+K","Alt+H","Alt+1-5","Shift","Ctrl+Z","Ctrl+Y","Ctrl+Shift+Z","Ctrl+1","Ctrl+2","Ctrl+3","Ctrl+S","Ctrl+Shift+Delete","Ctrl+Enter","W/A/S/D","Q / Z"}){
            std::string token=key;if(chinese&&token.back()==':')token.pop_back();
            check(help.find(token)!=std::string::npos,"Unified help includes every shortcut");
        }
    }
    std::cout<<"ALL LANGUAGE TESTS PASSED\n";return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
