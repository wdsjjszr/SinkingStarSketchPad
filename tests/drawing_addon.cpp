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
    drawing_wheel.reset();assert(drawing_wheel.consume(.5,0,input::WheelAction::Plane,true)==0&&drawing_wheel.consume(.5,0,input::WheelAction::Plane,true)==1);
    set_ground(camera_plane()+1);assert(camera_plane()==5.25);
}
void compile_legacy_backup_failure(const std::filesystem::path &draft){
    assert(!std::filesystem::exists(draft.parent_path()/L"drafts.pre-v4.txt"));
    data_path=draft;doc={};doc.format_version=2;legacy_draft_source=draft.parent_path()/L"missing-legacy-source";dirty=true;
    assert(!save()&&dirty&&doc.format_version==2);
}
void compile_world_export(Point size){export_svg(size);}
void compile_right_erase_transaction(){
    doc={};history={};data_path.clear();selected=Tool::Line;shape_gesture.clear();dragging=false;
    auto stroke=std::make_shared<Stroke>();stroke->points={{.2f,.2f},{.4f,.4f}};page().push_back(stroke);
    erase_before=page();erasing=true;erase_with_right=true;erase_at({200,200},{1000,1000});
    assert(page().empty()&&history[0].undo.empty());commit();assert(history[0].undo.size()==1&&selected==Tool::Line&&!erase_with_right);
    undo();assert(page().size()==1);
}
void compile_repeated_path_endpoint(){
    doc={};history={};data_path.clear();doc.grids[0]=native_ground::lattice(3.25);
    auto stroke=std::make_shared<Stroke>();stroke->tool=Tool::Path;stroke->lattice=doc.grids[0];stroke->points={{0,0,0}};page().push_back(stroke);path_active=true;
    camera.valid=true;camera.eye={0,0,10};camera.forward={0,0,-1};
    camera.matrix={.1,0,0,0,0,.1,0,0,0,0,-.1,1,0,0,0,1};camera.inverse={10,0,0,0,0,10,0,0,0,0,-10,10,0,0,0,1};
    path_click({500,500},{1000,1000});assert(!path_active&&page().size()==1&&history[0].undo.empty());
    doc.grids[0].origin.z=4.25;path_active=true;path_click({500,500},{1000,1000});assert(path_active&&page()[0]->points.back().z==1);
}
void compile_cross_height_shape(){
    doc={};history={};data_path.clear();doc.grids[0]=native_ground::lattice(3.25);pending={};pending.tool=Tool::Line;pending.lattice=doc.grids[0];pending.points={{0,0},{2,2}};
    dragging=true;shape_gesture.press();shape_gesture.release();set_ground(4.25,grid::HintSource::Wheel);
    assert(dragging&&shape_gesture.waiting()&&page().empty()&&pending.lattice.origin.z==3.25);
    camera.valid=true;camera.eye={0,0,10};camera.forward={0,0,-1};
    camera.matrix={.1,0,0,0,0,.1,0,0,0,0,-.1,1,0,0,0,1};camera.inverse={10,0,0,0,0,10,0,0,0,0,-10,10,0,0,0,1};
    Point target;assert(drawing_target(pending,{100,100},{1000,1000},false,target)&&target.z==1);
    pending.points[1]=target;shape_gesture.press();shape_gesture.release();commit();assert(page().size()==1&&page()[0]->points[1].z==1);
}
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
void compile_vertical_target_without_pointer_intersection(){
    doc={};doc.grids[0]=native_ground::lattice(4.25);pending={};pending.tool=Tool::Arrow;pending.lattice=native_ground::lattice(3.25);pending.points={{0,0},{1,1}};
    camera.valid=true;camera.eye={0,-10,4};camera.forward={0,1,0};
    camera.matrix={1,0,0,0,0,0,1,-4,0,1,0,9.9,0,1,0,10};assert(free_camera::inverse(camera.matrix,camera.inverse));
    Point target;assert(!drawing_target(pending,{500,500},{1000,1000},false,target));
    assert(drawing_target(pending,{500,500},{1000,1000},true,target)&&target.x==0&&target.y==0&&target.z==1);
    Point marker;assert(screen_point(pending.lattice,target,camera,{1000,1000},marker));
    pending.points[1]=target;assert(project_stroke(pending,camera,{1000,1000}).back().x==marker.x);
    camera.valid=false;assert(!drawing_target(pending,{500,500},{1000,1000},true,target));camera.valid=true;
    pending.points[0].y=20;assert(!drawing_target(pending,{500,500},{1000,1000},true,target));
    doc.grids[0].ready=false;assert(!drawing_target(pending,{500,500},{1000,1000},true,target));
}
