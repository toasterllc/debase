#pragma once
#include <atomic>
#include <vector>

namespace UI {

class Color {
public:
    Color() : Color(COLOR_BLACK) {}
    Color(int idx) : idx(idx) {}
    Color(int idx, NCURSES_COLOR_T r, NCURSES_COLOR_T g, NCURSES_COLOR_T b) : idx(idx), r(r), g(g), b(b) {}
    
    int idx = 0;
    NCURSES_COLOR_T r = 0;
    NCURSES_COLOR_T g = 0;
    NCURSES_COLOR_T b = 0;
    
    operator int() const { return COLOR_PAIR(idx); }
    bool operator ==(const Color& x) const { return idx==x.idx && r==x.r && g==x.g && b==x.b; }
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
    
    Color add(uint8_t r, uint8_t g, uint8_t b) {
        Color c(_IdxCustom++, ((NCURSES_COLOR_T)r*1000)/255, ((NCURSES_COLOR_T)g*1000)/255, ((NCURSES_COLOR_T)b*1000)/255);
        _custom.push_back(c);
        return c;
    }
    
    std::vector<Color>& custom() { return _custom; }
    const std::vector<Color>& custom() const { return _custom; }
    
    std::vector<std::reference_wrapper<const Color>> colors() const {
        return { normal, dimmed, selection, selectionSimilar, selectionCopy, menu, error };
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
};

} // namespace UI
