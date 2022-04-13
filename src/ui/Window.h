#pragma once
#define NCURSES_WIDECHAR 1
#include <vector>
#include "lib/ncurses/include/curses.h"
#include "lib/ncurses/include/panel.h"
#include "Bitfield.h"
#include "UI.h"
#include "UTF8.h"
#include "View.h"
#include <os/log.h>

namespace UI {

class Window : public View {
public:
    struct UninitType {};
    static constexpr UninitType Uninit;
    Window(UninitType) {}
    
    Window(WINDOW* win=nullptr) : _s{.win = win} {
        if (!_s.win) {
            _s.win = ::newwin(0, 0, 0, 0);
            assert(_s.win);
        }
        
        ::keypad(_s.win, true);
        ::meta(_s.win, true);
    }
    
    Window(const Window& x) = delete;
    Window(Window&& x) {
        std::swap(_s, x._s);
    }
    
    virtual ~Window() {
        if (_s.win) {
            ::delwin(_s.win);
        }
    }
    
    Point origin() const override { return View::origin(); }
    
    bool origin(const Point& x) override {
        const bool drawNeededPrev = drawNeeded();
        const bool changed = View::origin(x);
        // Suppress View's behavior of dirtying ourself when changing the origin
        if (!drawNeededPrev) drawNeeded(false);
        return changed;
    }
    
    GraphicsState convert(GraphicsState x) override {
        x.window = this;
//        x.finalWindow = this;
        x.originWindow = {};
        x.originScreen += origin();
        x.erased = false;
        return x;
    }
    
//    Size size() const override { return View::size(); }
//    bool size(const Size& x) override {
//        const bool changed = View::size(x);
//        if (changed) eraseNeeded(true);
//        return changed;
//    }
    
    virtual Point windowOrigin() const { return { getbegx(_s.win), getbegy(_s.win) }; }
    virtual bool windowOrigin(const Point& p) {
        if (p == windowOrigin()) return false;
        ::wmove(*this, p.y, p.x);
        return true;
    }
    
    virtual Size windowSize() const { return { getmaxx(_s.win), getmaxy(_s.win) }; }
    virtual bool windowSize(const Size& s) {
        if (s == windowSize()) return false;
//        const Size ss = {std::max(1,s.x), std::max(1,s.y)};
//        if (ss == windowSize()) return;
        ::wresize(*this, s.y, s.x);
        return true;
    }
    
    virtual Rect windowFrame() const { return { .origin=windowOrigin(), .size=windowSize() }; }
    virtual bool windowFrame(const Rect& x) {
        bool changed = false;
        changed |= windowSize(x.size);
        changed |= windowOrigin(x.origin);
        return changed;
    }
    
//    Point origin() const override { return { getbegx(_s.win), getbegy(_s.win) }; }
//    
//    Size size() const override { return { getmaxx(_s.win), getmaxy(_s.win) }; }
//    void size(const Size& s) override {
//        Size ss = {std::max(1,s.x), std::max(1,s.y)};
//        // Short-circuit if the size hasn't changed
//        if (ss == size()) return;
//        ::wresize(*this, ss.y, ss.x);
//    }
    
//    Point mousePosition(const Event& ev) const {
//        return ev.mouse.origin-origin();
//    }
    
//    // convert(): convert a point from the coorindate system of the parent window to the coordinate system of `this`
//    virtual Point convert(const Point& p) const {
//        return p-origin();
//    }
//    
//    // convert(): convert an event from the coorindate system of the parent window to the coordinate system of `this`
//    virtual Event convert(const Event& p) const {
//        Event r = p;
//        if (r.type == Event::Type::Mouse) {
//            r.mouse.point = convert(r.mouse.point);
//        }
//        return r;
//    }
    
    void drawRect() const override {
        // For the case where we're drawing the window's border, we attempt
        // to be more efficent than View's drawRect() by using ::box().
        ::box(*this, 0, 0);
    }
    
    void drawRect(const Rect& rect) const override {
        View::drawRect(rect);
    }
    
    void erase() override {
        // Don't call super because View's implementation will be redundant
        ::werase(*this);
    }
    
    void layout(GraphicsState gstate) override {
        if (!visible()) return;
        
        // If the frame that we previously set doesn't match our current frame, then ncurses changed
        // our frame out from beneath us. In that case, we need to do a full redraw.
        // This handles the case where the window is clipped due to going offscreen, in which case
        // the offscreen parts are lost (hence the need to redraw).
        if (windowFrame() != _s.framePrev) {
            eraseNeeded(true);
        }
        
        const bool frameChanged = windowFrame({gstate.originScreen, size()});
        _s.framePrev = windowFrame();
        
//        const Rect sb = _screenBounds();
//        const Rect wf = windowFrame();
//        const bool clipped = Intersection(sb, wf) != wf;
//        const bool frameChanged = windowFrame({gstate.originScreen, size()});
//        
//        os_log(OS_LOG_DEFAULT, "screenBounds={%d %d}, windowFrame={%d %d}", sb.w(), sb.h(), wf.w(), wf.h());
//        
//        // If our frame changed, and our window was previously clipped (ie it went offscreen),
//        // then we need to erase/redraw the whole window
//        if (frameChanged && clipped) {
//            eraseNeeded(true);
//        }
        
        // Reset the cursor state when a new window is encountered
        // This way, the last window (the one on top) gets to decide the cursor state
        cursorState({});
        
//        os_log(OS_LOG_DEFAULT, "SIZE CHANGED %d", windowSize().y);
        
//        os_log(OS_LOG_DEFAULT, "SIZE CHANGED %d", windowSize().y);
//        
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
////        
        // Update our size/origin based on ncurses' adjusted size
//        const Size sizeActual = windowSize();
//        const Size originScreenActual = windowOrigin();
//        const Size originScreenDelta = originScreenActual-gstate.originScreen;
//        if (originScreenDelta.x || originScreenDelta.y) {
////            layoutNeeded(true);
//            
//            os_log(OS_LOG_DEFAULT, "ORIGIN CHANGED");
//        }
//        
////        size(sizeActual);
////        origin(origin()+originScreenDelta);
//        
//        if (sizeActual != sizePrev) {
//            os_log(OS_LOG_DEFAULT, "SIZE CHANGED");
//            
////            abort();
////            // We need to erase+redraw after resizing
////            // (eraseNeeded=true implicity triggers a redraw)
////            eraseNeeded(true);
//        }
        
//        
//        if (originScreenDelta.x || originScreenDelta.y) {
//            os_log(OS_LOG_DEFAULT, "origin AFTER: %d %d", origin().x, origin().y);
//        }
//        
//        // Update our gstate based on ncurses' adjusted size
//        gstate.originWindow += originScreenDelta;
//        gstate.originScreen += originScreenDelta;
//        
//        layoutNeeded(true);
        
        View::layout(gstate);
    }
    
//    virtual void draw(GraphicsState gstate) override {
//        if (!visible()) return;
//        if (eraseNeeded()) {
//            gstate.erased = true;
//            ::werase(*this);
//            eraseNeeded(false);
//        }
//        
//        View::draw(gstate);
//    }
    
    virtual Window& operator =(Window&& x) { std::swap(_s, x._s); return *this; }
    
    virtual operator WINDOW*() const { return _s.win; }
    
private:
    struct {
        WINDOW* win = nullptr;
        Rect framePrev;
    } _s;
};

using WindowPtr = std::shared_ptr<Window>;

} // namespace UI
