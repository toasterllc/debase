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
};

using Point = Vector;
using Size = Vector;

struct Rect {
    Point point;
    Size size;
    
    bool operator ==(const Rect& x) const { return point==x.point && size==x.size; }
    bool operator !=(const Rect& x) const { return !(*this == x); }
    
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
        KeyLeft         = KEY_LEFT,
        KeyRight        = KEY_RIGHT,
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
        Point point;
        mmask_t bstate = 0;
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

struct ExitRequest : std::exception {};
struct WindowResize : std::exception {};

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

} // namespace UI
