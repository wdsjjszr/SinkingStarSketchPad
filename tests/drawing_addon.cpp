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
