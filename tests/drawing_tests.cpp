#include "../src/drawing_math.hpp"
#include <cassert>
#include <sstream>
using namespace sketch;
static_assert(nearest_grid_point({.1f,.2f}).x==0&&nearest_grid_point({.45f,.55f}).x==.5,"Snap to the nearest center or intersection");
static_assert(nearest_grid_point({-.5f,-.5f}).x==-.5&&nearest_grid_point({.5f,0}).y==0,"Negative intersections work and ties prefer centers");
static_assert(grid_shape_end(Tool::Line,{.5f,.5f},{3,.5f},true).y==.5,"Shift horizontal lines preserve legal half-cell coordinates");
static_assert(grid_shape_end(Tool::Rectangle,{0,0},{2,3},true).x==3&&grid_shape_end(Tool::Ellipse,{0,0},{2.2f,3.3f},false).x==2.2f,"Squares constrain on the plane; ellipse bounds remain continuous");
static_assert(!snaps_to_grid(Tool::Pen)&&!snaps_to_grid(Tool::Ellipse)&&snaps_to_grid(Tool::Arrow),"Only the requested geometric tools snap");
static_assert(grid_shape_end(Tool::Line,{-.5f,-.5f},{-3,-2},true).x==-2.5f&&grid_shape_end(Tool::Line,{-.5f,-.5f},{-3,-2},true).y==-2.5f,"Negative diagonal constraints snap to a legal intersection");
static_assert(grid_shape_end(Tool::Ellipse,{0,0},{-2,3},true).x==-3&&grid_shape_end(Tool::Rectangle,{.5f,.5f},{2.7f,-1.3f},true).y==-1.5f,"Circle and square constraints use plane distances");
int main(){
    Document doc;grid::Lattice g{true,{0,0,3.25},{1,0,0},{0,-1,0}};
    for(auto tool:{Tool::Pen,Tool::Line,Tool::Arrow,Tool::Rectangle,Tool::Ellipse,Tool::Text,Tool::Path}){
        auto stroke=std::make_shared<Stroke>();stroke->tool=tool;stroke->lattice=g;stroke->points={{-2.5f,1.25f},{3.5f,2.75f}};
        if(tool==Tool::Text){stroke->points.resize(1);stroke->text="世界坐标";}
        if(tool==Tool::Path)stroke->points={{-2,1},{3,1}};
        doc.pages[0].push_back(stroke);
    }
    auto screen=std::make_shared<Stroke>();screen->points={{.2f,.3f},{.5f,.6f}};doc.pages[0].push_back(screen);
    std::stringstream file;doc.write(file);auto loaded=Document::read(file);
    assert(loaded.format_version==3&&loaded.pages[0][0]->lattice.origin.z==3.25&&!loaded.pages[0].back()->lattice.ready);
    std::stringstream v2("SINKING_STAR_SKETCH 2\n0 1\n0 0 0 0 0 0 0 0 0 0\n0 0 0 0 0 0 0 0 0 0\n0 0 0 0 0 0 0 0 0 0\n1\n0 0 3 1 \"\"\n.2 .3\n0\n0\n");
    auto old=Document::read(v2);assert(old.format_version==2&&!old.pages[0][0]->lattice.ready);
    std::stringstream v1("SINKING_STAR_SKETCH 1\n0 1\n1\n0 0 3 1 \"\"\n.2 .3\n0\n0\n");
    assert(Document::read(v1).format_version==1);
    std::stringstream path_v2("SINKING_STAR_SKETCH 2\n0 1\n0 0 0 0 0 0 0 0 0 0\n0 0 0 0 0 0 0 0 0 0\n0 0 0 0 0 0 0 0 0 0\n1\n7 0 3 2 \"\"\n1 0 0 3.25 1 0 0 0 -1 0\n-2 1\n3 1\n0\n0\n");
    assert(Document::read(path_v2).pages[0][0]->lattice.origin.z==3.25);
    assert(projected_hit({{10,10},{40,10}},{20,10},3)&&!projected_hit({{10,10},{40,10}},{20,40},3));
    grid::Camera camera;camera.valid=true;camera.matrix={.1,0,0,0,0,.1,0,0,0,0,1,0,0,0,0,1};
    auto points=project_stroke(*doc.pages[0][3],camera,{1000,1000});assert(points.size()==5&&points.front().x!=doc.pages[0][3]->points[0].x);
    assert(project_stroke(*doc.pages[0][4],camera,{1000,1000}).size()==65);
    assert(project_stroke(*doc.pages[0][5],camera,{1000,1000}).size()==1);
    auto arrow=arrow_head({{0,0},{100,0}},3);assert(arrow.size()==3&&projected_hit(arrow,arrow.front(),3));
    auto before=project_stroke(*doc.pages[0][3],camera,{1000,1000});camera.matrix[12]=.02;
    auto after=project_stroke(*doc.pages[0][3],camera,{1000,1000});assert(before[0].x!=after[0].x);
    const auto &anchor=*doc.pages[0][5];assert(project_stroke(anchor,camera,{1000,1000}).size()==1&&anchor.width==3);
}
