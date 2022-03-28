#pragma once

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

struct ColorPalette {
    Color normal            = COLOR_BLACK;
    Color selectionMove     = COLOR_BLUE;
    Color selectionCopy     = COLOR_GREEN;
    Color selectionSimilar  = COLOR_BLACK;
    Color disabledText      = COLOR_BLACK;
    Color menu              = COLOR_RED;
    Color error             = COLOR_RED;
    
    std::vector<std::reference_wrapper<Color>> all() {
        return {
            normal,
            selectionMove,
            selectionCopy,
            selectionSimilar,
            disabledText,
            menu,
            error
        };
    };
    
    std::vector<std::reference_wrapper<Color>> all() const {
        return (((ColorPalette*)this)->all());
//        return const_cast<int&>(const_cast<const Foo*>(this)->get());
//        
//        return (((ColorPalette*)this)->all());
//        return std::remove_const_t<decltype(*this)>::all();
//        return all();
    };
};

} // namespace UI
