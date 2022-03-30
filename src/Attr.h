#pragma once
#include "Window.h"

namespace UI {

class Attr {
public:
    Attr() {}
    
    Attr(Window win, int attr) : _s({.win=win, .attr=attr}) {
        wattron(*_s.win, _s.attr);
    }
    
    Attr(const Attr& x) = delete;
    
    // Move constructor: use move assignment operator
    Attr(Attr&& x) { *this = std::move(x); }
    
    // Move assignment operator
    Attr& operator=(Attr&& x) {
        _s = std::move(x._s);
        x._s = {};
        return *this;
    }
    
    ~Attr() {
        if (_s.win) {
            wattroff(*_s.win, _s.attr);
        }
    }

private:
    struct {
        Window win;
        int attr = 0;
    } _s;
};

} // namespace UI
