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
int main(){}
