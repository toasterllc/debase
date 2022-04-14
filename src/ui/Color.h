#pragma once
#include <atomic>
#include <vector>
#include "lib/toastbox/RuntimeError.h"

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
    
//    Color() {}
//    Color(int idx, int r, int g, int b) : idx(idx), rgb({r,g,b}) {}
    bool operator ==(const Color& x) const { return idx==x.idx && rgb==x.rgb; }
};

struct ColorPair {
    int idx = WhiteOnBlackIdx;
    std::optional<int> fg;
    std::optional<int> bg;
    
    // WhiteOnBlackIdx: color-pair 0 is ncurses' immutable white-on-black color pair
    static constexpr int WhiteOnBlackIdx = 0;
//    ColorPair() {}
//    ColorPair(int idx) : idx(idx) {}
//    ColorPair(int idx, Color fg) : idx(idx), fg(fg) {}
//    ColorPair(int idx, Color fg, Color bg) : idx(idx), fg(fg), bg(bg) {}
    operator int() const { return COLOR_PAIR(idx); }
    
//    ColorPair() {}
//    ColorPair(int idx) : idx(idx) {}
//    ColorPair(int idx, Color fg) : idx(idx), fg(fg) {}
//    ColorPair(int idx, Color fg, Color bg) : idx(idx), fg(fg), bg(bg) {}
//    operator int() const { return COLOR_PAIR(idx); }
};







class ColorSwapper {
public:
    ColorSwapper() {}
    ColorSwapper(const Color& c) {
        assert(c.rgb);
        
        // Remember the previous RGB values for color index `idx`
        NCURSES_COLOR_T r=-1, g=-1, b=-1;
        int ir = ::color_content(c.idx, &r, &g, &b);
        if (ir != OK) throw std::runtime_error("color_content() failed");
        _prev = Color{.idx=c.idx, .rgb=RGB{r,g,b}};
        
        // Set the new RGB values for color index `idx`
        ir = ::init_color(c.idx, c.rgb->r, c.rgb->g, c.rgb->b);
        if (ir != OK) throw std::runtime_error("init_color() failed");
    }
    
    ColorSwapper(const ColorSwapper& x) = delete;
    ColorSwapper(ColorSwapper&& x) {
        std::swap(_prev, x._prev);
    }
    
    ColorSwapper& operator =(ColorSwapper&& x) {
        std::swap(_prev, x._prev);
        return *this;
    }
    
    ~ColorSwapper() {
        if (_prev) {
            ::init_color(_prev->idx, _prev->rgb->r, _prev->rgb->g, _prev->rgb->b);
        }
    }
    
private:
    std::optional<Color> _prev;
};


class ColorPairSwapper {
public:
    ColorPairSwapper() {}
    ColorPairSwapper(const ColorPair& p) {
        assert(p.fg || p.bg);
        
        // Remember the previous fg/bg colors for the color pair c.idx
        NCURSES_COLOR_T fg=-1, bg=-1;
        int ir = ::pair_content(p.idx, &fg, &bg);
        if (ir != OK) throw std::runtime_error("pair_content() failed");
        _prev = {.idx=p.idx, .fg=fg, .bg=bg};
        
        // Set the new RGB values for the color `c.idx`
        ir = ::init_pair(p.idx, (p.fg ? *p.fg : -1), (p.bg ? *p.bg : -1));
        if (ir != OK) throw std::runtime_error("init_pair() failed");
    }
    
    ColorPairSwapper(const ColorPairSwapper& x) = delete;
    
    ColorPairSwapper(ColorPairSwapper&& x) {
        std::swap(_prev, x._prev);
    }
    
    ColorPairSwapper& operator =(ColorPairSwapper&& x) {
        std::swap(_prev, x._prev);
        return *this;
    }
    
    ~ColorPairSwapper() {
        if (_prev) {
            ::init_pair(_prev->idx, *_prev->fg, *_prev->bg);
        }
    }
    
private:
    std::optional<ColorPair> _prev;
};








//struct ColorPairSwapper {
//    int idx = WhiteOnBlackIdx;
//    std::optional<int> fg;
//    std::optional<int> bg;
//    
//    // WhiteOnBlackIdx: color-pair 0 is ncurses' immutable white-on-black color pair
//    static constexpr int WhiteOnBlackIdx = 0;
////    ColorPair() {}
////    ColorPair(int idx) : idx(idx) {}
////    ColorPair(int idx, Color fg) : idx(idx), fg(fg) {}
////    ColorPair(int idx, Color fg, Color bg) : idx(idx), fg(fg), bg(bg) {}
//    operator int() const { return COLOR_PAIR(idx); }
//    
////    ColorPair() {}
////    ColorPair(int idx) : idx(idx) {}
////    ColorPair(int idx, Color fg) : idx(idx), fg(fg) {}
////    ColorPair(int idx, Color fg, Color bg) : idx(idx), fg(fg), bg(bg) {}
////    operator int() const { return COLOR_PAIR(idx); }
//};







