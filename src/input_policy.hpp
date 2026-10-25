#pragma once
#include <array>

namespace sketch::input {
enum class WheelAction {None,Plane,Color,Tool};
enum class RightAction {None,CancelShape,FinishPath,Erase};
constexpr RightAction right_action(bool canvas,bool shape,bool path){return !canvas?RightAction::None:shape?RightAction::CancelShape:path?RightAction::FinishPath:RightAction::Erase;}
inline constexpr std::array<WheelAction,4> default_wheel={WheelAction::Plane,WheelAction::None,WheelAction::Color,WheelAction::Tool};
constexpr int wheel_binding(bool ctrl,bool shift,bool alt){return int(ctrl)+int(shift)+int(alt)>1?-1:ctrl?1:shift?2:alt?3:0;}
constexpr unsigned cycle_index(unsigned current,int steps,unsigned count){int next=(int(current)-steps)%int(count);return unsigned(next<0?next+int(count):next);}
struct WheelState {
    double remainder=0;
    int binding=-1;
    WheelAction action=WheelAction::None;
    constexpr void reset(){remainder=0;binding=-1;action=WheelAction::None;}
    constexpr int consume(double wheel,int selected_binding,WheelAction selected_action,bool enabled){
        if(!enabled||selected_binding<0||selected_action==WheelAction::None){reset();return 0;}
        if(binding!=selected_binding||action!=selected_action){reset();binding=selected_binding;action=selected_action;}
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
