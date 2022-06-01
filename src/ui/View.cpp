#include "Window.h"
#include "Screen.h"
#include "lib/toastbox/Defer.h"

namespace UI {

const ColorPalette& View::colors() const {
    return screen().colors();
}

bool View::cursorState(const CursorState& x) {
    assert(_GState);
    CursorState xcopy = x;
    xcopy.origin += _GState.originScreen;
    return screen().cursorState(xcopy);
}

bool View::enabledWindow() const {
    return _enabled && window().enabled();
}

void View::track(Deadline deadline) {
    using namespace std::chrono;
    
    class Screen& scr = screen();
    GraphicsState gstate;
    
    const bool trackingPrev = _tracking;
    const bool trackStopPrev = _trackStop;
    
    _tracking = true;
    _trackStop = false;
    
    // Exception-safe state restore
    Defer(
        _tracking = trackingPrev;
        _trackStop = trackStopPrev;
    );
    
    do {
        // Copy the event! Subviews could recursively call track(), which will
        // update Screen's ivar, so we don't want to use a reference here.
        const Event ev = scr.eventNext(deadline);
        if (!ev) return; // Deadline passed
        
        // Get our gstate after calling nextEvent first, so that the gstate is calculated after
        // a full render pass (which occurs at the start of nextEvent), instead of before.
        // This is necessary when tracking eg menus that ncurses shifts away from the edge of
        // the window. In this case, the origin that's initially set for the menu changes
        // during the layout pass, and so the gstate calculated before layout is incorrect.
        if (!gstate) {
            gstate = scr.graphicsStateCalc(*this);
            if (!gstate) throw std::runtime_error("graphicsStateCalc() failed");
        }
        
        handleEvent(gstate, ev);
    } while (!_trackStop && deadline!=Once);
}


WINDOW* View::_window() const {
    return (WINDOW*)window();
}

} // namespace UI
