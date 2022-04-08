#pragma once
#define NCURSES_WIDECHAR 1
#include <vector>
#include "lib/ncurses/include/curses.h"
#include "lib/ncurses/include/panel.h"
#include "Bitfield.h"
#include "UI.h"
#include "CursorState.h"
#include "UTF8.h"
#include "View.h"

namespace UI {

class Window : public View {
public:
    class Attr {
    public:
        Attr() {}
        Attr(const Window& win, int attr) : _s({.win=&win, .attr=attr}) {
//            if (rand() % 2) {
//                wattron(*_s.win, A_REVERSE);
//            } else {
//                wattroff(*_s.win, A_REVERSE);
//            }
            
            wattron(*_s.win, _s.attr);
        }
        
        Attr(const Attr& x) = delete;
        Attr(Attr&& x) { std::swap(_s, x._s); }
        Attr& operator =(Attr&& x) { std::swap(_s, x._s); return *this; }
        
        ~Attr() {
            if (_s.win) {
                wattroff(*_s.win, _s.attr);
            }
        }
    
    private:
        struct {
            const Window* win = nullptr;
            int attr = 0;
        } _s;
    };
    
    Window() {}
    
    Window(WINDOW* win) : _s{.win = win} {
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
    
    virtual void drawBorder() const {
        ::box(*this, 0, 0);
    }
    
    virtual void drawLineHoriz(const Point& p, int len, chtype ch=0) const {
        mvwhline(*this, p.y, p.x, ch, len);
    }
    
    virtual void drawLineVert(const Point& p, int len, chtype ch=0) const {
        mvwvline(*this, p.y, p.x, ch, len);
    }
    
    virtual void drawRect(const Rect& rect) const {
        const int x1 = rect.origin.x;
        const int y1 = rect.origin.y;
        const int x2 = rect.origin.x+rect.size.x-1;
        const int y2 = rect.origin.y+rect.size.y-1;
        mvwhline(*this, y1, x1, 0, rect.size.x);
        mvwhline(*this, y2, x1, 0, rect.size.x);
        mvwvline(*this, y1, x1, 0, rect.size.y);
        mvwvline(*this, y1, x2, 0, rect.size.y);
        mvwaddch(*this, y1, x1, ACS_ULCORNER);
        mvwaddch(*this, y2, x1, ACS_LLCORNER);
        mvwaddch(*this, y1, x2, ACS_URCORNER);
        mvwaddch(*this, y2, x2, ACS_LRCORNER);
    }
    
    virtual void drawText(const Point& p, const char* txt) const {
        mvwprintw(*this, p.y, p.x, "%s", txt);
    }
    
    virtual void drawText(const Point& p, int widthMax, const char* txt) const {
        widthMax = std::max(0, widthMax);
        
        std::string str = txt;
        auto it = UTF8::NextN(str.begin(), str.end(), widthMax);
        str.resize(std::distance(str.begin(), it));
        mvwprintw(*this, p.y, p.x, "%s", str.c_str());
    }
    
    template <typename ...T_Args>
    void drawText(const Point& p, const char* fmt, T_Args&&... args) const {
        mvwprintw(*this, p.y, p.x, fmt, std::forward<T_Args>(args)...);
    }
    
    Point origin() const override { return { getbegx(_s.win), getbegy(_s.win) }; }
    
    Size size() const override { return { getmaxx(_s.win), getmaxy(_s.win) }; }
    void size(const Size& s) override {
        Size ss = {std::max(1,s.x), std::max(1,s.y)};
        // Short-circuit if the size hasn't changed
        if (ss == size()) return;
        ::wresize(*this, ss.y, ss.x);
    }
    
    Point mousePosition(const Event& ev) const {
        return ev.mouse.origin-origin();
    }
    
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
    
    virtual Attr attr(int attr) const { return Attr(*this, attr); }
    
    void drawBackground(const Window& win) override {
        if (borderColor()) {
            Attr color = attr(*borderColor());
            win.drawRect(bounds());
        }
    }
    
    bool layoutNeeded() const override { return View::layoutNeeded() || _s.sizePrev!=size(); }
    void layoutNeeded(bool x) override { View::layoutNeeded(x); }
    
    void layoutTree(const Window& win) override {
        // Detect size changes
        // ncurses can change our size out from under us (eg by the
        // terminal size changing), so we handle all size changes
        // here, instead of in the size() setter
        if (_s.sizePrev != size()) {
            // We need to erase+redraw after resizing
            // (eraseNeeded=true implicity sets drawNeeded=true)
            eraseNeeded(true);
            _s.sizePrev = size();
        }
        
        View::layoutTree(*this);
    }
    
    void drawTree(const Window& win) override {
        // Remember whether we erased ourself during this draw cycle
        // This is used by View instances (Button and TextField)
        // to know whether they need to be drawn again
        _s.erased = _s.eraseNeeded;
        
        // Erase ourself if needed, and remember that we did so
        if (_s.eraseNeeded) {
            ::werase(*this);
            _s.eraseNeeded = false;
        }
        
        View::drawTree(*this);
    }
    
    virtual bool handleEventTree(const Window& win, const Event& ev) override {
        return View::handleEventTree(*this, ev);
    }
    
    // eraseNeeded(): sets whether the window should be erased the next time it's drawn
    virtual void eraseNeeded(bool x) {
        _s.eraseNeeded = x;
        if (_s.eraseNeeded) {
            drawNeeded(true);
        }
    }
    
    // erased(): whether the window was erased during this draw cycle
    virtual bool erased() const { return _s.erased; }
    
    virtual Window& operator =(Window&& x) { std::swap(_s, x._s); return *this; }
    
    virtual operator WINDOW*() const { return _s.win; }
    
protected:
    template <typename X, typename Y>
    void _setForce(X& x, const Y& y) {
        x = y;
        drawNeeded(true);
    }
    
    template <typename X, typename Y>
    void _set(X& x, const Y& y) {
        if (x == y) return;
        x = y;
        drawNeeded(true);
    }
    
private:
    struct {
        WINDOW* win = nullptr;
        Size sizePrev;
        // eraseNeeded: tracks whether the window needs to be erased the next time it's drawn
        bool eraseNeeded = true;
        // erased: tracks whether the window was erased in this draw cycle
        bool erased = false;
    } _s;
};

using WindowPtr = std::shared_ptr<Window>;

} // namespace UI
