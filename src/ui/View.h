#pragma once
#include <optional>
#include "Color.h"
#include "lib/toastbox/Defer.h"

namespace UI {

class Window;

class View {
public:
    using Ptr = std::shared_ptr<View>;
    using WeakPtr = std::weak_ptr<View>;
    
    struct HitTestExpand {
        int l = 0;
        int r = 0;
        int t = 0;
        int b = 0;
    };
    
    static const ColorPalette& Colors() { return _Colors; }
    static void Colors(const ColorPalette& x) { _Colors = x; }
    
    virtual ~View() = default;
    
    virtual bool hitTest(const Point& p) const {
        Rect f = frame();
        f.origin.x -= _hitTestExpand.l;
        f.size.x   += _hitTestExpand.l;
        
        f.size.x   += _hitTestExpand.r;
        
        f.origin.y -= _hitTestExpand.t;
        f.size.y   += _hitTestExpand.t;
        
        f.size.y   += _hitTestExpand.b;
        return HitTest(f, p);
    }
    
    virtual Size sizeIntrinsic(Size constraint) { return size(); }
    
    virtual Point origin() const { return _origin; }
    virtual void origin(const Point& x) {
        if (_origin == x) return;
        _origin = x;
        drawNeeded(true);
    }
    
    virtual Size size() const { return _size; }
    virtual void size(const Size& x) {
        if (_size == x) return;
        _size = x;
        layoutNeeded(true);
        drawNeeded(true);
    }
    
    virtual Rect frame() const { return { .origin=origin(), .size=size() }; }
    virtual void frame(const Rect& x) {
        if (frame() == x) return;
        origin(x.origin);
        size(x.size);
    }
    
    virtual Rect bounds() const { return { .size = size() }; }
    
    // MARK: - Accessors
    virtual bool visible() const { return _visible; }
    virtual void visible(bool x) { _set(_visible, x); }
    
    virtual const bool interaction() const { return _interaction; }
    virtual void interaction(bool x) { _set(_interaction, x); }
    
    virtual const std::optional<Color> borderColor() const { return _borderColor; }
    virtual void borderColor(std::optional<Color> x) { _set(_borderColor, x); }
    
    virtual const HitTestExpand& hitTestExpand() const { return _hitTestExpand; }
    virtual void hitTestExpand(const HitTestExpand& x) { _setForce(_hitTestExpand, x); }
    
    // MARK: - Layout
    virtual bool layoutNeeded() const { return _layoutNeeded; }
    virtual void layoutNeeded(bool x) { _layoutNeeded = x; }
    virtual void layout(const Window& win) {}
    
    // MARK: - Draw
    virtual bool drawNeeded() const { return _drawNeeded; }
    virtual void drawNeeded(bool x) { _drawNeeded = x; }
    virtual void draw(const Window& win) {}
    virtual void drawBackground(const Window& win);
    
    // MARK: - Events
    virtual bool handleEvent(const Window& win, const Event& ev) { return false; }
    
    template <typename T, typename ...T_Args>
    std::shared_ptr<T> createSubview(T_Args&&... args) {
        auto view = std::make_shared<T>(std::forward<T_Args>(args)...);
        _subviews.push_back(view);
        layoutNeeded(true);
        return view;
    }
    
    virtual std::list<WeakPtr>& subviews() { return _subviews; }
    
    virtual void layoutTree(const Window& win) {
        if (!visible()) return;
        
        if (layoutNeeded()) {
            layout(win);
            layoutNeeded(false);
        }
        
        for (auto it=_subviews.begin(); it!=_subviews.end();) {
            Ptr view = (*it).lock();
            if (!view) {
                // Prune and continue
                it = _subviews.erase(it);
                continue;
            }
            
            view->layoutTree(win);
            it++;
        }
    }
    
    virtual void drawTree(const Window& win) {
        if (!visible()) return;
        
        // If the window was erased during this draw cycle, we need to redraw.
        // _winErased() has to be implemented out-of-line because:
        // 
        //   Window.h includes View.h
        //   ∴ View.h can't include Window.h
        //   ∴ View has to forward declare Window
        //   ∴ we can't call Window functions in View.h
        //
        if (drawNeeded() || _winErased(win)) {
            drawBackground(win);
            draw(win);
            drawNeeded(false);
        }
        
        for (auto it=_subviews.begin(); it!=_subviews.end();) {
            Ptr view = (*it).lock();
            if (!view) {
                // Prune and continue
                it = _subviews.erase(it);
                continue;
            }
            
            view->drawTree(win);
            it++;
        }
    }
    
    virtual bool handleEventTree(const Window& win, const Event& ev) {
        if (!visible()) return false;
        if (!interaction()) return false;
        // Let the subviews handle the event first
        for (auto it=_subviews.begin(); it!=_subviews.end();) {
            Ptr view = (*it).lock();
            if (!view) {
                // Prune and continue
                it = _subviews.erase(it);
                continue;
            }
            
            if (view->handleEventTree(win, ev)) return true;
            it++;
        }
        
        // None of the subviews wanted the event; let the view itself handle it
        if (handleEvent(win, ev)) return true;
        return false;
    }
    
    virtual void refresh(const Window& win) {
        layoutTree(win);
        drawTree(win);
        CursorState::Draw();
        ::update_panels();
        ::refresh();
    }
    
    virtual void track(const Window& win, const Event& ev) {
        _tracking = true;
        Defer(_tracking = false); // Exception safety
        Defer(_trackStop = false); // Exception safety
        
        do {
            refresh(win);
            
            _eventCurrent = UI::NextEvent();
            Defer(_eventCurrent = {}); // Exception safety
            
            handleEventTree(win, _eventCurrent);
        } while (!_trackStop);
    }
    
    // Signal track() to return
    virtual void trackStop() {
        _trackStop = true;
    }
    
    virtual bool tracking() const { return _tracking; }
    virtual const Event& eventCurrent() const { return _eventCurrent; }
    
protected:
    template <typename X, typename Y>
    void _setForce(X& x, const Y& y) {
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
    bool _winErased(const Window& win);
    
    static inline ColorPalette _Colors;
    
    std::list<WeakPtr> _subviews;
    Point _origin;
    Size _size;
    bool _visible = true;
    bool _interaction = true;
    bool _layoutNeeded = true;
    bool _drawNeeded = true;
    bool _tracking = false;
    bool _trackStop = false;
    Event _eventCurrent;
    
    HitTestExpand _hitTestExpand;
    
    std::optional<Color> _borderColor;
};

using ViewPtr = std::shared_ptr<View>;

} // namespace UI
