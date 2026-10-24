// Compile-only fixtures for the production gesture, history, plane and save paths.
#include "../src/addon.cpp"
#include <cassert>
void compile_shape_history(){
    doc={};history={};data_path.clear();dirty=false;
    pending={};pending.tool=Tool::Rectangle;pending.points={{.2f,.2f},{.4f,.4f}};
    dragging=true;shape_gesture.press();shape_gesture.release();
    commit();assert(page().empty()&&history[0].undo.empty()&&dragging);
    settle_drawing();assert(page().empty()&&!dragging);
    dragging=true;shape_gesture.press();shape_gesture.move(true);settle_drawing();
    assert(page().size()==1&&history[0].undo.size()==1);
    pending={};pending.tool=Tool::Line;pending.points={{.1f,.1f},{.6f,.6f}};
    dragging=true;shape_gesture.press();shape_gesture.release();shape_gesture.press();shape_gesture.release();commit();
    assert(page().size()==2&&history[0].undo.size()==2);
    undo();assert(page().size()==1);redo();assert(page().size()==2);
}
void compile_plane_capture(){
    doc={};history={};data_path.clear();shape_gesture.clear();dragging=false;
    auto stroke=std::make_shared<Stroke>();stroke->lattice=native_ground::lattice(3.25);stroke->points={{-1.25f,2},{3.5f,4}};
    page().push_back(stroke);set_ground(4.25);assert(page()[0]->lattice.origin.z==3.25&&camera_plane()==4.25);
    plane_wheel.remainder=0;assert(plane_wheel.consume(.5,true)==0&&plane_wheel.consume(.5,true)==1);
    set_ground(camera_plane()+1);assert(camera_plane()==5.25);
}
void compile_legacy_backup_failure(const std::filesystem::path &draft){
    assert(!std::filesystem::exists(draft.parent_path()/L"drafts.pre-v3.txt"));
    data_path=draft;doc={};doc.format_version=2;legacy_draft_source=draft.parent_path()/L"missing-legacy-source";dirty=true;
    assert(!save()&&dirty&&doc.format_version==2);
}
void compile_world_export(Point size){export_svg(size);}
void compile_settings_roundtrip(const std::filesystem::path &isolated_directory){
    data_path=isolated_directory/L"drafts.txt";auto ini=isolated_directory/L"settings.ini";
    WritePrivateProfileStringW(L"Interface",L"Language",L"en",ini.c_str());
    hint_preferences.mode=grid::HintMode::WheelOnly;hint_preferences.duration_ms=1250;
    hint_preferences.style={true,true,true,false,false,37};wheel_actions[0]=input::WheelAction::Color;
    drawing_preferences_dirty=true;save_drawing_preferences();assert(!drawing_preferences_dirty);
    hint_preferences={};wheel_actions=input::default_wheel;load_drawing_preferences();
    assert(hint_preferences.duration_ms==1250&&hint_preferences.mode==grid::HintMode::WheelOnly&&hint_preferences.style.opacity==37);
    assert(hint_preferences.style.centers&&hint_preferences.style.intersections&&!hint_preferences.style.borders&&!hint_preferences.style.checker);
    assert(wheel_actions[0]==input::WheelAction::Color);
    wchar_t language[16];GetPrivateProfileStringW(L"Interface",L"Language",L"",language,16,ini.c_str());assert(std::wstring(language)==L"en");
}
