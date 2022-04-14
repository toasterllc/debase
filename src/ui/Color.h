#pragma once
#include <atomic>
#include <vector>

namespace UI {

struct RGB {
    int r = 0;
    int g = 0;
    int b = 0;
    bool operator ==(const RGB& x) const { return r==x.r && g==x.g && b==x.b; }
};

struct Color {
    int idx = COLOR_BLACK;
    std::optional<RGB> rgb;
    
    Color() {}
    Color(int idx, int r, int g, int b) : idx(idx), rgb({r,g,b}) {}
    bool operator ==(const Color& x) const { return idx==x.idx && rgb==x.rgb; }
};

struct ColorPair {
    int idx = WhiteOnBlackIdx;
    std::optional<Color> fg;
    std::optional<Color> bg;
    
    // WhiteOnBlackIdx: color-pair 0 is ncurses' immutable white-on-black color pair
    static constexpr int WhiteOnBlackIdx = 0;
    ColorPair() {}
    ColorPair(int idx) : idx(idx) {}
    ColorPair(int idx, Color fg) : idx(idx), fg(fg) {}
    ColorPair(int idx, Color fg, Color bg) : idx(idx), fg(fg), bg(bg) {}
    operator int() const { return COLOR_PAIR(idx); }
};

struct ColorPalette {
    ColorPair normal;
    ColorPair dimmed;
    ColorPair selection;
    ColorPair selectionSimilar;
    ColorPair selectionCopy;
    ColorPair menu;
    ColorPair error;
    
    std::vector<std::reference_wrapper<const ColorPair>> colorPairs() const {
        return { normal, dimmed, selection, selectionSimilar, selectionCopy, menu, error };
    };
};






//class ColorPalette {
//public:
//    Color normal            = COLOR_BLACK;
//    Color dimmed            = COLOR_BLACK;
//    Color selection         = COLOR_BLUE;
//    Color selectionSimilar  = COLOR_BLACK;
//    Color selectionCopy     = COLOR_GREEN;
//    Color menu              = COLOR_RED;
//    Color error             = COLOR_RED;
//    
//    Color add(uint8_t r, uint8_t g, uint8_t b) {
//        Color c(_IdxCustom++, ((NCURSES_COLOR_T)r*1000)/255, ((NCURSES_COLOR_T)g*1000)/255, ((NCURSES_COLOR_T)b*1000)/255);
//        _custom.push_back(c);
//        return c;
//    }
//    
//    std::vector<Color>& custom() { return _custom; }
//    const std::vector<Color>& custom() const { return _custom; }
//    
//    std::vector<std::reference_wrapper<const Color>> colors() const {
//        return { normal, dimmed, selection, selectionSimilar, selectionCopy, menu, error };
//    };
//    
//private:
//    // _IdxCustomInit: start outside the standard 0-7 range because we don't want
//    // to clobber the standard terminal colors. This is because reading the current
//    // terminal colors isn't reliable (via color_content), therefore when we
//    // restore colors on exit, we won't necessarily be restoring the original
//    // color. So if we're going to clobber colors, clobber the colors that are less
//    // likely to be used.
//    static constexpr int _IdxCustomInit = 16;
//    static inline std::atomic<int> _IdxCustom = _IdxCustomInit;
//    
//    std::vector<Color> _custom;
//};

} // namespace UI
