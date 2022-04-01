#pragma once
#include "Color.h"

namespace UI {

class Control {
public:
    Control(const ColorPalette& colors) : colors(colors) {}
    
    virtual bool hitTest(const UI::Point& p, UI::Size expand={0,0}) const {
        return UI::HitTest(frame, p, expand);
    }
    
    virtual void layout() {}
    
    bool drawNeeded = true;
    virtual void draw(const Window& win) {
        assert(drawNeeded);
        drawNeeded = false;
    }
    
    const ColorPalette& colors;
    Rect frame;
};

} // namespace UI
