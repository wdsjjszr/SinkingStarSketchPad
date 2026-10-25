#include "../src/ui_scale.hpp"
using namespace sketch::ui;
static_assert(UIScale{}.percent==100&&UIScale{}.px(12)==18&&UIScale{}.px(256)==384,"100 percent retains 18px text and the reference proportions");
static_assert(UIScale(10).percent==75&&UIScale(500).percent==200&&UIScale(150).legacy(400)==600,"Manual scale is bounded and legacy windows only multiply by the manual ratio");
static_assert(UIScale(200).px(28)==84&&UIScale(100,24).px(12)==24,"Geometry and font sizing share one scale, including host font size");
constexpr bool placement(){
    auto right=beside({20,30,384,600},200,300,320,1920,1080,12);
    auto left=beside({1400,30,384,600},900,300,320,1920,1080,12);
    auto small=beside({20,30,384,600},900,300,320,260,200,12);
    return right.x==416&&right.y==200&&left.x==1088&&left.y==748&&small.w==236&&small.h==176&&small.x==12&&small.y==12;
}
static_assert(placement(),"The flyout opens beside its panel, flips left and fits the viewport");
constexpr bool dismissal(){
    DismissClick click;click.consume();
    if(!click.block(true)||!click.block(true)||!click.block(false))return false;
    return !click.block(false)&&!click.block(true);
}
static_assert(dismissal(),"Closing clicks stay blocked through release; the next intentional click reaches the canvas");
static_assert(Rect{10,20,30,40}.contains(10,20)&&!Rect{10,20,30,40}.contains(40,20),"UI hit regions use the same bounds as drawing");
