#pragma once

namespace sketch::input {
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
