#include "../src/input_policy.hpp"
#include <cassert>
#include <iostream>
using namespace sketch::input;
constexpr bool plane_wheel_checks(){
    PlaneWheel wheel;
    if(wheel.consume(.25,true)||wheel.consume(.5,true)||wheel.consume(.25,true)!=1)return false;
    if(wheel.consume(-2.5,true)!=-2||wheel.consume(-.5,true)!=-1)return false;
    wheel.consume(.5,true);wheel.consume(1,false);
    return wheel.remainder==0&&wheel.consume(3,true)==3;
}
static_assert(plane_wheel_checks(),"Whole wheel steps move planes; fractions accumulate and UI/focus ownership resets the remainder");
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
