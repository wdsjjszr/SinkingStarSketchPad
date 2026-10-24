#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <deque>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>
#include "grid_math.hpp"

namespace sketch {
struct Point { float x = 0, y = 0, z = 0; };
enum class Tool : unsigned { Pen, Line, Arrow, Rectangle, Ellipse, Text, Eraser, Path };
struct Stroke {
    Tool tool = Tool::Pen;
    unsigned color = 0;
    float width = 3;
    std::vector<Point> points;
    std::string text;
    grid::Lattice lattice;
};
using Page = std::vector<std::shared_ptr<const Stroke>>;
struct History {
    std::deque<Page> undo, redo;
    void record(const Page &p) { undo.push_back(p); if (undo.size() > 60) undo.pop_front(); redo.clear(); }
    bool back(Page &p) { return move(undo, redo, p); }
    bool forward(Page &p) { return move(redo, undo, p); }
    static bool move(std::deque<Page> &from, std::deque<Page> &to, Page &p) {
        if (from.empty()) return false;
        to.push_back(p); p = std::move(from.back()); from.pop_back(); return true;
    }
};
struct Document {
    std::array<Page, 3> pages;
    unsigned page = 0;
    bool cursor = true;
    std::array<grid::Lattice,3> grids;
    unsigned format_version=4;
    size_t point_count() const {
        size_t n = 0;
        for (const auto &p : pages) for (const auto &s : p) n += s->points.size();
        return n;
    }
    void validate() const {
        if (page > 2) throw std::runtime_error("Invalid page");
        for(const auto &g:grids)if(!g.valid())throw std::runtime_error("Invalid grid calibration");
        size_t total = 0;
        for (const auto &p : pages) {
            if (p.size() > 2000) throw std::runtime_error("Too many strokes");
            for (const auto &s : p) {
                if (!s || (s->tool > Tool::Path || s->tool == Tool::Eraser) || s->color > 4 || !std::isfinite(s->width) || s->width < 1 || s->width > 16
                    || s->points.empty() || s->points.size() > 10000 || s->text.size() > 2048)
                    throw std::runtime_error("Invalid stroke");
                if (s->tool == Tool::Text && s->points.size() != 1) throw std::runtime_error("Invalid text position");
                if (s->tool != Tool::Text && s->tool != Tool::Pen && s->tool != Tool::Path && s->points.size() != 2) throw std::runtime_error("Invalid shape");
                if(!s->lattice.valid()||(s->tool==Tool::Path&&!s->lattice.ready))throw std::runtime_error("Invalid stroke grid");
                for (const auto &pt : s->points)
                    if (!std::isfinite(pt.x) || !std::isfinite(pt.y) || !std::isfinite(pt.z) || std::abs(pt.z)>1000000
                        || (pt.z!=0&&(!s->lattice.ready||(s->tool!=Tool::Line&&s->tool!=Tool::Arrow&&s->tool!=Tool::Path)))
                        || (s->lattice.ready ? (std::abs(pt.x)>1000000||std::abs(pt.y)>1000000||(s->tool==Tool::Path&&(std::round(pt.x)!=pt.x||std::round(pt.y)!=pt.y||std::round(pt.z)!=pt.z))) : (pt.x < 0 || pt.x > 1 || pt.y < 0 || pt.y > 1)))
                        throw std::runtime_error("Invalid coordinates");
                total += s->points.size();
                if (total > 120000) throw std::runtime_error("Too many points");
            }
        }
    }
    static void write_grid(std::ostream &out,const grid::Lattice &g){
        out<<g.ready<<' '<<g.origin.x<<' '<<g.origin.y<<' '<<g.origin.z<<' '
           <<g.u.x<<' '<<g.u.y<<' '<<g.u.z<<' '<<g.v.x<<' '<<g.v.y<<' '<<g.v.z<<'\n';
    }
    static grid::Lattice read_grid(std::istream &in){
        grid::Lattice g;unsigned ready;
        if(!(in>>ready>>g.origin.x>>g.origin.y>>g.origin.z>>g.u.x>>g.u.y>>g.u.z>>g.v.x>>g.v.y>>g.v.z)||ready>1)
            throw std::runtime_error("Invalid grid data");
        g.ready=ready!=0;if(!g.valid())throw std::runtime_error("Invalid grid data");return g;
    }
    void write(std::ostream &out) const {
        validate();
        out << "SINKING_STAR_SKETCH 4\n" << page << ' ' << cursor << '\n' << std::setprecision(17);
        for(const auto &g:grids)write_grid(out,g);
        for (const auto &p : pages) {
            out << p.size() << '\n';
            for (const auto &s : p) {
                out << unsigned(s->tool) << ' ' << s->color << ' ' << s->width << ' ' << s->points.size() << ' ' << std::quoted(s->text) << '\n';
                write_grid(out,s->lattice);
                for (const auto &pt : s->points) out << pt.x << ' ' << pt.y << ' ' << pt.z << '\n';
            }
        }
        if (!out) throw std::runtime_error("Draft write failed");
    }
    static Document read(std::istream &in) {
        Document d; std::string magic; unsigned version = 0, cursor = 0;
        if (!(in >> magic >> version) || magic != "SINKING_STAR_SKETCH" || version<1 || version>4)
            throw std::runtime_error("Unsupported draft file");
        if (!(in >> d.page >> cursor) || cursor > 1 || d.page > 2) throw std::runtime_error("Invalid draft header");
        d.cursor = cursor != 0;
        d.format_version=version;
        if(version>=2)for(auto &g:d.grids)g=read_grid(in);
        size_t total = 0;
        for (auto &p : d.pages) {
            size_t n = 0;
            if (!(in >> n) || n > 2000) throw std::runtime_error("Invalid stroke count");
            for (size_t i = 0; i < n; ++i) {
                auto s = std::make_shared<Stroke>(); unsigned tool = 0; size_t count = 0;
                if (!(in >> tool >> s->color >> s->width >> count >> std::quoted(s->text)) || (tool>7||tool==6||(version==1&&tool>5)) || count == 0 || count > 10000 || s->text.size() > 2048)
                    throw std::runtime_error("Invalid stroke data");
                total += count; if (total > 120000) throw std::runtime_error("Draft too large");
                s->tool = Tool(tool); s->points.resize(count);
                if(version>=3||s->tool==Tool::Path)s->lattice=read_grid(in);
                for (auto &pt : s->points) if (!(in >> pt.x >> pt.y)||(version>=4&&!(in>>pt.z))) throw std::runtime_error("Truncated draft");
                p.push_back(s);
            }
        }
        in >> std::ws;
        if (!in.eof()) throw std::runtime_error("Unexpected trailing data");
        d.validate(); return d;
    }
};
inline Point pixel(Point p, Point size) { return { p.x * size.x, p.y * size.y,p.z }; }
inline Point normal(Point p, Point size) { return { std::clamp(p.x / std::max(1.f, size.x), 0.f, 1.f), std::clamp(p.y / std::max(1.f, size.y), 0.f, 1.f) }; }
inline float distance(Point a, Point b) { return std::hypot(a.x - b.x, a.y - b.y); }
inline float segment_distance(Point p, Point a, Point b) {
    const float dx = b.x - a.x, dy = b.y - a.y;
    const float norm = dx * dx + dy * dy;
    const float t = norm > 0 ? std::clamp(((p.x-a.x)*dx+(p.y-a.y)*dy)/norm, 0.f, 1.f) : 0;
    return distance(p, {a.x + t*dx, a.y + t*dy});
}
inline Point snapped(Point a, Point b, bool square) {
    const float dx = b.x-a.x, dy = b.y-a.y;
    if (square) { const float s = std::max(std::abs(dx), std::abs(dy)); return { a.x+std::copysign(s,dx), a.y+std::copysign(s,dy) }; }
    const float angle = std::round(std::atan2(dy,dx) / .785398163f) * .785398163f, len = std::hypot(dx,dy);
    return {a.x + std::cos(angle)*len, a.y + std::sin(angle)*len};
}
inline std::vector<Point> outline(const Stroke &s, Point size) {
    std::vector<Point> pts;
    Point a = pixel(s.points.front(),size), b = pixel(s.points.back(),size);
    if (s.tool == Tool::Rectangle) {
        if(a.x==b.x&&a.y==b.y)return {a};
        if(a.x==b.x||a.y==b.y)return {a,b};
        return {a, {b.x,a.y}, b, {a.x,b.y}, a};
    }
    if (s.tool == Tool::Ellipse) {
        Point c{(a.x+b.x)/2, (a.y+b.y)/2};
        for (int i=0;i<=64;++i) { float angle=i*6.283185307f/64; pts.push_back({c.x+std::abs(b.x-a.x)*.5f*std::cos(angle),c.y+std::abs(b.y-a.y)*.5f*std::sin(angle)}); }
    } else for (auto p : s.points) pts.push_back(pixel(p,size));
    return pts;
}
inline bool hit(const Stroke &s, Point p, Point size) {
    auto pts=outline(s,size); float radius = s.width*.5f+9;
    if (pts.size()==1) return distance(pts[0],p)<=radius;
    for (size_t i=1;i<pts.size();++i) if (segment_distance(p,pts[i-1],pts[i])<=radius) return true;
    return false;
}
}
