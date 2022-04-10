#pragma once
#include <list>
#include <chrono>
#include "Bitfield.h"

namespace UI {

struct Vector {
    int x = 0;
    int y = 0;
    
    bool operator ==(const Vector& v) const { return x==v.x && y==v.y; }
    bool operator !=(const Vector& v) const { return !(*this == v); }
    
    Vector operator +(const Vector& v) const { return {x+v.x, y+v.y}; }
    template <typename T> Vector operator +(const T& t) const { return {x+t, y+t}; }
    
    Vector& operator +=(const Vector& v) { x+=v.x; y+=v.y; return *this; }
    template <typename T> Vector& operator +=(const T& t) { x+=t; y+=t; return *this; }
    
    Vector operator -(const Vector& v) const { return {x-v.x, y-v.y}; }
    template <typename T> Vector operator -(const T& t) const { return {x-t, y-t}; }
    
    Vector& operator -=(const Vector& v) { x-=v.x; y-=v.y; return *this; }
    template <typename T> Vector& operator -=(const T& t) { x-=t; y-=t; return *this; }
    
    template <typename T> Vector operator *(const T& t) const { return {x*t, y*t}; }
    
    template <typename T> Vector& operator *=(const T& t) { x*=t; y*=t; return *this; }
};

using Point = Vector;
using Size = Vector;

struct Rect {
    Point origin;
    Size size;
    
    bool operator ==(const Rect& x) const { return origin==x.origin && size==x.size; }
    bool operator !=(const Rect& x) const { return !(*this == x); }
    
    int xmin() const { return origin.x; }
    int xmax() const { return origin.x+size.x-1; }
    
    int ymin() const { return origin.y; }
    int ymax() const { return origin.y+size.y-1; }
    
    Point tl() const { return {xmin(), ymin()}; }
    Point tr() const { return {xmax(), ymin()}; }
    Point bl() const { return {xmin(), ymax()}; }
    Point br() const { return {xmax(), ymax()}; }
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

class Screen;
class Window;
struct GraphicsState {
//        GraphicsState() {}
    operator bool() const { return screen; }
    
    Screen* screen = nullptr;
    const Window* window = nullptr;
    Point originWindow; // For drawing purposes (relative to nearest window)
    Point originScreen; // For event purposes (relative to screen)
    bool erased = false;
};

struct ExitRequest : std::exception {};
struct WindowResize : std::exception {};

enum class Align {
    Left,
    Center,
    Right,
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

inline constexpr Rect Inset(const Rect& x, const Size& s) {
    return {x.origin+s, x.size-s-s};
}

inline constexpr bool Empty(const Rect& x) {
    return x.size.x==0 || x.size.y==0;
}

inline constexpr bool HitTest(const Rect& r, const Point& p, Size expand={0,0}) {
    return !Empty(Intersection(Inset(r, {-expand.x,-expand.y}), {p, {1,1}}));
}

} // namespace UI
