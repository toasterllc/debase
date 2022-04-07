#pragma once
#include "Color.h"

namespace UI {

class View {
public:
    View(const ColorPalette& colors) : _colors(colors) {}
    
    virtual bool hitTest(const Point& p) const {
        Rect f = _frame;
        f.origin.x -= hitTestExpand.l;
        f.size.x   += hitTestExpand.l;
        
        f.size.x   += hitTestExpand.r;
        
        f.origin.y -= hitTestExpand.t;
        f.size.y   += hitTestExpand.t;
        
        f.size.y   += hitTestExpand.b;
        return HitTest(f, p);
    }
    
    virtual Size sizeIntrinsic(Size constraint) { return size(); }
    
    Point origin() const { return _frame.origin; }
    void origin(const Point& x) {
        if (_frame.origin == x) return;
        _frame.origin = x;
        drawNeeded(true);
    }
    
    Size size() const { return _frame.size; }
    void size(const Size& x) {
        if (_frame.size == x) return;
        _frame.size = x;
        layoutNeeded(true);
        drawNeeded(true);
    }
    
    Rect frame() const {
        return _frame;
    }
    
    void frame(const Rect& x) {
        if (_frame == x) return;
        _frame = x;
        layoutNeeded(true);
        drawNeeded(true);
    }
    
    virtual bool layoutNeeded() const { return _layoutNeeded; }
    virtual void layoutNeeded(bool x) { _layoutNeeded = x; }
    virtual bool layout(const Window& win) {
        if (layoutNeeded()) {
            layoutNeeded(false);
            return true;
        }
        return false;
    }
    
    virtual bool drawNeeded() const { return _drawNeeded; }
    virtual void drawNeeded(bool x) { _drawNeeded = x; }
    virtual bool draw(const Window& win) {
        // If the window was erased during this draw cycle, we need to redraw
        if (drawNeeded() || win.erased()) {
            drawNeeded(false);
            return true;
        }
        return false;
    }
    
    virtual bool handleEvent(const Window& win, const Event& ev) {
        return false;
    }
    
    const auto& colors() const { return _colors; }
    
    struct {
        int l = 0;
        int r = 0;
        int t = 0;
        int b = 0;
    } hitTestExpand;
    
protected:
    template <typename X, typename Y>
    void _setAlways(X& x, const Y& y) {
        x = y;
        View::drawNeeded(true);
    }
    
    template <typename X, typename Y>
    void _set(X& x, const Y& y) {
        if (x == y) return;
        x = y;
        View::drawNeeded(true);
    }
    
private:
    const ColorPalette& _colors;
    
    Rect _frame;
    bool _layoutNeeded = true;
    bool _drawNeeded = true;
};

} // namespace UI
