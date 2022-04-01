#pragma once
#include "Color.h"

namespace UI {

class Control {
public:
    Control(const ColorPalette& colors) : colors(colors) {}
    
    virtual bool hitTest(const Point& p) const {
        Rect f = frame;
        f.point.x -= hitTestExpand.l;
        f.size.x  += hitTestExpand.l;
        
        f.size.x  += hitTestExpand.r;
        
        f.point.y -= hitTestExpand.t;
        f.size.y  += hitTestExpand.t;
        
        f.size.y  += hitTestExpand.b;
        return HitTest(f, p);
    }
    
    virtual void layout(const Window& win) {}
    
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
    struct {
        int l = 0;
        int r = 0;
        int t = 0;
        int b = 0;
    } hitTestExpand;
};

} // namespace UI
