#include "../src/model.hpp"
#include <iostream>
#include <sstream>
using namespace sketch;
void check(bool ok,const char*label){if(!ok)throw std::runtime_error(label);std::cout<<"PASS: "<<label<<'\n';}
int main(int argc,char**argv){try{
    Document d;auto stroke=std::make_shared<Stroke>();stroke->points={{.2f,.3f},{.7f,.3f}};d.pages[0].push_back(stroke);
    auto text=std::make_shared<Stroke>();text->tool=Tool::Text;text->points={{.2f,.1f}};text->text="沉星草稿：中文与引号 \"测试\"\n第二行";d.pages[1].push_back(text);
    auto path=std::make_shared<Stroke>();path->tool=Tool::Path;path->numbered=true;path->lattice={true,{0,0,2},{1,0,0},{0,-1,0}};path->points={{0,0,0},{0,0,1},{0,0,2}};d.pages[2].push_back(path);
    std::stringstream ss;d.write(ss);auto read=Document::read(ss);check(read.pages[1][0]->text==text->text&&read.pages[0][0]->points[1].x==.7f,"UTF-8 and coordinates round-trip");
    check(read.format_version==5&&read.pages[2][0]->numbered&&read.pages[2][0]->points.size()==3&&!read.pages[0][0]->numbered,"V5 retains numbered paths without numbering other marks");
    stroke->numbered=true;bool bad_numbering=false;try{d.validate();}catch(...){bad_numbering=true;}check(bad_numbering,"Numbering is restricted to paths");stroke->numbered=false;
    Page p;History h;h.record(p);p.push_back(stroke);h.back(p);check(p.empty(),"Undo stroke");h.forward(p);check(p.size()==1,"Redo stroke");h.record(p);p.clear();h.back(p);check(p.size()==1,"Undo clear");h.record(p);check(h.redo.empty(),"New edit invalidates redo");
    for(int i=0;i<80;++i)h.record(p);check(h.undo.size()==60,"History is bounded");
    check(hit(*stroke,{400,300},{1000,1000})&&!hit(*stroke,{400,500},{1000,1000}),"Geometric erase hit and miss");
    Point snap=snapped({100,100},{200,108},false);check(std::abs(snap.y-100)<.001f,"45-degree line snapping");
    for(auto data: {std::string("BAD 1"),ss.str().substr(0,ss.str().size()/2),ss.str()+"BAD"}){bool rejected=false;try{std::stringstream in(data);Document::read(in);}catch(...){rejected=true;}check(rejected,"Malformed draft rejected");}
    stroke->points[0].x=std::numeric_limits<float>::infinity();bool rejected=false;try{d.validate();}catch(...){rejected=true;}check(rejected,"Nonfinite coordinates rejected");stroke->points[0].x=.2f;
    if(argc>1){
        auto dir=std::filesystem::u8path(argv[1]);std::filesystem::create_directories(dir);
        Document fixture;
        for(unsigned i=0;i<5;++i){auto s=std::make_shared<Stroke>();s->tool=Tool(i);s->color=i;s->width=5;s->points={{.15f,.20f+i*.12f},{.65f,.26f+i*.12f}};fixture.pages[0].push_back(s);}
        text->points={{.25f,.05f}};fixture.pages[0].push_back(text);std::ofstream out(dir/L"drafts.txt");fixture.write(out);
    }
    std::cout<<"ALL TESTS PASSED\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
