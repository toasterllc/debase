#pragma once

namespace UI {

struct Color {
    int idx = 0;
    int r = 0;
    int g = 0;
    int b = 0;
    
    operator int() const { return COLOR_PAIR(idx); }
    bool operator ==(const Color& x) const { return !memcmp(this, &x, sizeof(Color)); }
};

struct Colors {
    static constexpr Color SelectionMove    = Color{1,    0,    0, 1000};
    static constexpr Color SelectionCopy    = Color{2,    0, 1000,    0};
    static constexpr Color SelectionSimilar = Color{3,  550,  550, 1000};
    static constexpr Color SubtitleText     = Color{4,  300,  300,  300};
    static constexpr Color MenuBorder = Color{5,  800,  300,  300};
    
    static constexpr Color All[] = {
        SelectionMove,
        SelectionCopy,
        SelectionSimilar,
        SubtitleText,
        MenuBorder,
    };
};

} // namespace UI
