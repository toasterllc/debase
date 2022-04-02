#pragma once
#include "Color.h"

namespace UI {

class Control {
public:
    Control(const ColorPalette& colors) : colors(colors) {}
    
    virtual bool hitTest(const Point& p) const {
        Rect f = _frame;
        f.point.x -= hitTestExpand.l;
        f.size.x  += hitTestExpand.l;
        
        f.size.x  += hitTestExpand.r;
        
        f.point.y -= hitTestExpand.t;
        f.size.y  += hitTestExpand.t;
        
        f.size.y  += hitTestExpand.b;
        return HitTest(f, p);
    }
    
    virtual Size sizeIntrinsic() { return size(); }
    
    Point position() const { return _frame.point; }
    void position(const Point& x) {
        if (_frame.point == x) return;
        _frame.point = x;
        drawNeeded = true;
    }
    
    Size size() const { return _frame.size; }
    void size(const Size& x) {
        if (_frame.size == x) return;
        _frame.size = x;
        layoutNeeded = true;
        drawNeeded = true;
    }
    
    Rect frame() const {
        return _frame;
    }
    
    void frame(const Rect& x) {
        if (_frame == x) return;
        _frame = x;
        layoutNeeded = true;
        drawNeeded = true;
    }
    
    bool layoutNeeded = true;
    virtual void layout(const Window& win) {
        assert(layoutNeeded); // For debugging unnecessary layout
        layoutNeeded = false;
    }
    
    bool drawNeeded = true;
    virtual void draw(const Window& win) {
        assert(drawNeeded); // For debugging unnecessary layout
        drawNeeded = false;
    }
    
    virtual bool handleEvent(const Window& win, const Event& ev) {
        return false;
    }
    
    const ColorPalette& colors;
    struct {
        int l = 0;
        int r = 0;
        int t = 0;
        int b = 0;
    } hitTestExpand;

private:
    Rect _frame;
};

} // namespace UI
