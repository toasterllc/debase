#pragma once
#include "Color.h"

namespace UI {

class Control {
public:
    Control(const ColorPalette& colors) : colors(colors) {}
    
    virtual bool hitTest(const Point& p, Size expand={0,0}) const {
        return HitTest(frame, p, expand);
    }
    
    virtual void layout() {}
    
    bool drawNeeded = true;
    virtual void draw(const Window& win) {
        assert(drawNeeded);
        drawNeeded = false;
    }
    
    virtual Event handleEvent(const Window& win, const Event& ev) {
        return ev;
    }
    
    const ColorPalette& colors;
    Rect frame;
};

} // namespace UI
