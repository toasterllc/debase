#pragma once
#include "Window.h"

namespace UI {

class Screen : public Window {
public:
    using Window::Window;
    
    Point origin() const override {
        return { getbegx((WINDOW*)(*this)), getbegy((WINDOW*)(*this)) };
    }
    bool origin(const Point& x) override { return false; } // Ignore attempts to set screen origin
    
    Size size() const override {
        return { getmaxx((WINDOW*)(*this)), getmaxy((WINDOW*)(*this)) };
    }
    bool size(const Size& x) override { return false; } // Ignore attempts to set screen size
    
    Size windowSize() const override { return Window::windowSize(); }
    bool windowSize(const Size& s) override { return false; } // Ignore attempts to set screen size
    
    const ColorPalette& colors() const override {
        return _colors;
    }
    
    bool colors(ColorPalette&& colors) {
        _colors = std::move(colors);
        return true;
    }
    
    bool cursorState(const CursorState& x) override {
        _cursorState = x;
        return true;
    }
    
//    void layout(const Window& win, const Point& orig) override {
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
//        View::layout(*this, {});
//    }
//    
//    void draw(const Window& win, const Point& orig) override {
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
//        View::draw(*this, {});
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
//            view->draw(win, _TState.origin()+view->origin());
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
//            if (view->handleEvent(win, _TState.origin()+view->origin(), SubviewConvert(*view, ev))) return true;
//            it++;
//        }
//        
//        // None of the subviews wanted the event; let the view itself handle it
//        if (handleEvent(win, ev)) return true;
//        return false;
//    }
    
//    virtual bool suppressEvents() const { return _suppressEvents; }
//    virtual bool suppressEvents(bool x) {
//        if (x == _suppressEvents) return false;
//        _suppressEvents = x;
//        return true;
//    }
    
    virtual void refresh() {
        GraphicsState gstate = graphicsStateCalc(*this);
        gstate.orderPanels = _orderPanelsNeeded;
        
        // Reset cursor state
        // The last view in the last window to set the cursor state, wins
        _cursorState = {};
        
        layout(gstate);
        
        // If _orderPanelsNeeded=false at function entry, but _orderPanelsNeeded=true after a layout pass,
        // do another layout pass specifically to order the panels. If we didn't do this, we'd draw a
        // single frame where the panels have the wrong z-ordering.
        if (!gstate.orderPanels && _orderPanelsNeeded) {
            gstate.orderPanels = true;
            layout(gstate);
        }
        
        draw(gstate);
        ::update_panels();
        ::refresh();
        _cursorDraw();
        
        _orderPanelsNeeded = false;
    }
    
    virtual Event eventNext(Deadline deadline=Forever) {
        using namespace std::chrono;
        
        refresh();
        
        // Wait for another event
        for (;;) {
            // Set the appropriate timeout according to the deadline
            {
                int ms = 0;
                if (deadline==Forever || deadline==Once) ms = -1;
                else if (deadline == Poll) ms = 0;
                else {
                    ms = (int)std::max((intmax_t)0, (intmax_t)duration_cast<milliseconds>(deadline-steady_clock::now()).count());
                }
                wtimeout(*this, ms);
            }
            
            int ch = ::wgetch(*this);
            if (ch == ERR) {
                // We got an error:
                //   if timeout isn't enabled, wait for an event again
                //   if timeout is enabled and it's expired, return an empty event
                if (deadline==Forever || deadline==Once) continue;
                // >= and not > so that deadline=now() can be given and we're
                // guaranteed to perform a single iteration
                if (steady_clock::now() >= deadline) return {};
            }
            
            Event ev = {
                .id = _eventCurrent.id+1,
                .type = (Event::Type)ch,
                .time = steady_clock::now(),
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
            
            // Only set _eventCurrent once we're sure that we're returning the event
            _eventCurrent = ev;
            return ev;
        }
    }
    
    virtual Event eventNext(std::chrono::milliseconds timeout) {
        return eventNext(std::chrono::steady_clock::now()+timeout);
    }
    
    virtual const Event& eventCurrent() const {
        return _eventCurrent;
    }
    
    // eventSince() returns whether events have occurred since the given event
    virtual bool eventSince(const Event& ev) {
        return _eventCurrent.id != ev.id;
    }
    
    virtual GraphicsState graphicsStateCalc(View& target) {
        return _graphicsStateCalc(target, {.screen=this}, *this);
    }
    
    virtual bool orderPanelsNeeded() { return _orderPanelsNeeded; }
    virtual void orderPanelsNeeded(bool x) { _orderPanelsNeeded = x; }
    
private:
    void _cursorDraw() {
        if (hitTest(_cursorState.origin)) {
            ::curs_set(_cursorState.visible);
            ::move(_cursorState.origin.y, _cursorState.origin.x);
            
        } else {
            ::curs_set(false);
        }
    }
    
    GraphicsState _graphicsStateCalc(View& target, GraphicsState gstate, View& view) const {
        gstate = view.convert(gstate);
        if (&view == &target) return gstate;
        
        auto it = view.subviewsBegin();
        for (;;) {
            ViewPtr subview = view.subviewsNext(it);
            if (!subview) return {};
            GraphicsState gsubview = _graphicsStateCalc(target, gstate, *subview);
            if (gsubview) return gsubview;
        }
    }
    
    _GraphicsStateSwapper _gstate = View::GStatePush({.screen=this});
    Event _eventCurrent;
    ColorPalette _colors;
    CursorState _cursorState;
    bool _orderPanelsNeeded = false;
};

using ScreenPtr = std::shared_ptr<Screen>;

} // namespace UI
