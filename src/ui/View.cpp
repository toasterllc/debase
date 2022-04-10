#include "Window.h"
#include "Screen.h"
#include "lib/toastbox/Defer.h"

namespace UI {

WINDOW* View::_gstateWindow() const {
    assert(_GState);
    return *_GState.window;
}

void View::track(const Event& ev) {
    assert(_GState);
    assert(!_tracking);
    
    Screen& screen = *_GState.screen;
    const GraphicsState gstate = screen.graphicsStateCalc(*this);
    if (!gstate) throw std::runtime_error("graphicsStateCalc() failed");
    
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
        
        handleEventTree(gstate, _eventCurrent);
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
//            handleEventTree(*this, {}, _eventCurrent);
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
