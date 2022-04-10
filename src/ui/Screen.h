#pragma once
#include "Window.h"

namespace UI {

class Screen : public Window {
public:
    using Window::Window;
    
    Point origin() const override {
        return { getbegx((WINDOW*)(*this)), getbegy((WINDOW*)(*this)) };
    }
    
    void origin(const Point& x) override {
        // Can't set screen origin
        abort();
    }
    
    Size size() const override {
        return { getmaxx((WINDOW*)(*this)), getmaxy((WINDOW*)(*this)) };
    }
    
    void size(const Size& x) override {
        // Can't set screen size
        abort();
    }
    
    Size windowSize() const override { return Window::windowSize(); }
    void windowSize(const Size& s) override {} // Ignore attempts to set screen size
    
    
    
    
    
    
//    void layoutTree(const Window& win, const Point& orig) override {
//        windowSize(size());
//        windowOrigin(orig);
//        
////        if (_s.originPrev != orig) {
////            
////            _s.originPrev = orig;
////        }
////        
////        // Detect size changes
////        // ncurses can change our size out from under us (eg by the
////        // terminal size changing), so we handle all size changes
////        // here, instead of in the size() setter
////        if (_s.sizePrev != size()) {
////            // We need to erase+redraw after resizing
////            // (eraseNeeded=true implicity sets drawNeeded=true)
////            eraseNeeded(true);
////            _s.sizePrev = size();
////        }
//        
//        View::layoutTree(*this, {});
//    }
//    
//    void drawTree(const Window& win, const Point& orig) override {
//        // Remember whether we erased ourself during this draw cycle
//        // This is used by View instances (Button and TextField)
//        // to know whether they need to be drawn again
//        _s.erased = _s.eraseNeeded;
//        
//        // Erase ourself if needed, and remember that we did so
//        if (_s.eraseNeeded) {
//            ::werase(*this);
//            _s.eraseNeeded = false;
//        }
//        
//        View::drawTree(*this, {});
//    }
    
    
    
    
    
    
    
    
    static void LayoutTree(GraphicsState gstate, View& view) {
        if (!view.visible()) return;
        
        // When we hit a window, set the window's size/origin according to the gstate, and reset gstate
        // to reference the new window, and the origin to {0,0}
        if (Window* win = dynamic_cast<Window*>(&view)) {
            win->windowSize(win->size());
            win->windowOrigin(gstate.origin);
            
            gstate.window = win;
            gstate.origin = {};
        
        } else {
            gstate.origin += view.origin();
        }
        
        auto gpushed = View::GStatePush(gstate);
        
        if (view.layoutNeeded()) {
            view.layout();
            view.layoutNeeded(false);
        }
        
        auto it = view.subviews();
        for (;;) {
            ViewPtr subview = view.subviewsNext(it);
            if (!subview) break;
            LayoutTree(gstate, *subview);
        }
    }
    
    static void DrawTree(GraphicsState gstate, View& view) {
        if (!view.visible()) return;
        
        // When we hit a window, reset gstate to reference the new window, and the origin to {0,0}
        // Also, erase the window if needed, and remember that we did so in the gstate
        if (Window* win = dynamic_cast<Window*>(&view)) {
            gstate.window = win;
            gstate.origin = {};
            
            if (win->eraseNeeded()) {
                gstate.erased = true;
                ::werase(*win);
                win->eraseNeeded(false);
            }
        
        } else {
            gstate.origin += view.origin();
        }
        
        auto gpushed = View::GStatePush(gstate);
        
        // Redraw the view if it says it needs it, or if this part of the view tree has been erased
        if (view.drawNeeded() || gstate.erased) {
            view.drawBackground();
            view.draw();
            view.drawBorder();
            view.drawNeeded(false);
        }
        
        auto it = view.subviews();
        for (;;) {
            ViewPtr subview = view.subviewsNext(it);
            if (!subview) break;
            DrawTree(gstate, *subview);
        }
    }
    
    
    
    
    
    
    
    
    
    
    
    
//    static void DrawTree(Window* window, Point origin, View& view) {
//        if (!view.visible()) return;
//        
//        // When we hit a window, reset GraphicsState to reference the new window, and the origin to {0,0}
//        if (Window* viewWindow = dynamic_cast<Window*>(&view)) {
//            window = viewWindow;
//            origin = {};
//        }
//        
//        GraphicsState gstate(View::GState(), *window, origin);
//        
//        if (drawNeeded() || _winErased(win)) {
//            drawBackground();
//            draw();
//            drawBorder();
//            drawNeeded(false);
//        }
//        
//        auto it = view.subviews();
//        for (;;) {
//            ViewPtr subview = view.subviewsNext(it);
//            if (!subview) break;
//            DrawTree(window, origin, *subview);
//        }
//    }
//    
//    static void DrawTree(const Window& win, const Point& orig) {
//        if (!visible()) return;
//        _TreeState treeState(_TState, win, orig);
//        
//        // If the window was erased during this draw cycle, we need to redraw.
//        // _winErased() has to be implemented out-of-line because:
//        // 
//        //   Window.h includes View.h
//        //   ∴ View.h can't include Window.h
//        //   ∴ View has to forward declare Window
//        //   ∴ we can't call Window functions in View.h
//        //
//        if (drawNeeded() || _winErased(win)) {
//            drawBackground();
//            draw();
//            drawBorder();
//            drawNeeded(false);
//        }
//        
//        for (auto it=_subviews.begin(); it!=_subviews.end();) {
//            Ptr view = (*it).lock();
//            if (!view) {
//                // Prune and continue
//                it = _subviews.erase(it);
//                continue;
//            }
//            
//            view->drawTree(win, _TState.origin()+view->origin());
//            it++;
//        }
//    }
    
    static bool HandleEventTree(const Window& win, const Point& orig, const Event& ev) {
        if (!visible()) return false;
        if (!interaction()) return false;
        _TreeState treeState(_TState, win, orig);
        
        // Let the subviews handle the event first
        for (auto it=_subviews.begin(); it!=_subviews.end();) {
            Ptr view = (*it).lock();
            if (!view) {
                // Prune and continue
                it = _subviews.erase(it);
                continue;
            }
            
            if (view->handleEventTree(win, _TState.origin()+view->origin(), SubviewConvert(*view, ev))) return true;
            it++;
        }
        
        // None of the subviews wanted the event; let the view itself handle it
        if (handleEvent(win, ev)) return true;
        return false;
    }
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    virtual void refresh() {
        LayoutTree({}, *this);
        DrawTree({}, *this);
        CursorState::Draw();
        ::update_panels();
        ::refresh();
    }
    
    virtual void track(const Window& win, const Event& ev) {
//        _TreeState treeState(_TState, win, {});
        
        _tracking = true;
        Defer(_tracking = false); // Exception safety
        Defer(_trackStop = false); // Exception safety
        
        do {
            refresh();
            
            _eventCurrent = UI::NextEvent();
            Defer(_eventCurrent = {}); // Exception safety
            
            handleEventTree(*this, {}, _eventCurrent);
        } while (!_trackStop);
    }
    
    // Signal track() to return
    virtual void trackStop() {
        _trackStop = true;
    }
    
    virtual bool tracking() const { return _tracking; }
    
    virtual const Event& eventCurrent() const { return _eventCurrent; }
    
private:
    bool _tracking = false;
    bool _trackStop = false;
    Event _eventCurrent;
};

using ScreenPtr = std::shared_ptr<Screen>;

} // namespace UI
