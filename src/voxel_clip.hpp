#pragma once
#include "preview_mesh.hpp"
#include "native_ground.hpp"
#include <tuple>
#include <memory>

namespace sketch::grid {
constexpr double contact_epsilon=1e-4;
constexpr bool in_front(double z,double plane,int side){return side*(z-plane)>contact_epsilon;}
using PaintPolygon=std::vector<PreviewVertex>;
constexpr double edge(PreviewVertex a,PreviewVertex b,PreviewVertex p){return (b.x-a.x)*(p.y-a.y)-(b.y-a.y)*(p.x-a.x);}
inline double area(const PaintPolygon &p){double result=0;for(size_t n=0;n<p.size();++n){auto a=p[n],b=p[(n+1)%p.size()];result+=a.x*b.y-a.y*b.x;}return result*.5;}
inline PaintPolygon half_plane(const PaintPolygon &p,PreviewVertex a,PreviewVertex b,bool inside){
    PaintPolygon out;
    for(size_t n=0;n<p.size();++n){auto first=p[n],last=p[(n+1)%p.size()];double x=edge(a,b,first),y=edge(a,b,last);if(!inside){x=-x;y=-y;}
        if(x>=0)out.push_back(first);if((x>0&&y<0)||(x<0&&y>0))out.push_back(interpolate(first,last,x/(x-y)));
    }
    return out;
}
struct ScreenBounds {double left=0,top=0,right=0,bottom=0;};
template<class Points> inline ScreenBounds bounds(const Points &p){ScreenBounds b{p[0].x,p[0].y,p[0].x,p[0].y};for(auto v:p){b.left=std::min(b.left,v.x);b.top=std::min(b.top,v.y);b.right=std::max(b.right,v.x);b.bottom=std::max(b.bottom,v.y);}return b;}
constexpr bool overlaps(ScreenBounds a,ScreenBounds b){return a.left<b.right&&a.right>b.left&&a.top<b.bottom&&a.bottom>b.top;}
inline bool covers(const PaintPolygon &cover,const std::array<PreviewVertex,3> &points){
    for(size_t n=0;n<cover.size();++n)for(auto point:points)if(edge(cover[n],cover[(n+1)%cover.size()],point)<0)return false;
    return true;
}
inline std::vector<PaintPolygon> subtract(PaintPolygon subject,const PaintPolygon &cover){
    std::vector<PaintPolygon> out;
    if(!overlaps(bounds(subject),bounds(cover)))return {std::move(subject)};
    for(size_t n=0;n<cover.size()&&subject.size()>=3;++n){auto a=cover[n],b=cover[(n+1)%cover.size()];auto outside=half_plane(subject,a,b,false);
        if(outside.size()>=3&&std::abs(area(outside))>1e-8)out.push_back(std::move(outside));subject=half_plane(subject,a,b,true);
    }
    return out;
}
struct ScreenCuts {
    static constexpr int tile=64;
    std::vector<PaintPolygon> polygons;
    std::vector<ScreenBounds> polygon_bounds;
    std::vector<std::vector<unsigned>> bins;
    int columns=0,rows=0;
    void reset(double width,double height){polygons.clear();polygon_bounds.clear();columns=int(std::ceil(width/tile));rows=int(std::ceil(height/tile));bins.assign(size_t(columns)*rows,{});}
    std::array<int,4> tiles(ScreenBounds b)const{return {std::clamp(int(std::floor(b.left/tile)),0,columns-1),std::clamp(int(std::floor(b.top/tile)),0,rows-1),std::clamp(int(std::floor(b.right/tile)),0,columns-1),std::clamp(int(std::floor(b.bottom/tile)),0,rows-1)};}
    void add(PaintPolygon p){
        if(p.size()<3||std::abs(area(p))<1e-8)return;if(area(p)<0)std::reverse(p.begin(),p.end());auto b=bounds(p);auto region=tiles(b);unsigned id=unsigned(polygons.size());polygons.push_back(std::move(p));polygon_bounds.push_back(b);
        for(int y=region[1];y<=region[3];++y)for(int x=region[0];x<=region[2];++x)bins[size_t(y)*columns+x].push_back(id);
    }
    PreviewMesh cut(PreviewMesh mesh)const{
        if(polygons.empty())return mesh;PreviewMesh result;result.triangles.reserve(mesh.triangles.size());
        std::vector<unsigned> ids;std::vector<size_t> seen(polygons.size(),mesh.triangles.size());std::vector<PaintPolygon> pieces,next;
        for(size_t n=0;n<mesh.triangles.size();++n){const auto &triangle=mesh.triangles[n];auto b=bounds(triangle.points);auto region=tiles(b);ids.clear();bool hidden=false;
            for(int y=region[1];y<=region[3]&&!hidden;++y)for(int x=region[0];x<=region[2]&&!hidden;++x)for(auto id:bins[size_t(y)*columns+x]){
                if(seen[id]==n)continue;seen[id]=n;if(!overlaps(b,polygon_bounds[id]))continue;
                if(covers(polygons[id],triangle.points)){hidden=true;break;}ids.push_back(id);
            }
            if(hidden)continue;if(ids.empty()){result.triangles.push_back(triangle);continue;}
            std::sort(ids.begin(),ids.end());pieces.clear();pieces.emplace_back(triangle.points.begin(),triangle.points.end());
            for(auto id:ids){next.clear();for(auto &piece:pieces){auto difference=subtract(std::move(piece),polygons[id]);for(auto &p:difference)next.push_back(std::move(p));}pieces.swap(next);if(pieces.empty())break;}
            for(auto &p:pieces)result.polygon(p,triangle.shade);
        }
        return result;
    }
};
struct FaceRect {int axis=0;double level=0,u0=0,v0=0,u1=0,v1=0;};
using FaceCells=std::set<std::pair<int,int>>; // Row, column: merge runs without scanning empty world bounds.
inline std::vector<std::array<int,4>> merge_faces(FaceCells cells){
    std::vector<std::array<int,4>> out;
    while(!cells.empty()){
        int v0=cells.begin()->first,u0=cells.begin()->second,u1=u0+1,v1=v0+1;
        while(cells.count({v0,u1}))++u1;
        for(;;){bool full=true;for(int u=u0;u<u1;++u)if(!cells.count({v1,u})){full=false;break;}if(!full)break;++v1;}
        for(int v=v0;v<v1;++v)for(int u=u0;u<u1;++u)cells.erase({v,u});out.push_back({u0,v0,u1,v1});
    }
    return out;
}
inline std::vector<FaceRect> exposed_faces(const native_ground::Snapshot &terrain){
    std::map<std::tuple<int,int,int>,FaceCells> groups;
    for(const auto &[p,id]:terrain.voxels){const int cell[]={p.x,p.y,p.z};
        for(int axis=0;axis<3;++axis)for(int sign:{-1,1}){int neighbor[]={p.x,p.y,p.z};neighbor[axis]+=sign;if(terrain.voxels.count({neighbor[0],neighbor[1],neighbor[2]}))continue;
            int level=2*cell[axis]+(axis==2?(sign>0?2:0):sign),u=axis==0?p.y:p.x,v=axis==2?p.y:p.z;groups[{axis,level,sign}].insert({v,u});}
    }
    std::vector<FaceRect> faces;
    for(auto &[key,cells]:groups){auto [axis,level,sign]=key;for(auto r:merge_faces(std::move(cells)))faces.push_back({axis,level*.5,r[0]-.5,r[1]-(axis==2?.5:0),r[2]-.5,r[3]-(axis==2?.5:0)});}
    return faces;
}
inline std::array<V,4> face_quad(FaceRect face){
    auto point=[&](double u,double v){return face.axis==0?V{face.level,u,v}:face.axis==1?V{u,face.level,v}:V{u,v,face.level};};
    return {point(face.u0,face.v0),point(face.u1,face.v0),point(face.u1,face.v1),point(face.u0,face.v1)};
}
inline std::vector<V> clip_height(const std::vector<V> &polygon,double cut,int side){
    std::vector<V> out;
    for(size_t n=0;n<polygon.size();++n){auto a=polygon[n],b=polygon[(n+1)%polygon.size()];double da=side*(a.z-cut),db=side*(b.z-cut);
        if(da>=0)out.push_back(a);if((da>0&&db<0)||(da<0&&db>0))out.push_back(a+(b-a)*(da/(da-db)));}
    return out;
}
inline void project_cover(ScreenCuts &cuts,const Camera &camera,const std::vector<V> &world,double width,double height){
    Polygon polygon;polygon.count=unsigned(world.size());for(size_t n=0;n<world.size();++n)polygon.points[n]=clip_point(camera,world[n]);polygon=clip(polygon);
    PaintPolygon screen;for(unsigned n=0;n<polygon.count;++n){auto p=polygon.points[n];screen.push_back({(p.x/p.w+1)*.5*width,(1-p.y/p.w)*.5*height,1});}cuts.add(std::move(screen));
}
struct OcclusionCache {
    Camera view;
    Lattice plane;
    std::shared_ptr<const native_ground::Snapshot> terrain;
    std::vector<FaceRect> faces;
    ScreenCuts cuts;
    PreviewMesh mesh;
    double width=0,height=0;
    int x=0,y=0;
    unsigned layers=0;
    bool ready=false;
    bool update(const Camera &camera,const Lattice &lattice,std::shared_ptr<const native_ground::Snapshot> snapshot,int first_x,int first_y,double w,double h,const Preview &style){
        unsigned mask=unsigned(style.centers)|(unsigned(style.intersections)<<1)|(unsigned(style.borders)<<2)|(unsigned(style.checker)<<3);
        if(ready&&same_view(view,camera)&&same(plane,lattice)&&terrain==snapshot&&x==first_x&&y==first_y&&width==w&&height==h&&layers==mask)return false;
        if(terrain!=snapshot){terrain=std::move(snapshot);faces=terrain?exposed_faces(*terrain):std::vector<FaceRect>{};}
        view=camera;plane=lattice;x=first_x;y=first_y;width=w;height=h;layers=mask;ready=true;cuts.reset(w,h);
        if(terrain){
            double i,j,sx,sy;Ray ray;
            if(preview_focus(camera,lattice,i,j)&&camera.project(lattice.world(i,j),sx,sy)&&camera.ray(sx,sy,ray)){
                int side=ray.direction.z<0?1:-1;double cut=lattice.origin.z+side*contact_epsilon;
                for(auto face:faces){auto quad=face_quad(face);bool front=false;for(auto p:quad)front=front||in_front(p.z,lattice.origin.z,side);if(!front)continue;
                    auto p=clip_height(std::vector<V>(quad.begin(),quad.end()),cut,side);if(p.size()>=3)project_cover(cuts,camera,p,w,h);}
                FaceCells caps;for(const auto &[p,id]:terrain->voxels)if(p.z<cut&&p.z+1>cut)caps.insert({p.y,p.x});
                for(auto r:merge_faces(std::move(caps))){auto quad=face_quad({2,cut,r[0]-.5,r[1]-.5,r[2]-.5,r[3]-.5});project_cover(cuts,camera,std::vector<V>(quad.begin(),quad.end()),w,h);}
            }
        }
        PreviewPainter painter{camera,lattice,w,h,1,0,0,false,{}};painter.layers(style,CellMask(x,y,128,128));mesh=cuts.cut(std::move(painter.mesh));return true;
    }
};
}
