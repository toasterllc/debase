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

class Colors {
private:
    // _Idx0: start outside the standard 0-7 range because we don't want to clobber the standard terminal colors.
    // This is because reading the current terminal colors isn't reliable (via color_content), therefore when we
    // restore colors on exit, we won't necessarily be restoring the original color. So if we're going to clobber
    // colors, clobber the colors that are less likely to be used.
    static constexpr int _Idx0 = 16;
    
public:
    static constexpr Color SelectionMove    = Color{_Idx0+0,    0,    0, 1000};
    static constexpr Color SelectionCopy    = Color{_Idx0+1,    0, 1000,    0};
    static constexpr Color SelectionSimilar = Color{_Idx0+2,  550,  550, 1000};
    static constexpr Color SubtitleText     = Color{_Idx0+3,  300,  300,  300};
    static constexpr Color Menu             = Color{_Idx0+4,  800,  300,  300};
    static constexpr Color Error            = Color{_Idx0+5, 1000,    0,    0};
    
    static inline std::vector<Color> All = {
        SelectionMove,
        SelectionCopy,
        SelectionSimilar,
        SubtitleText,
        Menu,
        Error,
    };
};

} // namespace UI
