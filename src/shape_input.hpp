#pragma once

namespace sketch::input {
struct ShapeGesture {
    enum class Phase {Idle,Pressed,Drag,AwaitEnd,SecondPress};
    Phase phase=Phase::Idle;
    constexpr bool active()const{return phase!=Phase::Idle;}
    constexpr bool waiting()const{return phase==Phase::AwaitEnd;}
    constexpr bool can_commit()const{return phase==Phase::Idle||phase==Phase::Drag;}
    constexpr void clear(){phase=Phase::Idle;}
    constexpr void await_end(){if(active())phase=Phase::AwaitEnd;}
    constexpr void press(){phase=waiting()?Phase::SecondPress:Phase::Pressed;}
    constexpr void move(bool crossed_threshold){if(phase==Phase::Pressed&&crossed_threshold)phase=Phase::Drag;}
    constexpr bool release(){
        if(phase==Phase::Pressed){phase=Phase::AwaitEnd;return false;}
        if(phase==Phase::Drag||phase==Phase::SecondPress){clear();return true;}
        return false;
    }
};
}
