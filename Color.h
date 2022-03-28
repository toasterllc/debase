#pragma once
#include <atomic>

namespace UI {

class Color {
public:
    Color() : Color(COLOR_BLACK) {}
    Color(int idx) : idx(idx) {}
    Color(int idx, NCURSES_COLOR_T r, NCURSES_COLOR_T g, NCURSES_COLOR_T b) :
    idx(idx), r(r), g(g), b(b), custom(true) {}
    
    int idx = 0;
    NCURSES_COLOR_T r = 0;
    NCURSES_COLOR_T g = 0;
    NCURSES_COLOR_T b = 0;
    bool custom = false;
    
    operator int() const { return COLOR_PAIR(idx); }
    bool operator ==(const Color& x) const { return !memcmp(this, &x, sizeof(Color)); }
};

class ColorPalette {
public:
    Color normal            = COLOR_BLACK;
    Color dimmed            = COLOR_BLACK;
    Color selection         = COLOR_BLUE;
    Color selectionSimilar  = COLOR_BLACK;
    Color selectionCopy     = COLOR_GREEN;
    Color menu              = COLOR_RED;
    Color error             = COLOR_RED;
    
    Color add(int r, int g, int b) {
        r = (r*1000)/255;
        g = (g*1000)/255;
        b = (b*1000)/255;
        const Color& c = _custom.emplace_back(_IdxCustom++, r, g, b);
        return c;
    }
    
    const std::vector<Color>& custom() { return _custom; }
    
    std::vector<std::reference_wrapper<Color>> all() {
        return {
            normal,
            dimmed,
            selection,
            selectionSimilar,
            selectionCopy,
            menu,
            error,
        };
    };
    
private:
    // _IdxCustomInit: start outside the standard 0-7 range because we don't want
    // to clobber the standard terminal colors. This is because reading the current
    // terminal colors isn't reliable (via color_content), therefore when we
    // restore colors on exit, we won't necessarily be restoring the original
    // color. So if we're going to clobber colors, clobber the colors that are less
    // likely to be used.
    static constexpr int _IdxCustomInit = 16;
    static inline std::atomic<int> _IdxCustom = _IdxCustomInit;
    
    std::vector<Color> _custom;
    
//    std::vector<std::reference_wrapper<Color>> all() {
//        return {
//            normal,
//            selectionMove,
//            selectionCopy,
//            selectionSimilar,
//            dimmed,
//            menu,
//            error
//        };
//    };
//    
//    std::vector<std::reference_wrapper<Color>> all() const {
//        return (((ColorPalette*)this)->all());
////        return const_cast<int&>(const_cast<const Foo*>(this)->get());
////        
////        return (((ColorPalette*)this)->all());
////        return std::remove_const_t<decltype(*this)>::all();
////        return all();
//    };
};

} // namespace UI
