#include "../src/shape_input.hpp"
using sketch::input::ShapeGesture;
constexpr bool gestures(){
    ShapeGesture shape;shape.press();if(shape.can_commit()||shape.release()||!shape.waiting())return false;
    if(shape.can_commit()||shape.release())return false;
    shape.press();if(shape.can_commit()||!shape.release()||shape.active())return false;
    shape.press();shape.move(true);shape.move(false);if(!shape.can_commit()||!shape.release())return false;
    shape.press();shape.release();shape.clear();return !shape.active()&&shape.can_commit();
}
static_assert(gestures(),"Drag-release and click-click commit exactly once; preview clicks never commit through Save, and cancellation clears the gesture");
constexpr bool pause_for_plane(){
    ShapeGesture shape;shape.await_end();if(shape.active())return false;
    shape.press();shape.move(true);shape.await_end();if(!shape.waiting()||shape.release()||shape.can_commit())return false;
    shape.press();return shape.release()&&!shape.active();
}
static_assert(pause_for_plane(),"Middle marking converts a drag to click-click completion; releasing held left cannot commit it");
int main(){}
