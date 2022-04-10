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
    
//    static bool HandleEventTree(const Window& win, const Point& orig, const Event& ev) {
//        if (!visible()) return false;
//        if (!interaction()) return false;
//        _TreeState treeState(_TState, win, orig);
//        
//        // Let the subviews handle the event first
//        for (auto it=_subviews.begin(); it!=_subviews.end();) {
//            Ptr view = (*it).lock();
//            if (!view) {
//                // Prune and continue
//                it = _subviews.erase(it);
//                continue;
//            }
//            
//            if (view->handleEventTree(win, _TState.origin()+view->origin(), SubviewConvert(*view, ev))) return true;
//            it++;
//        }
//        
//        // None of the subviews wanted the event; let the view itself handle it
//        if (handleEvent(win, ev)) return true;
//        return false;
//    }
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    Event nextEvent() {
        _refresh();
        
        // Wait for another event
        for (;;) {
            int ch = ::wgetch(*this);
            if (ch == ERR) continue;
            
            Event ev = {
                .type = (Event::Type)ch,
                .time = std::chrono::steady_clock::now(),
            };
            switch (ev.type) {
            case Event::Type::Mouse: {
                MEVENT mouse = {};
                int ir = ::getmouse(&mouse);
                if (ir != OK) continue;
                ev.mouse = {
                    .origin = {mouse.x, mouse.y},
                    .bstate = mouse.bstate,
                };
                break;
            }
            
            case Event::Type::WindowResize: {
                throw WindowResize();
            }
            
            case Event::Type::KeyCtrlC:
            case Event::Type::KeyCtrlD: {
                throw ExitRequest();
            }
            
            default: {
                break;
            }}
            
            return ev;
        }
    }
    
    GraphicsState graphicsStateCalc(View& target) {
        return _graphicsStateCalc(target, {.screen=this}, *this);
    }
    
//    void track(View& target, const Event& ev) {
//        _track(target, ev);
//    }
    
//    // Signal track() to return
//    void trackStop() {
//        _trackStop = true;
//    }
    
    
    
//    virtual void track(const Window& win, const Event& ev) {
////        _TreeState treeState(_TState, win, {});
//        
//        _tracking = true;
//        Defer(_tracking = false); // Exception safety
//        Defer(_trackStop = false); // Exception safety
//        
//        do {
//            refresh();
//            
//            _eventCurrent = UI::NextEvent();
//            Defer(_eventCurrent = {}); // Exception safety
//            
//            handleEventTree(*this, {}, _eventCurrent);
//        } while (!_trackStop);
//    }
    
//    virtual bool tracking() const { return _tracking; }
    
//    const Event& eventCurrent() const { return _eventCurrent; }
    
private:
    void _refresh() {
        GraphicsState gstate = {.screen=this};
        layoutTree(gstate);
        drawTree(gstate);
        CursorState::Draw();
        ::update_panels();
        ::refresh();
    }
    
    GraphicsState _graphicsStateCalc(View& target, GraphicsState gstate, View& view) const {
        if (&view == &target) return gstate;
        
        gstate = view.convert(gstate);
        auto it = view.subviews();
        for (;;) {
            ViewPtr subview = view.subviewsNext(it);
            if (!subview) return {};
            GraphicsState gsubview = _graphicsStateCalc(target, gstate, *subview);
            if (gsubview) return gsubview;
        }
    }
    
    _GraphicsStateSwapper _gpushed = View::GStatePush({.screen=this});
    
//    
//    bool _tracking = false;
//    bool _trackStop = false;
//    Event _eventCurrent;
};

using ScreenPtr = std::shared_ptr<Screen>;

} // namespace UI
