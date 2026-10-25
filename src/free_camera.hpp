#pragma once
#include "grid_math.hpp"
#include <cstring>
#include <cstdint>

namespace sketch::free_camera {
using Mat=std::array<double,16>;
using V=grid::V;
inline Mat identity(){return {1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1};}
inline Mat mul(const Mat&a,const Mat&b){Mat c{};for(int i=0;i<4;++i)for(int j=0;j<4;++j)for(int k=0;k<4;++k)c[i*4+j]+=a[i*4+k]*b[k*4+j];return c;}
inline bool inverse(const Mat&m,Mat&out){
    double a[4][8]{};for(int r=0;r<4;++r)for(int c=0;c<4;++c){if(!std::isfinite(m[r*4+c]))return false;a[r][c]=m[r*4+c];a[r][c+4]=r==c;}
    for(int c=0;c<4;++c){int p=c;for(int r=c+1;r<4;++r)if(std::abs(a[r][c])>std::abs(a[p][c]))p=r;
        if(std::abs(a[p][c])<1e-12)return false;for(int j=0;j<8;++j)std::swap(a[c][j],a[p][j]);
        double d=a[c][c];for(auto&x:a[c])x/=d;for(int r=0;r<4;++r)if(r!=c){d=a[r][c];for(int j=0;j<8;++j)a[r][j]-=d*a[c][j];}}
    for(int r=0;r<4;++r)for(int c=0;c<4;++c){out[r*4+c]=a[r][c+4];if(!std::isfinite(out[r*4+c]))return false;}return true;
}
inline Mat load(const float*p){Mat m;std::copy(p,p+16,m.begin());return m;}
inline void store(float*p,const Mat&m){for(int i=0;i<16;++i)p[i]=float(m[i]);}
inline bool pair_valid(const Mat&a,const Mat&b){auto p=mul(a,b);double e=0;for(int i=0;i<16;++i)e=std::max(e,std::abs(p[i]-(i%5==0)));return e<.025;}
inline V direction(const Mat&m,V p){return {m[0]*p.x+m[1]*p.y+m[2]*p.z,m[4]*p.x+m[5]*p.y+m[6]*p.z,m[8]*p.x+m[9]*p.y+m[10]*p.z};}
inline V position(const Mat&m,V p){return direction(m,p)+V{m[3],m[7],m[11]};}
inline V unit(V p){auto n=grid::length(p);return n>1e-10?p*(1/n):V{};}
inline Mat translation(V p){auto m=identity();m[3]=p.x;m[7]=p.y;m[11]=p.z;return m;}
inline Mat rotation(V axis,double angle){
    axis=unit(axis);double x=axis.x,y=axis.y,z=axis.z,c=std::cos(angle),s=std::sin(angle),t=1-c;
    return {t*x*x+c,t*x*y-s*z,t*x*z+s*y,0,t*x*y+s*z,t*y*y+c,t*y*z-s*x,0,t*x*z-s*y,t*y*z+s*x,t*z*z+c,0,0,0,0,1};
}
struct Frame {
    Mat vp{},inv{},view{},proj{};
    V eye{},forward{},right{},up{};
    uint32_t frame=0,width=0,height=0;
    bool valid=false;
};
inline Frame decode(const float*p){
    Frame f;uint32_t type;std::memcpy(&type,p+226,4);if(type!=8)return f;
    for(int i=0;i<224;++i)if(!std::isfinite(p[i]))return f;
    if(p[216]<=0||p[217]<=0||1/p[216]<64||1/p[217]<64||1/p[216]>16384||1/p[217]>16384)return f;
    f.vp=load(p);f.inv=load(p+16);f.view=load(p+160);f.proj=load(p+192);
    Mat inv_view;if(!pair_valid(f.vp,f.inv)||!inverse(f.view,inv_view))return f;
    auto product=mul(f.proj,f.view);for(int i=0;i<16;++i)if(std::abs(product[i]-f.vp[i])>.02*(1+std::abs(f.vp[i])))return f;
    f.eye={p[208],p[209],p[210]};V forward{p[212],p[213],p[214]};if(grid::dot(forward,forward)<.98||grid::dot(forward,forward)>1.02)return f;f.forward=unit(forward);
    if(grid::length(f.forward)<.99||grid::length(f.eye-position(inv_view,{}))>.1)return f;
    grid::Camera c{f.vp,f.inv,0,true,f.eye,f.forward};V center,x,y;
    if(!c.unproject(.5,.5,.5,center)||!c.unproject(.51,.5,.5,x)||!c.unproject(.5,.49,.5,y))return f;
    f.right=unit(x-center);f.up=unit(y-center);if(grid::length(f.right)<.99||grid::length(f.up)<.99)return f;
    f.width=uint32_t(std::round(1/p[216]));f.height=uint32_t(std::round(1/p[217]));std::memcpy(&f.frame,p+225,4);f.valid=true;return f;
}
struct Controller {
    bool enabled=false,holding=false,keep_view=true;
    Frame base;
    V pivot{},offset{};
    double yaw=0,pitch=0,zoom=1,speed=5,sensitivity=.08;
    bool invert_x=false,invert_y=true;
    bool enable(const Frame&f,double plane_z){
        if(!f.valid)return false;base=f;offset={};yaw=pitch=0;zoom=1;
        double distance=std::abs(f.forward.z)>.01?(plane_z-f.eye.z)/f.forward.z:20;
        if(!std::isfinite(distance)||distance<=0||distance>10000)distance=20;
        pivot=f.eye+f.forward*distance;enabled=true;holding=false;return true;
    }
    constexpr void leave(){holding=enabled&&keep_view;enabled=false;}
    constexpr void restore(){enabled=false;holding=false;}
    constexpr bool active_view() const{return enabled||holding;}
    Mat orient() const {constexpr double rad=3.141592653589793/180;return mul(rotation({0,0,1},yaw*rad),rotation(base.right,pitch*rad));}
    Mat world_transform() const {return mul(translation(pivot+offset),mul(orient(),translation(pivot*-1)));}
    std::array<double,2> pitch_limits() const {
        // Relative +/-85 degrees can cross the horizon from an already tilted
        // game view. Limit the total tilt, keeping the original side of the pole.
        constexpr double degrees=180/3.141592653589793;
        // Derive the tilt sign from the actual rotation axis, so mirrored or
        // left-handed projection conventions do not reverse these bounds.
        const double tangent_z=base.right.x*base.forward.y-base.right.y*base.forward.x;
        const double initial=std::atan2(tangent_z,-base.forward.z)*degrees;
        return initial>=0?std::array<double,2>{-initial,std::max(85.,initial)-initial}
                         :std::array<double,2>{std::min(-85.,initial)-initial,-initial};
    }
    void limit_angles(){auto limits=pitch_limits();yaw=std::remainder(yaw,360.);pitch=std::clamp(pitch,limits[0],limits[1]);}
    void rotate(double dx,double dy){if(!std::isfinite(dx)||!std::isfinite(dy))return;yaw+=dx;pitch+=dy;limit_angles();}
    void mouse_rotate(double dx,double dy){rotate(dx*sensitivity*(invert_x?1:-1),dy*sensitivity*(invert_y?-1:1));}
    void move(double right,double forward,double up,double seconds,bool fast){
        if(!enabled||!std::isfinite(seconds))return;seconds=std::clamp(seconds,0.,.1);
        // Ground navigation follows yaw, independently of camera pitch.
        // Tilting near vertical must not change or reverse WASD directions.
        auto turn=rotation({0,0,1},yaw*3.141592653589793/180);
        auto r=direction(turn,base.right),u=direction(turn,base.up);r.z=u.z=0;r=unit(r);u=unit(u);
        auto d=r*right+u*forward+V{0,0,up};auto length=grid::length(d);if(length>1)d=d*(1/length);
        offset=offset+d*(seconds*speed*(fast?2:1));
        offset.x=std::clamp(offset.x,-5000.,5000.);offset.y=std::clamp(offset.y,-5000.,5000.);offset.z=std::clamp(offset.z,-5000.,5000.);
    }
    void magnify(double wheel){if(std::isfinite(wheel))zoom=std::clamp(zoom*std::pow(1.06,std::clamp(wheel,-20.,20.)),.05,20.);}
    bool apply(float*p) const {
        if(!active_view()||!base.valid||!std::isfinite(yaw)||!std::isfinite(pitch)||!std::isfinite(zoom)||!grid::finite(offset)||zoom<.05||zoom>20)return false;
        Frame original=decode(p);if(!original.valid)return false;
        Mat delta=world_transform(),inv_delta;if(!inverse(delta,inv_delta))return false;
        auto view=mul(base.view,inv_delta),proj=original.proj;for(int c=0;c<4;++c){proj[c]*=zoom;proj[4+c]*=zoom;}
        auto vp=mul(proj,view);Mat inv_vp,inv_proj,original_inv;if(!inverse(vp,inv_vp)||!inverse(proj,inv_proj)||!inverse(original.vp,original_inv))return false;
        for(int n=0;n<4;++n){auto reflection=load(p+32+16*n),reflection_inv=load(p+96+16*n);
            if(pair_valid(reflection,reflection_inv)&&reflection!=identity()){
                auto relative=mul(original_inv,reflection),next=mul(vp,relative);Mat next_inv;
                if(inverse(next,next_inv)){store(p+32+16*n,next);store(p+96+16*n,next_inv);}
            }}
        store(p,vp);store(p+16,inv_vp);store(p+160,view);store(p+176,inv_proj);store(p+192,proj);
        auto eye=position(delta,base.eye),forward=unit(direction(orient(),base.forward));
        p[208]=float(eye.x);p[209]=float(eye.y);p[210]=float(eye.z);p[212]=float(forward.x);p[213]=float(forward.y);p[214]=float(forward.z);return true;
    }
};
}
