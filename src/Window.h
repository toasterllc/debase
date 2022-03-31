#pragma once
#define NCURSES_WIDECHAR 1
#include "lib/ncurses/include/curses.h"
#include "lib/ncurses/include/panel.h"
#include "Bitfield.h"

namespace UI {

struct Vector {
    int x = 0;
    int y = 0;
    
    bool operator==(const Vector& v) const { return x==v.x && y==v.y; }
    bool operator!=(const Vector& v) const { return !(*this == v); }
    
    Vector operator+(const Vector& v) const { return {x+v.x, y+v.y}; }
    template <typename T> Vector operator+(const T& t) const { return {x+t, y+t}; }
    
    Vector& operator+=(const Vector& v) { x+=v.x; y+=v.y; return *this; }
    template <typename T> Vector& operator+=(const T& t) { x+=t; y+=t; return *this; }
    
    Vector operator-(const Vector& v) const { return {x-v.x, y-v.y}; }
    template <typename T> Vector operator-(const T& t) const { return {x-t, y-t}; }
    
    Vector& operator-=(const Vector& v) { x-=v.x; y-=v.y; return *this; }
    template <typename T> Vector& operator-=(const T& t) { x-=t; y-=t; return *this; }
};

using Point = Vector;
using Size = Vector;

struct Rect {
    Point point;
    Size size;
    
    bool operator==(const Rect& x) const { return point==x.point && size==x.size; }
    bool operator!=(const Rect& x) const { return !(*this == x); }
    
    int xmin() const { return point.x; }
    int xmax() const { return point.x+size.x-1; }
    
    int ymin() const { return point.y; }
    int ymax() const { return point.y+size.y-1; }
};

struct Event {
    enum class Type : int {
        None            = 0,
        Mouse           = KEY_MOUSE,
        WindowResize    = KEY_RESIZE,
        KeyDelete       = '\x7F',
        KeyFnDelete     = KEY_DC,
        KeyEscape       = '\x1B',
        KeyReturn       = '\n',
        KeyCtrlC        = '\x03',
        KeyCtrlD        = '\x04',
        KeyC            = 'c',
    };
    
    struct MouseButtons : Bitfield<uint8_t> {
        static constexpr Bit Left  = 1<<0;
        static constexpr Bit Right = 1<<1;
        using Bitfield::Bitfield;
    };
    
    Type type = Type::None;
    MEVENT mouse = {};
    
    operator bool() const { return type!=Type::None; }
    
    bool mouseDown(MouseButtons buttons=MouseButtons::Left) const {
        if (type != Type::Mouse) return false;
        return mouse.bstate & _DownMaskForButtons(buttons);
    }
    
    bool mouseUp(MouseButtons buttons=MouseButtons::Left) const {
        if (type != Type::Mouse) return false;
        return mouse.bstate & _UpMaskForButtons(buttons);
    }
    
private:
    static mmask_t _DownMaskForButtons(MouseButtons buttons) {
        mmask_t r = 0;
        if (buttons & MouseButtons::Left)  r |= BUTTON1_PRESSED;
        if (buttons & MouseButtons::Right) r |= BUTTON3_PRESSED;
        return r;
    }
    
    static mmask_t _UpMaskForButtons(MouseButtons buttons) {
        mmask_t r = 0;
        if (buttons & MouseButtons::Left)  r |= BUTTON1_RELEASED;
        if (buttons & MouseButtons::Right) r |= BUTTON3_RELEASED;
        return r;
    }
};

struct ExitRequest : std::exception {};

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

inline Rect Inset(const Rect& x, const Size& s) {
    return {x.point+s, x.size-s-s};
}

inline bool Empty(const Rect& x) {
    return x.size.x==0 || x.size.y==0;
}

inline bool HitTest(const Rect& r, const Point& p, Size expand={0,0}) {
    return !Empty(Intersection(Inset(r, {-expand.x,-expand.y}), {p, {1,1}}));
}

inline void Redraw() {
    ::update_panels();
    ::refresh();
//    ::doupdate();
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
        const int x2 = rect.point.x+rect.size.x-1;
        const int y2 = rect.point.y+rect.size.y-1;
        mvwhline(*this, y1, x1, 0, rect.size.x);
        mvwhline(*this, y2, x1, 0, rect.size.x);
        mvwvline(*this, y1, x1, 0, rect.size.y);
        mvwvline(*this, y1, x2, 0, rect.size.y);
        mvwaddch(*this, y1, x1, ACS_ULCORNER);
        mvwaddch(*this, y2, x1, ACS_LLCORNER);
        mvwaddch(*this, y1, x2, ACS_URCORNER);
        mvwaddch(*this, y2, x2, ACS_LRCORNER);
    }
    
    template <typename ...T_Args>
    void drawText(const Point& p, const char* fmt, T_Args&&... args) const {
        mvwprintw(*this, p.y, p.x, fmt, std::forward<T_Args>(args)...);
    }
    
    void erase() const {
        ::werase(*this);
    }
    
    Size size() const {
        return { getmaxx(_win), getmaxy(_win) };
    }
    
    Rect bounds() const {
        return Rect{ .size = size() };
    }
    
    Rect frame() const {
        return Rect{
            .point = { getbegx(_win), getbegy(_win) },
            .size  = size(),
        };
    }
    
    bool hitTest(const Point& p) const {
        return HitTest(frame(), p);
    }
    
    Event nextEvent() {
        // Wait for another mouse event
        for (;;) {
            Event ev = {
                .type = (Event::Type)::wgetch(*this),
            };
            switch (ev.type) {
            case Event::Type::Mouse: {
                MEVENT mouse = {};
                int ir = ::getmouse(&mouse);
                if (ir != OK) continue;
                return Event{
                    .type = Event::Type::Mouse,
                    .mouse = mouse,
                };
            }
            
            case Event::Type::KeyCtrlC:
            case Event::Type::KeyCtrlD: {
                throw ExitRequest();
            }
            
            default: {
                return ev;
            }}
        }
    }
    
    operator WINDOW*() const { return _win; }
    
private:
    WINDOW* _win = nullptr;
};

using Window = std::shared_ptr<_Window>;

} // namespace UI