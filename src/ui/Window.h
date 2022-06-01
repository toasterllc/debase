#pragma once
#include <vector>
#include "Bitfield.h"
#include "UI.h"
#include "UTF8.h"
#include "View.h"

namespace UI {

class Window : public View {
public:
    struct UninitType {};
    static constexpr UninitType Uninit = {};
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
    
    virtual bool enabled() const override { return View::enabled(); }
    virtual bool enabled(bool x) override {
        if (View::enabled(x)) {
            eraseNeeded(true);
            return true;
        }
        return false;
    }
    
    // focusable(): whether the window can receive focus, to be overridden by subclasses
    virtual bool focusable() const { return false; }
    
    GraphicsState convert(GraphicsState x) override {
        x.window = this;
        x.originWindow = {};
        x.originScreen += origin();
        x.erased = false;
        return x;
    }
    
    virtual Point windowOrigin() const { return { getbegx(_s.win), getbegy(_s.win) }; }
    virtual bool windowOrigin(const Point& p) {
        if (p == windowOrigin()) return false;
        ::wmove(*this, p.y, p.x);
        return true;
    }
    
    virtual Size windowSize() const { return { getmaxx(_s.win), getmaxy(_s.win) }; }
    virtual bool windowSize(const Size& s) {
        if (s == windowSize()) return false;
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
        
        // If our window size that we previously set doesn't match our current window size, then
        // ncurses changed it out from beneath us. In that case, we need to do a full redraw.
        // This handles the case where the window is clipped due to going offscreen (specifically,
        // the bottom region of the window when resizing the terminal vertically), in which case
        // the offscreen parts are lost (hence the need to redraw).
        if (windowSize() != _s.sizePrev) eraseNeeded(true);
        
        // Set our window frame to our desired window frame, which we won't necessarily get
        const Rect fwant = {gstate.originScreen, size()};
        windowFrame(fwant);
        const Rect fgot = windowFrame();
        _s.sizePrev = fgot.size;
        
        // Backport any window frame adjustments to our view's frame.
        // This is necessary when ncurses 'denies' some portion of our requested
        // frame because it would go offscreen.
        const Size originDelta = fgot.origin-fwant.origin;
        frame({origin()+originDelta, fgot.size});
        
        // Reset the cursor state when a new window is encountered
        // This way, the last window (the one on top) gets to decide the cursor state
        // We only want to do this for focusable windows though, because we have
        // windows (such as CommitPanel) that can't receive focus and therefore
        // shouldn't affect the cursor state.
        if (focusable()) {
            cursorState({});
        }
        
        View::layout(gstate);
    }
    
    virtual Window& operator =(Window&& x) { std::swap(_s, x._s); return *this; }
    
    virtual operator WINDOW*() const { return _s.win; }
    
private:
    struct {
        WINDOW* win = nullptr;
        Size sizePrev;
    } _s;
};

using WindowPtr = std::shared_ptr<Window>;

} // namespace UI
