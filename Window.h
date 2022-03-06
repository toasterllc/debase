#pragma once
#include "lib/ncurses/include/curses.h"
#include "lib/ncurses/include/panel.h"

struct Vector {
    int x = 0;
    int y = 0;    
};

using Point = Vector;
using Size = Vector;

struct Rect {
    Point point;
    Size size;
};

inline Rect Intersection(const Rect& a, const Rect& b) {
    const int minX = std::max(a.point.x, b.point.x);
    const int maxX = std::min(a.point.x+a.size.x, b.point.x+b.size.x);
    const int w = maxX-minX;
    
    const int minY = std::max(a.point.y, b.point.y);
    const int maxY = std::min(a.point.y+a.size.y, b.point.y+b.size.y);
    const int h = maxY-minY;
    
    if (w<=0 || h<=0) return {};
    return {
        .point = {minX, minY},
        .size = {w, h},
    };
}

inline bool Empty(const Rect& r) {
    return r.size.x==0 || r.size.y==0;
}

class Window {
public:
    static void Redraw() {
        ::update_panels();
        ::doupdate();
    }
    
    class Attr {
    public:
        Attr() {}
        Attr(const Window& win, int attr) : _s({.win=&win, .attr=attr}) {
            wattron(*_s.win, _s.attr);
        }
        
        Attr(const Attr& x) = delete;
        
        // Move constructor: use move assignment operator
        Attr(Attr&& x) { *this = std::move(x); }
        
        // Move assignment operator
        Attr& operator=(Attr&& x) {
            _s = std::move(x._s);
            x._s = {};
            return *this;
        }
        
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
    
    // Invalid
    Window() {}
    
    struct NewType {};
    static constexpr NewType New;
    
    Window(NewType) {
        _s.win = ::newwin(0, 0, 0, 0);
        assert(_s.win);
        
        ::keypad(_s.win, true);
        ::meta(_s.win, true);
    }
    
    Window(WINDOW* window) {
        _s.win = window;
        ::keypad(_s.win, true);
        ::meta(_s.win, true);
    }
    
    // Move constructor: use move assignment operator
    Window(Window&& x) { *this = std::move(x); }
    
    // Move assignment operator
    Window& operator=(Window&& x) {
        _s = std::move(x._s);
        x._s = {};
        return *this;
    }
    
    void setSize(const Size& s) {
        ::wresize(*this, std::max(1, s.y), std::max(1, s.x));
    }
    
    void setPosition(const Point& p) {
        ::mvwin(*this, p.y, p.x);
    }
    
    void drawBorder() {
        ::box(*this, 0, 0);
    }
    
    void drawLineHoriz(const Point& p, int len) {
        mvwhline(*this, p.y, p.x, 0, len);
    }
    
    void drawLineVert(const Point& p, int len) {
        mvwvline(*this, p.y, p.x, 0, len);
    }
    
    void drawRect(const Rect& rect) {
        const int x1 = rect.point.x;
        const int y1 = rect.point.y;
        const int x2 = rect.point.x+rect.size.x;
        const int y2 = rect.point.y+rect.size.y;
        mvwhline(*this, y1, x1, 0, rect.size.x);
        mvwhline(*this, y2, x1, 0, rect.size.x);
        mvwvline(*this, y1, x1, 0, rect.size.y);
        mvwvline(*this, y1, x2, 0, rect.size.y);
        mvwaddch(*this, y1, x1, ACS_ULCORNER);
        mvwaddch(*this, y2, x1, ACS_LLCORNER);
        mvwaddch(*this, y1, x2, ACS_URCORNER);
        mvwaddch(*this, y2, x2, ACS_LRCORNER);
    }
    
    void drawText(const Point& p, const char* fmt, ...) {
        va_list args;
        va_start(args, fmt);
        drawText(p, fmt, args);
        va_end(args);
    }

    void drawText(const Point& p, const char* fmt, va_list args) {
        ::wmove(*this, p.y, p.x);
        ::vw_printw(*this, fmt, args);
    }
    
    void erase() {
        ::werase(*this);
    }
    
    Rect rect() const {
        return Rect{
            .point = { getbegx(_s.win), getbegy(_s.win) },
            .size  = { getmaxx(_s.win), getmaxy(_s.win) },
        };
    }
    
    bool hitTest(const Point& p) const {
        return !Empty(Intersection(rect(), Rect{.point=p, .size={1,1}}));
    }
    
    int getChar() const {
        return ::wgetch(*this);
    }
    
    operator WINDOW*() const { return _s.win; }
    
    Attr setAttr(int attr) {
        return Attr(*this, attr);
    }
    
private:
    struct {
        WINDOW* win = nullptr;
    } _s = {};
};
