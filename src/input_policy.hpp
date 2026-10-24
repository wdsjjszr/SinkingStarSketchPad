#pragma once
#include <array>

namespace sketch::input {
enum class WheelAction {None,Plane,Color};
inline constexpr std::array<WheelAction,4> default_wheel={WheelAction::Plane,WheelAction::None,WheelAction::Color,WheelAction::None};
struct PlaneWheel {
    double remainder=0;
    constexpr int consume(double wheel,bool enabled){
        if(!enabled){remainder=0;return 0;}
        remainder+=wheel;int steps=int(remainder);remainder-=steps;return steps;
    }
};
// Keep editing intent across focus changes, but never carry a held mouse
// button into the game after Alt+Tab or a click used to activate its window.
struct Focus {
    bool active=false, wait_for_release=true;
    constexpr bool update(bool foreground,bool mouse_down) {
        const bool lost=active&&!foreground;
        if(!foreground||!active)wait_for_release=true;
        active=foreground;
        if(foreground&&!mouse_down)wait_for_release=false;
        return lost;
    }
    constexpr bool canvas_ready() const {return active&&!wait_for_release;}
};
inline bool tool_shortcuts(bool ctrl,bool shift,bool alt){return alt&&!ctrl&&!shift;}
inline bool capture_keyboard(bool text,bool ctrl,bool alt){return text||ctrl||alt;}
}
