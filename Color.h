#pragma once

namespace UI {

struct Color {
    int idx = 0;
    NCURSES_COLOR_T r = 0;
    NCURSES_COLOR_T g = 0;
    NCURSES_COLOR_T b = 0;
    
    operator int() const { return COLOR_PAIR(idx); }
    bool operator ==(const Color& x) const { return !memcmp(this, &x, sizeof(Color)); }
};

struct ColorPalette {
    Color selectionMove;
    Color selectionCopy;
    Color selectionSimilar;
    Color subtitleText;
    Color menu;
    Color error;
    
    std::vector<std::reference_wrapper<Color>> all() {
        return {
            selectionMove,
            selectionCopy,
            selectionSimilar,
            subtitleText,
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
