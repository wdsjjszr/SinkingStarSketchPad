#include "../src/model.hpp"
#include "../src/native_ground_pick.hpp"
#include <iostream>
#include <sstream>
using namespace sketch;
void check(bool v,const char *name){if(!v)throw std::runtime_error(name);std::cout<<"PASS: "<<name<<'\n';}
grid::Camera perspective(double cx,double cy,double height,double zoom){
    grid::Camera c;c.valid=true;
    c.matrix={zoom,0,0,-zoom*cx,0,zoom,0,-zoom*cy,0,0,-1000./999,1000*(height-1)/999,0,0,-1,height};
    double a[4][8]{};
    for(int r=0;r<4;++r)for(int col=0;col<4;++col){a[r][col]=c.matrix[r*4+col];a[r][col+4]=r==col;}
    for(int col=0;col<4;++col){int pivot=col;for(int r=col+1;r<4;++r)if(std::abs(a[r][col])>std::abs(a[pivot][col]))pivot=r;
        for(int j=0;j<8;++j)std::swap(a[col][j],a[pivot][j]);double d=a[col][col];for(int j=0;j<8;++j)a[col][j]/=d;
        for(int r=0;r<4;++r)if(r!=col){double scale=a[r][col];for(int j=0;j<8;++j)a[r][j]-=scale*a[col][j];}}
    for(int r=0;r<4;++r)for(int col=0;col<4;++col)c.inverse[r*4+col]=a[r][col+4];return c;
}
int main(int argc,char **argv){try{
    auto first=perspective(8.5,8.5,500,50),second=perspective(11.5,7.5,500,73);
    const grid::V origin{9,9,2.75};std::array<std::array<double,2>,3> clicks;
    first.project(origin,clicks[0][0],clicks[0][1]);first.project(origin+grid::V{1,0,0},clicks[1][0],clicks[1][1]);first.project(origin+grid::V{0,-1,0},clicks[2][0],clicks[2][1]);
    double x,y;check(second.project(origin,x,y),"Calibration target projects into the camera");grid::Lattice g;
    check(grid::calibrate(first,clicks,second,x,y,g),"Four-click perspective calibration");
    check(grid::length(g.origin-origin)<1e-6&&grid::length(g.u-grid::V{1,0,0})<1e-6,"Actual grid plane and spacing recovered");
    grid::Lattice rejected;
    check(!grid::calibrate(first,clicks,first,clicks[0][0],clicks[0][1],rejected),"No camera movement rejected");
    native_ground::Scene scene;scene.floors={{{0,0,0},native_ground::Source::Fence,1},{{0,0,3},native_ground::Source::Fence,2}};
    native_ground::Snapshot ground;ground.set_geometry(scene);
    const auto picking_camera=perspective(0,0,10,1);
    auto hit=native_ground::pick(ground,picking_camera,.5,.5);
    check(hit.result==native_ground::PickResult::Ground&&hit.center.z==3,"Overlapping native floors select the closest visible surface");
    check(native_ground::pick(ground,picking_camera,.6,.5).result==native_ground::PickResult::None,"An infinite plane cannot be selected outside native ground cells");
    for(auto c:{first,second,perspective(-12,13,320,80)}){
        auto world=g.world(-4,7);c.project(world,x,y);double i,j;
        check(g.snap(c,x,y,i,j)&&i==-4&&j==7,"Pan and zoom preserve lattice coordinates");
        grid::Ray ray;grid::V plane;c.ray(x,y,ray);grid::plane(ray,g.origin.z,plane);
        check(grid::length(plane-world)<1e-7,"Projection and plane intersection round-trip");
    }
    auto stroke=std::make_shared<Stroke>();stroke->tool=Tool::Path;stroke->lattice=g;stroke->points={{-4,7},{-3,7},{-2,7}};
    Document d;d.grids[0]=g;d.pages[0].push_back(stroke);std::stringstream saved;d.write(saved);auto read=Document::read(saved);
    check(read.pages[0][0]->points[0].x==-4&&grid::length(read.grids[0].origin-origin)<1e-6,"World paths and calibration persist");
    d.grids[0].origin={100,100,0};check(d.pages[0][0]->lattice.origin.x==g.origin.x,"Recalibration does not move existing paths");
    std::stringstream legacy("SINKING_STAR_SKETCH 1\n0 1\n1\n0 0 3 1 \"\"\n.2 .3\n0\n0\n");
    check(Document::read(legacy).pages[0].size()==1,"Version 1 screen drafts remain readable");
    stroke->points[0].x=.2f;bool bad=false;try{d.validate();}catch(...){bad=true;}check(bad,"Fractional grid coordinates rejected");stroke->points[0].x=-4;
    if(argc>1){Document fixture;fixture.grids[0]=g;auto path=std::make_shared<Stroke>();path->tool=Tool::Path;path->width=5;path->color=1;path->lattice={true,{0,0,0},{1,0,0},{0,1,0}};
        for(int i=-4;i<=4;++i)path->points.push_back({float(i),0});fixture.pages[0].push_back(path);
        std::filesystem::create_directories(std::filesystem::u8path(argv[1]));std::ofstream out(std::filesystem::u8path(argv[1])/"drafts.txt");fixture.write(out);}
    std::cout<<"ALL GRID TESTS PASSED\n";return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
