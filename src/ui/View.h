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
    
    const auto& visible() const { return _visible; }
    template <typename T> void visible(const T& x) { _set(_visible, x); }
    
    const auto& interaction() const { return _interaction; }
    template <typename T> void interaction(const T& x) { _set(_interaction, x); }
    
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
    
    static void TreeLayout(const Window& win, View& view) {
        if (!view.visible()) return;
        
        if (view.layoutNeeded()) {
            view.layout(win);
            view.layoutNeeded(false);
        }
        
        for (auto subviews=view.subviews(); *subviews; subviews++) {
            TreeLayout(win, *(*subviews));
        }
    }
    
    static void TreeDraw(const Window& win, View& v) {
        if (!v.visible()) return;
        
        // If the window was erased during this draw cycle, we need to redraw
        if (v.drawNeeded() || win.erased()) {
            v.draw(win);
            v.drawNeeded(false);
        }
        
        for (auto subviews=v.subviews(); *subviews; subviews++) {
            TreeDraw(win, *(*subviews));
        }
    }
    
    static bool TreeHandleEvent(const Window& win, View& view, const Event& ev) {
        if (!view.visible()) return false;
        if (!view.interaction()) return false;
        // Let the subviews handle the event first
        for (auto subviews=view.subviews(); *subviews; subviews++) {
            if (TreeHandleEvent(win, *(*subviews), ev)) return true;
        }
        // None of the subviews wanted the event; let the window handle it
        if (view.handleEvent(win, ev)) return true;
        return false;
    }
    
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
    bool _visible = true;
    bool _interaction = true;
    bool _layoutNeeded = true;
    bool _drawNeeded = true;
};

} // namespace UI
