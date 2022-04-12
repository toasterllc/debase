#include "Window.h"
#include "Screen.h"
#include "lib/toastbox/Defer.h"

namespace UI {

bool View::cursorState(CursorState x) {
    assert(_GState);
    x.origin += _GState.originScreen;
    return screen().cursorState(x);
}

void View::track(const Event& ev, Deadline deadline) {
    using namespace std::chrono;
    
    class Screen& scr = screen();
    GraphicsState gstate;
    
//        if (target) {
//            gstate = find(*target, gstate, *this);
//            if (!gstate) throw std::runtime_error("couldn't find target view");
//        
//        } else {
//            target = this;
//        }
    
    const bool trackingPrev = _tracking;
    const bool trackStopPrev = _trackStop;
    const Event eventCurrentPrev = _eventCurrent;
    
    _tracking = true;
    _trackStop = false;
    
    // Exception-safe state restore
    Defer(
        _tracking = trackingPrev;
        _trackStop = trackStopPrev;
        _eventCurrent = eventCurrentPrev;
    );
    
    do {
        _eventCurrent = scr.nextEvent(deadline);
        if (!_eventCurrent) return; // Deadline passed
        
        // Get our gstate after calling nextEvent first, so that the gstate is calculated after
        // a full render pass (which occurs at the start of nextEvent), instead of before.
        // This is necessary when tracking eg menus that ncurses shifts away from the edge of
        // the window. In this case, the origin that's initially set for the menu changes
        // during the layout pass, and so the gstate calculated before layout is incorrect.
        if (!gstate) {
            gstate = scr.graphicsStateCalc(*this);
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


WINDOW* View::_window() const {
    return (WINDOW*)window();
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
