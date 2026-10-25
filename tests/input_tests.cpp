#include "../src/input_policy.hpp"
#include <cassert>
#include <iostream>
using namespace sketch::input;
static_assert(right_action(true,true,true)==RightAction::CancelShape&&right_action(true,false,true)==RightAction::FinishPath&&right_action(true,false,false)==RightAction::Erase&&right_action(false,true,true)==RightAction::None,"Right-click cancels shapes, finishes paths, then erases only on an available canvas");
constexpr bool drawing_wheel_checks(){
    WheelState wheel;
    if(wheel.consume(.25,0,WheelAction::Plane,true)||wheel.consume(.5,0,WheelAction::Plane,true)||wheel.consume(.25,0,WheelAction::Plane,true)!=1)return false;
    if(wheel.consume(-2.5,0,WheelAction::Plane,true)!=-2||wheel.consume(-.5,0,WheelAction::Plane,true)!=-1)return false;
    wheel.consume(.5,0,WheelAction::Plane,true);if(wheel.consume(.5,2,WheelAction::Color,true))return false;
    if(wheel.consume(.5,2,WheelAction::Color,true)!=1)return false;
    wheel.consume(.5,2,WheelAction::Color,true);if(wheel.consume(.5,2,WheelAction::Plane,true))return false;
    wheel.consume(1,2,WheelAction::Plane,false);if(wheel.remainder!=0||wheel.binding!=-1)return false;
    wheel.consume(.5,0,WheelAction::Plane,true);wheel.consume(1,-1,WheelAction::Plane,true);
    if(wheel.remainder!=0||wheel.consume(3,0,WheelAction::Plane,true)!=3)return false;
    wheel.consume(.5,0,WheelAction::Color,true);wheel.consume(1,0,WheelAction::None,true);return wheel.remainder==0;
}
static_assert(drawing_wheel_checks(),"Fractional steps only combine within the same enabled binding/action; UI, multiple modifiers and disabled actions reset them");
static_assert(wheel_binding(false,false,false)==0&&wheel_binding(true,false,false)==1&&wheel_binding(false,true,false)==2&&wheel_binding(false,false,true)==3,"Four independent modifier bindings");
static_assert(wheel_binding(true,true,false)==-1&&wheel_binding(true,false,true)==-1&&wheel_binding(false,true,true)==-1&&wheel_binding(true,true,true)==-1,"Multiple modifiers never dispatch an action");
static_assert(default_wheel[0]==WheelAction::Plane&&default_wheel[1]==WheelAction::None&&default_wheel[2]==WheelAction::Color&&default_wheel[3]==WheelAction::None,"Default wheel bindings preserve height adjustment and add Shift color cycling");
static_assert(cycle_color(4,1,5)==0&&cycle_color(0,-1,5)==4&&cycle_color(2,12,5)==4&&cycle_color(2,-12,5)==0,"Palette wraps in both directions, including multiple steps");
constexpr bool held_middle_focus(){
    Focus focus;focus.update(true,true);if(focus.canvas_ready())return false;
    focus.update(true,false);if(!focus.canvas_ready())return false;
    focus.update(false,true);focus.update(true,true);if(focus.canvas_ready())return false;
    focus.update(true,false);return focus.canvas_ready();
}
static_assert(held_middle_focus(),"A held mouse button, including middle, cannot mark while reactivating the window");
int main(){
    Focus f;
    assert(!f.update(true,true)&&!f.canvas_ready()); // click to activate
    assert(!f.update(true,false)&&f.canvas_ready());
    assert(f.update(false,true)&&!f.canvas_ready()); // Alt+Tab mid-stroke
    assert(!f.update(false,false)&&!f.canvas_ready());
    assert(!f.update(true,true)&&!f.canvas_ready());
    assert(!f.update(true,false)&&f.canvas_ready());
    assert(!f.update(true,true)&&f.canvas_ready()); // new intentional stroke
    assert(f.update(false,false));
    assert(!f.update(true,false)&&f.canvas_ready()); // keyboard reactivation
    assert(!capture_keyboard(false,false,false)); // movement, even after UI click
    assert(capture_keyboard(true,false,false)); // text must not move the player
    assert(capture_keyboard(false,true,false));
    assert(capture_keyboard(false,false,true));
    assert(!tool_shortcuts(false,false,false)); // A/R/etc remain game controls
    assert(tool_shortcuts(false,false,true));
    assert(!tool_shortcuts(true,false,true));
    assert(!tool_shortcuts(false,true,true));
    std::cout<<"PASS: focus transitions, click reactivation, keyboard passthrough and tool chords\n";
}
