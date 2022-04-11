#include "Window.h"
#include "Screen.h"
#include "lib/toastbox/Defer.h"

namespace UI {

WINDOW* View::_gstateWindow() const {
    assert(_GState);
    return *_GState.window;
}

void View::track(const Event& ev) {
    assert(!_tracking);
    
    class Screen& screen = Screen();
    GraphicsState gstate;
    
//        if (target) {
//            gstate = find(*target, gstate, *this);
//            if (!gstate) throw std::runtime_error("couldn't find target view");
//        
//        } else {
//            target = this;
//        }
    
    _tracking = true;
    Defer(_tracking = false); // Exception safety
    Defer(_trackStop = false); // Exception safety
    
    do {
        _eventCurrent = screen.nextEvent();
        Defer(_eventCurrent = {}); // Exception safety
        
        // Get our gstate after calling nextEvent first, so that the gstate is calculated after
        // a full render pass (which occurs at the start of nextEvent), instead of before.
        // This is necessary when tracking eg menus that ncurses shifts away from the edge of
        // the window. In this case, the origin that's initially set for the menu changes
        // during the layout pass, and so the gstate calculated before layout is incorrect.
        if (!gstate) {
            gstate = screen.graphicsStateCalc(*this);
            if (!gstate) throw std::runtime_error("graphicsStateCalc() failed");
        }
        
        handleEvent(gstate, _eventCurrent);
    } while (!_trackStop);
    
    
    
//        if (!view.visible()) return;
//        if (!view.interaction()) return;
//        
//        gstate.origin += view.origin();
//        
//        if (!target || &view==target) {
//            _tracking = true;
//            Defer(_tracking = false); // Exception safety
//            Defer(_trackStop = false); // Exception safety
//            
//            do {
//                refresh();
//                
//                _eventCurrent = UI::NextEvent();
//                Defer(_eventCurrent = {}); // Exception safety
//                
//                HandleEventTree(gstate, view);
//            } while (!_trackStop);
//        
//        } else {
//            auto it = view.subviews();
//            for (;;) {
//                ViewPtr subview = view.subviewsNext(it);
//                if (!subview) break;
//                track(target, gstate, subview);
//            }
//        }
//        
//        
//        
//        
//        
//        
//        
//        
//        
//        
////        _TreeState treeState(_TState, win, {});
//        
//        _tracking = true;
//        Defer(_tracking = false); // Exception safety
//        Defer(_trackStop = false); // Exception safety
//        
//        do {
//            refresh();
//            
//            _eventCurrent = UI::NextEvent();
//            Defer(_eventCurrent = {}); // Exception safety
//            
//            handleEvent(*this, {}, _eventCurrent);
//        } while (!_trackStop);
}



//void View::track(const Event& ev) {
//    assert(_GState);
//    assert(!_tracking);
//    
//    _tracking = true;
//    Defer(_tracking = false); // Exception safe
//    
//    _GState.screen->track(*this, ev);
//}
//
//void View::trackStop() {
//    assert(_GState);
//    assert(_tracking);
//    _GState.screen->trackStop();
//}


} // namespace UI