//
//
//class CustomColors {
//public:
//    // add(): create a new color pair with a given color index as the foreground color
//    ColorPair add(int idx) {
//        const ColorPair colorPair = {.idx=_colorPairIdx, .fg=idx};
//        _colorPairSwappers.emplace_back(colorPair);
//        _colorPairIdx++;
//        return colorPair;
//    }
//    
//    // add(): create a new color pair with a given RGB components as the foreground color
//    ColorPair add(uint8_t r, uint8_t g, uint8_t b) {
//        const Color color = {.idx=_colorIdx, .rgb=RGB{((int)r*1000)/255, ((int)g*1000)/255, ((int)b*1000)/255}};
//        const ColorPair colorPair = {.idx=_colorPairIdx, .fg=_colorIdx};
//        
//        _colorSwappers.emplace_back(color);
//        _colorPairSwappers.emplace_back(colorPair);
//        
////        const Color colorPrev = _ColorSet(color);
////        const ColorPair colorPairPrev = _ColorPairSet(colorPair);
////        
////        _colorsPrev.push_back(colorPrev);
////        _colorPairsPrev.push_back(colorPairPrev);
//        
//        _colorIdx++;
//        _colorPairIdx++;
//        
//        return colorPair;
//    }
//    
////    ~CustomColors() {
////        for (const Color& color : _colorsPrev) _ColorSet(color);
////        for (const ColorPair& colorPair : _colorPairsPrev) _ColorPairSet(colorPair);
////    }
//    
//private:
////    static Color _ColorSet(const Color& c) {
////        if (!c.rgb) return c;
////        
////        Color x = c;
////        
////        // Remember the previous RGB values for color index c.idx
////        {
////            NCURSES_COLOR_T r,g,b;
////            color_content(c.idx, &r, &g, &b);
////            x.rgb.emplace(RGB{r,g,b});
////        }
////        
////        // Set the new RGB values for the color `c.idx`
////        {
////            ::init_color(c.idx, c.rgb->r, c.rgb->b, c.rgb->b);
////        }
////        return x;
////    }
////    
////    static ColorPair _ColorPairSet(const ColorPair& c) {
////        if (!c.fg && !c.bg) return c;
////        
////        ColorPair x = c;
//////        if (c.fg) x.fg = _ColorInit(*c.fg);
//////        if (c.bg) x.bg = _ColorInit(*c.bg);
////        
////        // Remember the previous fg/bg colors for the color pair c.idx
////        {
////            NCURSES_COLOR_T fg, bg;
////            pair_content(c.idx, &fg, &bg);
////            if (c.fg) x.fg = fg;
////            if (c.bg) x.bg = bg;
////        }
////        
////        // Set the new RGB values for the color `c.idx`
////        {
////            ::init_pair(c.idx, (c.fg ? *c.fg : -1), (c.bg ? *c.bg : -1));
////        }
////        
////        return x;
////    }
//    
//    // _ColorIdxInit: start outside the standard 0-7 range because we don't want
//    // to clobber the standard terminal colors. This is because reading the current
//    // terminal colors isn't reliable (via color_content), therefore when we
//    // restore colors on exit, we won't necessarily be restoring the original
//    // color. So if we're going to clobber colors, clobber the colors that are less
//    // likely to be used.
//    static constexpr int _ColorIdxInit = 16;
//    
//    int _colorIdx = _ColorIdxInit;
//    int _colorPairIdx = 1;
//    
//    std::vector<ColorSwapper> _colorSwappers;
//    std::vector<ColorPairSwapper> _colorPairSwappers;
//};
//



class ColorPalette {
public:
    ColorPair normal;
    ColorPair dimmed;
    ColorPair selection;
    ColorPair selectionSimilar;
    ColorPair selectionCopy;
    ColorPair menu;
    ColorPair error;
    
    // add(): create a new color pair with a given color index as the foreground color
    ColorPair add(int idx) {
        const ColorPair colorPair = {.idx=_colorPairIdx, .fg=idx};
        _colorPairSwappers.emplace_back(colorPair);
        _colorPairIdx++;
        return colorPair;
    }
    
    // add(): create a new color with the given RGB components, and assign it to a new color pair as the foreground color
    ColorPair add(uint8_t r, uint8_t g, uint8_t b) {
        const Color color = {.idx=_colorIdx, .rgb=RGB{((int)r*1000)/255, ((int)g*1000)/255, ((int)b*1000)/255}};
        const ColorPair colorPair = {.idx=_colorPairIdx, .fg=_colorIdx};
        
        _colorSwappers.emplace_back(color);
        _colorPairSwappers.emplace_back(colorPair);
        
        _colorIdx++;
        _colorPairIdx++;
        
        return colorPair;
    }
    
private:
    // _ColorIdxInit: start outside the standard 0-7 range because we don't want
    // to clobber the standard terminal colors. This is because reading the current
    // terminal colors isn't reliable (via color_content), therefore when we
    // restore colors on exit, we won't necessarily be restoring the original
    // color. So if we're going to clobber colors, clobber the colors that are less
    // likely to be used.
    static constexpr int _ColorIdxInit = 16;
    
    int _colorIdx = _ColorIdxInit;
    int _colorPairIdx = 1;
    
    std::vector<ColorSwapper> _colorSwappers;
    std::vector<ColorPairSwapper> _colorPairSwappers;
};

} // namespace UI
