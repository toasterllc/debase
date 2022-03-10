#pragma once
#include "lib/ncurses/include/curses.h"
#include "lib/ncurses/include/panel.h"

namespace UI {

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

enum class Event : int {
    Mouse           = KEY_MOUSE,
    WindowResize    = KEY_RESIZE,
    KeyDelete       = '\x7F',
    KeyDeleteFn     = KEY_DC,       // Fn+Delete
    KeyEscape       = '\x1B',
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

inline void Redraw() {
    ::update_panels();
    ::doupdate();
}

class _Window {
public:
    _Window(WINDOW* win=nullptr) : _win(win) {
        if (!_win) {
            _win = ::newwin(0, 0, 0, 0);
            assert(_win);
        }
        
        ::keypad(_win, true);
        ::meta(_win, true);
    }
    
    ~_Window() {
        ::delwin(_win);
        _win = nullptr;
    }
    
    void setSize(const Size& s) {
        ::wresize(*this, std::max(1, s.y), std::max(1, s.x));
    }
    
    void setPosition(const Point& p) {
        ::mvwin(*this, p.y, p.x);
    }
    
    void drawBorder() const {
        ::box(*this, 0, 0);
    }
    
    void drawLineHoriz(const Point& p, int len) const {
        mvwhline(*this, p.y, p.x, 0, len);
    }
    
    void drawLineVert(const Point& p, int len) const {
        mvwvline(*this, p.y, p.x, 0, len);
    }
    
    void drawRect(const Rect& rect) const {
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
    
    void drawText(const Point& p, const char* fmt, ...) const {
        va_list args;
        va_start(args, fmt);
        drawText(p, fmt, args);
        va_end(args);
    }

    void drawText(const Point& p, const char* fmt, va_list args) const {
        ::wmove(*this, p.y, p.x);
        ::vw_printw(*this, fmt, args);
    }
    
    void erase() const {
        ::werase(*this);
    }
    
    Rect rect() const {
        return Rect{
            .point = { getbegx(_win), getbegy(_win) },
            .size  = { getmaxx(_win), getmaxy(_win) },
        };
    }
    
    bool hitTest(const Point& p) const {
        return !Empty(Intersection(rect(), Rect{.point=p, .size={1,1}}));
    }
    
    Event nextEvent() const {
        return (Event)::wgetch(*this);
    }
    
    operator WINDOW*() const { return _win; }
    
//    Attr setAttr(int attr) const {
//        return Attr(*this, attr);
//    }
    
private:
    WINDOW* _win = nullptr;
};

using Window = std::shared_ptr<_Window>;

} // namespace UI
