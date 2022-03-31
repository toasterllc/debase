#pragma once
#include "Window.h"

namespace UI {

class Attr {
public:
    Attr() {}
    
    Attr(int attr) : _attr(attr) {
        attron(_attr);
    }
    
    Attr(const Attr& x) = delete;
    
    // Move constructor
    Attr(Attr&& x) {
        std::swap(_attr, x._attr);
    }
    
    // Move assignment operator
    Attr& operator=(Attr&& x) {
        std::swap(_attr, x._attr);
        return *this;
    }
    
    ~Attr() {
        if (_attr) {
            attroff(_attr);
        }
    }

private:
    int _attr = 0;
};

} // namespace UI
