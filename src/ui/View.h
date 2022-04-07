#pragma once
#include "Color.h"

namespace UI {

class View {
public:
    View(const ColorPalette& colors) : _colors(colors) {}
    virtual ~View() = default;
    
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
    
    virtual Point origin() const { return _frame.origin; }
    virtual void origin(const Point& x) {
        if (_frame.origin == x) return;
        _frame.origin = x;
        drawNeeded(true);
    }
    
    virtual Size size() const { return _frame.size; }
    virtual void size(const Size& x) {
        if (_frame.size == x) return;
        _frame.size = x;
        layoutNeeded(true);
        drawNeeded(true);
    }
    
    virtual Rect frame() const {
        return _frame;
    }
    
    virtual void frame(const Rect& x) {
        if (_frame == x) return;
        _frame = x;
        layoutNeeded(true);
        drawNeeded(true);
    }
    
    // MARK: - Accessors
    const auto& colors() const { return _colors; }
    
    const auto& hidden() const { return _hidden; }
    template <typename T> void hidden(const T& x) { _set(_hidden, x); }
    
    // MARK: - Layout
    virtual bool layoutNeeded() const { return _layoutNeeded; }
    virtual void layoutNeeded(bool x) { _layoutNeeded = x; }
    virtual void layout(const Window& win) {}
    
    // MARK: - Draw
    virtual bool drawNeeded() const { return _drawNeeded; }
    virtual void drawNeeded(bool x) { _drawNeeded = x; }
    virtual void draw(const Window& win) {}
    
    // MARK: - Events
    virtual bool handleEvent(const Window& win, const Event& ev) { return false; }
    
    virtual View*const* subviews() { return _subviews; }
    
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
    View*const _subviews[1] = { nullptr };
    Rect _frame;
    bool _hidden = false;
    bool _layoutNeeded = true;
    bool _drawNeeded = true;
};

} // namespace UI
