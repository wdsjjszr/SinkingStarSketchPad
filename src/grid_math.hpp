#pragma once
#include <array>
#include <cmath>
#include <algorithm>

namespace sketch::grid {
struct V { double x=0,y=0,z=0; };
constexpr V operator+(V a,V b){return {a.x+b.x,a.y+b.y,a.z+b.z};}
constexpr V operator-(V a,V b){return {a.x-b.x,a.y-b.y,a.z-b.z};}
constexpr V operator*(V a,double b){return {a.x*b,a.y*b,a.z*b};}
constexpr double dot(V a,V b){return a.x*b.x+a.y*b.y+a.z*b.z;}
inline double length(V a){return std::sqrt(dot(a,a));}
inline bool finite(V a){return std::isfinite(a.x)&&std::isfinite(a.y)&&std::isfinite(a.z);}
struct Ray {V origin,direction;};
constexpr bool viewing_ray(V a,V b,V eye,V forward,Ray &ray){
    auto direction=b-a;const double along=dot(direction,forward);
    if(along>-1e-12&&along<1e-12)return false;
    // Start on the eye plane and point into the view, independent of depth order.
    ray.origin=a+direction*(dot(eye-a,forward)/along);
    ray.direction=direction*(along>0?1:-1);return true;
}
struct Camera {
    std::array<double,16> matrix{},inverse{};
    unsigned frame=0;
    bool valid=false;
    V eye{},forward{};
    bool project(V p,double &x,double &y) const {
        double w=matrix[12]*p.x+matrix[13]*p.y+matrix[14]*p.z+matrix[15];
        if(!valid||!std::isfinite(w)||w<=1e-8)return false;
        x=((matrix[0]*p.x+matrix[1]*p.y+matrix[2]*p.z+matrix[3])/w+1)*.5;
        y=(1-(matrix[4]*p.x+matrix[5]*p.y+matrix[6]*p.z+matrix[7])/w)*.5;
        return std::isfinite(x)&&std::isfinite(y)&&std::abs(x)<100&&std::abs(y)<100;
    }
    bool unproject(double x,double y,double depth,V &out) const {
        const double a=2*x-1,b=1-2*y;
        double w=inverse[12]*a+inverse[13]*b+inverse[14]*depth+inverse[15];
        if(!valid||!std::isfinite(w)||std::abs(w)<1e-12)return false;
        out={(inverse[0]*a+inverse[1]*b+inverse[2]*depth+inverse[3])/w,
             (inverse[4]*a+inverse[5]*b+inverse[6]*depth+inverse[7])/w,
             (inverse[8]*a+inverse[9]*b+inverse[10]*depth+inverse[11])/w};
        return finite(out);
    }
    bool ray(double x,double y,Ray &r) const {
        V a,b;if(!unproject(x,y,.25,a)||!unproject(x,y,.75,b)||!viewing_ray(a,b,eye,forward,r))return false;
        double n=length(r.direction);if(!std::isfinite(n)||n<1e-8)return false;
        r.direction=r.direction*(1/n);return finite(r.origin);
    }
};
inline bool plane(Ray ray,double z,V &out){
    if(std::abs(ray.direction.z)<1e-8)return false;
    out=ray.origin+ray.direction*((z-ray.origin.z)/ray.direction.z);
    return finite(out)&&length(out)<1e7;
}
inline bool triangulate(Ray a,Ray b,V &out,double &separation){
    const double c=dot(a.direction,b.direction),denom=1-c*c;
    if(denom<1e-7)return false; // A real camera translation is needed.
    V w=a.origin-b.origin;
    double d=dot(a.direction,w),e=dot(b.direction,w);
    V p=a.origin+a.direction*((c*e-d)/denom);
    V q=b.origin+b.direction*((e-c*d)/denom);
    separation=length(p-q);out=(p+q)*.5;
    return finite(out)&&length(out)<1e7;
}
struct Lattice {
    bool ready=false;
    V origin,u,v;
    bool valid() const {
        if(!ready)return true;
        double a=length(u),b=length(v),det=u.x*v.y-u.y*v.x;
        return finite(origin)&&finite(u)&&finite(v)&&length(origin)<1e7&&a>.0001&&b>.0001&&a<1e5&&b<1e5
            &&std::abs(det)>a*b*.2&&std::abs(u.z)<.001*a&&std::abs(v.z)<.001*b;
    }
    V world(double i,double j) const {return origin+u*i+v*j;}
    bool snap(const Camera &c,double x,double y,double &i,double &j) const {
        Ray r;V p;if(!ready||!valid()||!c.ray(x,y,r)||!plane(r,origin.z,p))return false;
        p=p-origin;double det=u.x*v.y-u.y*v.x;
        i=std::round((p.x*v.y-p.y*v.x)/det);j=std::round((u.x*p.y-u.y*p.x)/det);
        return std::isfinite(i)&&std::isfinite(j)&&std::abs(i)<=1000000&&std::abs(j)<=1000000;
    }
};
inline bool calibrate(const Camera &first,const std::array<std::array<double,2>,3>&clicks,
                      const Camera &second,double x,double y,Lattice &out){
    Ray a,b;V p;double error;
    if(!first.ray(clicks[0][0],clicks[0][1],a)||!second.ray(x,y,b)||!triangulate(a,b,p,error))return false;
    V right,down;
    if(!first.ray(clicks[1][0],clicks[1][1],a)||!plane(a,p.z,right)
        ||!first.ray(clicks[2][0],clicks[2][1],b)||!plane(b,p.z,down))return false;
    Lattice l{true,p,right-p,down-p};
    double u=length(l.u),v=length(l.v);
    if(!l.valid()||u/v<.65||u/v>1.55||std::abs(dot(l.u,l.v))>u*v*.35||error>std::min(u,v)*.12)return false;
    double sx,sy;if(!second.project(p,sx,sy)||std::hypot(sx-x,sy-y)>.003)return false;
    out=l;return true;
}
}
