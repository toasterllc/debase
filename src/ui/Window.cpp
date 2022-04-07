#include "Window.h"
#include "View.h"

namespace UI {

static void _layout(const Window& win, View* v) {
    if (v->hidden()) return;
    
    if (v->layoutNeeded()) {
        v->layout(win);
        v->layoutNeeded(false);
    }
    
    for (auto subviews=v->subviews(); *subviews; subviews++) {
        _layout(win, *subviews);
    }
}

void Window::_layout() {
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
        ::UI::_layout(*this, *views);
    }
    
    layoutNeeded(false);
}

static void _draw(const Window& win, View* v) {
    if (v->hidden()) return;
    
    // If the window was erased during this draw cycle, we need to redraw
    if (v->drawNeeded() || win.erased()) {
        v->draw(win);
        v->drawNeeded(false);
    }
    
    for (auto subviews=v->subviews(); *subviews; subviews++) {
        ::UI::_draw(win, *subviews);
    }
}

void Window::_draw() {
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
        ::UI::_draw(*this, *views);
    }
    
    drawNeeded(false);
}

static bool _handleEvent(const Window& win, View* v, const Event& ev) {
    if (v->hidden()) return false;
    if (v->handleEvent(win, ev)) return true;
    for (auto subviews=v->subviews(); *subviews; subviews++) {
        if (_handleEvent(win, *subviews, ev)) return true;
    }
    return false;
}

bool Window::_handleEvent(const Event& ev) {
    // Let the window handle the event first
    if (handleEvent(ev)) return true;
    
    // Let the subviews handle the event
    for (auto views=subviews(); *views; views++) {
        if (::UI::_handleEvent(*this, *views, ev)) return true;
    }
    return false;
}

} // namespace UI
