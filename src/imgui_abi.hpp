#pragma once
// ReShade is built with MSVC. For ImVec2 (a non-POD 8-byte return value),
// MSVC uses a hidden output pointer while the MinGW ABI returns a register.
// Keep this boundary explicit; never call an ImVec2-returning API directly
// from a MinGW build. ImVec4 is 16 bytes and uses sret with both compilers.
inline ImVec2 gui_vec2(ImVec2 (*fn)()) {
#if defined(__MINGW32__)
    ImVec2 value;
    reinterpret_cast<void (*)(ImVec2 *)>(fn)(&value);
    return value;
#else
    return fn();
#endif
}
inline ImVec2 gui_text_size(const char *text,float wrap=-1.f) {
#if defined(__MINGW32__)
    ImVec2 value;
    reinterpret_cast<void (*)(ImVec2 *,const char *,const char *,bool,float)>(imgui_function_table_instance()->CalcTextSize)(&value,text,nullptr,false,wrap);
    return value;
#else
    return ImGui::CalcTextSize(text,nullptr,false,wrap);
#endif
}
