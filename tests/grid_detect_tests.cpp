#include "../src/grid_detect.hpp"
#include <iostream>
#include <fstream>
#include <chrono>
using namespace sketch;
void check(bool ok,const char *name){if(!ok)throw std::runtime_error(name);std::cout<<"PASS: "<<name<<'\n';}
int main(int argc,char**argv){try{
    std::vector<grid::V> points;
    for(int x=0;x<12;++x)for(int y=0;y<10;++y){points.push_back({x*1.25+.5,y*1.25+.75,2});points.push_back(points.back());}
    for(int n=0;n<40;++n)points.push_back({n*.193,n*.117,4.2});
    auto result=grid::detect(points);
    check(result.lattice.ready&&result.centers==120,"Automatic lattice rejects duplicates and scattered objects");
    check(std::abs(result.lattice.u.x-1.25)<.001&&std::abs(result.lattice.origin.x-.5)<.001&&std::abs(result.lattice.origin.y+.5)<.001&&result.lattice.origin.z==2,"Spacing, center phase and height inferred from instances");
    check(!grid::detect({{0,0,0},{1,0,0},{2,0,0},{3,0,0},{4,0,0},{5,0,0}}).lattice.ready,"Single line of instances cannot establish a grid");
    std::vector<grid::V> dense;for(int x=0;x<16;++x)for(int y=0;y<16;++y)dense.push_back({x+.5,y+.5,0});
    auto dense_grid=grid::detect(dense);check(dense_grid.centers==256&&dense_grid.lattice.u.x==1,"Dense grid sampling does not alias the cell spacing");
    for(int x=0;x<3;++x)for(int y=0;y<2;++y)dense.push_back({x+.5,y+.5,3});
    auto layers=grid::detect_layers(dense);
    check(layers.size()==2&&layers[0].lattice.origin.z==0&&layers[1].lattice.origin.z==3,"Manual selection retains a smaller floor above the automatic winner");
    check(layers[1].cells.size()==6&&layers[1].cells.count({500,500})&&!layers[1].cells.count({3500,500}),"Layer footprints contain actual occupied cells only");
    check(grid::best_detection(layers)->lattice.origin.z==0,"Automatic mode keeps the highest-scoring layer");
    float matrix[12]={0,-1,0,5,1,0,0,7,0,0,1,0};grid::V p;
    check(grid::instance_center(matrix,p)&&p.x==5&&p.y==7,"Quarter-turn instance center decoded");
    matrix[2]=.3f;check(!grid::instance_center(matrix,p),"Non-grid animation matrix rejected");
    if(argc>1){std::vector<grid::V> actual;
        for(int n=1;n<argc;++n){std::ifstream f(argv[n],std::ios::binary);std::string data((std::istreambuf_iterator<char>(f)),{});auto centers=grid::instance_centers(data.data(),data.size());actual.insert(actual.end(),centers.begin(),centers.end());}
        auto start=std::chrono::steady_clock::now();auto found=grid::detect(actual);
        check(found.lattice.ready&&found.centers>=15&&std::abs(found.lattice.u.x-1)<.001&&grid::length(found.lattice.origin)<.001,"Actual game upload data identifies integer cell centers, spacing 1 and plane Z=0");
        std::cout<<"Actual centers "<<found.centers<<", edges "<<found.edges<<", inference ms "<<std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count()<<'\n';}
    std::cout<<"ALL AUTOMATIC GRID TESTS PASSED\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
