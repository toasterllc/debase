#include "Window.h"
#include "View.h"

namespace UI {

void Window::layoutTree() {
    // Detect size changes that can occurs from underneath us
    // by ncurses (eg by the terminal size changing)
    if (_s.sizePrev != size()) {
        // We need to erase (and redraw) after resizing
        eraseNeeded(true);
        _s.sizePrev = size();
    }
    
    // Layout the window itself
    layout();
    
    // Layout the window's subviews
    for (auto views=subviews(); *views; views++) {
        (*views)->layoutTree(*this);
    }
    
    layoutNeeded(false);
}

void Window::drawTree() {
    // Remember whether we erased ourself during this draw cycle
    // This is used by View instances (Button and TextField)
    // to know whether they need to be drawn again
    _s.erased = _s.eraseNeeded;
    
    // Erase ourself if needed, and remember that we did so
    if (_s.eraseNeeded) {
        ::werase(*this);
        _s.eraseNeeded = false;
    }
    
    // Draw the window itself
    draw();
    
    // Draw the window's subviews
    for (auto views=subviews(); *views; views++) {
        (*views)->drawTree(*this);
    }
    
    drawNeeded(false);
}

bool Window::handleEventTree(const Event& ev) {
    // Let the subviews handle the event first
    for (auto views=subviews(); *views; views++) {
        if ((*views)->handleEventTree(*this, ev)) return true;
    }
    // None of the subviews wanted the event; let the window handle it
    if (handleEvent(ev)) return true;
    return false;
}

} // namespace UI
