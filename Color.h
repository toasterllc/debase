#pragma once

struct Color {
    int idx = 0;
    int r = 0;
    int g = 0;
    int b = 0;
    
    operator int() const { return idx; }
    bool operator ==(const Color& x) const { return !memcmp(this, &x, sizeof(Color)); }
};

namespace Colors {
    constexpr Color SelectionMove = Color{1,    0,    0, 1000};
    constexpr Color SelectionCopy = Color{2,    0, 1000,    0};
    constexpr Color SubtitleText  = Color{3,  300,  300,  300};
} // namespace Colors
