#pragma once
#include <list>
#include <chrono>

#define NCURSES_WIDECHAR 1

#if __APPLE__
#include "lib/ncurses/build-mac/include/curses.h"
#include "lib/ncurses/build-mac/include/panel.h"
#elif __linux__
#include "lib/ncurses/build-linux/include/curses.h"
#include "lib/ncurses/build-linux/include/panel.h"
#endif

#include "Bitfield.h"

namespace UI {

struct Vector {
    int x = 0;
    int y = 0;
    
    constexpr bool operator ==(const Vector& v) const { return x==v.x && y==v.y; }
    constexpr bool operator !=(const Vector& v) const { return !(*this == v); }
    
    constexpr Vector operator +(const Vector& v) const { return {x+v.x, y+v.y}; }
    template <typename T> constexpr Vector operator +(const T& t) const { return {x+t, y+t}; }
    
    constexpr Vector& operator +=(const Vector& v) { x+=v.x; y+=v.y; return *this; }
    template <typename T> constexpr Vector& operator +=(const T& t) { x+=t; y+=t; return *this; }
    
    constexpr Vector operator -(const Vector& v) const { return {x-v.x, y-v.y}; }
    template <typename T> constexpr Vector operator -(const T& t) const { return {x-t, y-t}; }
    
    constexpr Vector& operator -=(const Vector& v) { x-=v.x; y-=v.y; return *this; }
    template <typename T> constexpr Vector& operator -=(const T& t) { x-=t; y-=t; return *this; }
    
    constexpr Vector operator -() const { return {-x, -y}; }
    
    template <typename T> constexpr Vector operator *(const T& t) const { return {x*t, y*t}; }
    
    template <typename T> constexpr Vector& operator *=(const T& t) { x*=t; y*=t; return *this; }
};

using Point = Vector;
using Size = Vector;

struct Rect {
    Point origin;
    Size size;
    
    constexpr bool operator ==(const Rect& x) const { return origin==x.origin && size==x.size; }
    constexpr bool operator !=(const Rect& x) const { return !(*this == x); }
    
    // Width/height
    constexpr int w() const { return size.x; }
    constexpr int h() const { return size.y; }
    
    // Left/right/top/bottom
    constexpr int l() const { return origin.x; }
    constexpr int r() const { return origin.x+size.x; }
    constexpr int t() const { return origin.y; }
    constexpr int b() const { return origin.y+size.y; }
    
    // Mid-x/mid-y
    constexpr int mx() const { return (l()+r())/2; }
    constexpr int my() const { return (t()+b())/2; }
    
    // Top-left, top-right, bottom-left, bottom-right, mid
    constexpr Point tl() const { return {l(), t()}; }
    constexpr Point tr() const { return {r(), t()}; }
    constexpr Point bl() const { return {l(), b()}; }
    constexpr Point br() const { return {r(), b()}; }
    constexpr Point m() const { return {mx(), my()}; }
};

struct Edges {
    int l = 0;
    int r = 0;
    int t = 0;
    int b = 0;
    
    Edges operator -() const { return {-l, -r, -t, -b}; }
};

struct Event {
    enum class Type : int {
        None            = 0,
        Mouse           = KEY_MOUSE,
        WindowResize    = KEY_RESIZE,
        KeyDelete       = '\x7F',
        KeyFnDelete     = KEY_DC,
        KeyLeft         = KEY_LEFT,
        KeyRight        = KEY_RIGHT,
        KeyUp           = KEY_UP,
        KeyDown         = KEY_DOWN,
        KeyTab          = '\t',
        KeyBackTab      = KEY_BTAB,
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
    std::chrono::steady_clock::time_point time = {};
    struct {
        Point origin; // Screen coordinates
        mmask_t bstate = 0;
        
//        Point position(const Window& win) const {
//            return position-win.origin();
//        }
    } mouse;
    
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

struct CursorState {
    bool visible = false;
    Point origin;
};

class Screen;
class Window;
struct GraphicsState {
//        GraphicsState() {}
    operator bool() const { return screen; }
    
    Screen* screen = nullptr;
    Window* window = nullptr;
//    Window* finalWindow = nullptr;
    Point originWindow; // For drawing purposes (relative to nearest window)
    Point originScreen; // For event purposes (relative to screen)
    bool erased = false;
    bool orderPanels = false;
};

struct WindowResize : std::exception {};
struct ExitRequest : std::exception {};

enum class Align {
    Left,
    Center,
    Right,
};

enum class Truncate {
    Head,
    Tail,
};

inline constexpr Rect Intersection(const Rect& a, const Rect& b) {
    const int minX = std::max(a.origin.x, b.origin.x);
    const int maxX = std::min(a.origin.x+a.size.x, b.origin.x+b.size.x);
    const int w = maxX-minX;
    
    const int minY = std::max(a.origin.y, b.origin.y);
    const int maxY = std::min(a.origin.y+a.size.y, b.origin.y+b.size.y);
    const int h = maxY-minY;
    
    if (w<=0 || h<=0) return {};
    return {
        .origin = {minX, minY},
        .size = {w, h},
    };
}

inline constexpr Rect Inset(const Rect& x, const Edges& e) {
    Rect r = x;
    r.origin += Size{e.l, e.t};
    r.size   -= Size{e.l, e.t};
    r.size   -= Size{e.r, e.b};
    return r;
}

inline constexpr Rect Inset(const Rect& x, const Size& s) {
    return Inset(x, {.l=s.x, .r=s.x, .t=s.y, .b=s.y});
}

inline constexpr bool Empty(const Rect& x) {
    return x.size.x==0 || x.size.y==0;
}

inline constexpr bool HitTest(const Rect& r, const Point& p, Edges inset={}) {
    return !Empty(Intersection(Inset(r, inset), {p, {1,1}}));
}

inline constexpr bool HitTest(const Rect& r, const Point& p, Size inset) {
    return !Empty(Intersection(Inset(r, inset), {p, {1,1}}));
}

} // namespace UI
