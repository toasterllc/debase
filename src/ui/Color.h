#pragma once
#include <atomic>
#include <vector>
#include "lib/toastbox/RuntimeError.h"

namespace UI {

using ColorIdx       = NCURSES_COLOR_T;
using ColorPairIdx   = NCURSES_PAIRS_T;

// ColorPair: represents an ncurses color pair
struct ColorPair {
    ColorPairIdx idx = WhiteOnBlackIdx;
    
    ColorPair() {}
    ColorPair(ColorPairIdx idx) : idx(idx) {}
    
    // WhiteOnBlackIdx: color-pair 0 is ncurses' immutable white-on-black color pair
    static constexpr ColorPairIdx WhiteOnBlackIdx = 0;
    operator attr_t() const { return COLOR_PAIR(idx); }
};

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
    ColorPair add(ColorIdx colorIdx) {
        _colorPairSwappers.emplace_back(_colorPairIdx, colorIdx, -1);
        return _colorPairIdx++;
    }
    
    // add(): create a new color (specified in the range [0,255])
    // with the given RGB components, and assign it to a new color
    // pair as the foreground color
    ColorPair add(uint8_t r, uint8_t g, uint8_t b) {
        _colorIdxSwappers.emplace_back(_colorIdx,
            ((_ColorComponent)r*1000)/255,
            ((_ColorComponent)g*1000)/255,
            ((_ColorComponent)b*1000)/255
        );
        return add(_colorIdx++);
    }
    
private:
    // _ColorComponent: represents a single r/g/b component of a color.
    // For some reason, ncurses uses NCURSES_COLOR_T to mean a color index,
    // but also the value of a color component. See color_content() for
    // proof -- the first argument is a color index, and the remaining
    // arguments are the color components, but they all have the same type.
    using _ColorComponent = NCURSES_COLOR_T;
    
//    // _ColorIdx: represents an ncurses color
//    struct _ColorIdx {
//        ColorIdx idx = COLOR_BLACK;
//        
//        _ColorIdx() {}
//        _ColorIdx(ColorIdx idx) : idx(idx) {}
//        operator ColorIdx() const { return idx; }
//    };
    
    // _ColorIdxSwapper: sets the RGB components of an ncurses color, restoring the original
    // RGB components upon destruction
    class _ColorIdxSwapper {
    public:
        _ColorIdxSwapper() {}
        _ColorIdxSwapper(ColorIdx idx, _ColorComponent r, _ColorComponent g, _ColorComponent b) : _s{.idx=idx} {
            // Remember the previous RGB values for color index `idx`
            int ir = ::color_content(*_s.idx, &_s.r, &_s.g, &_s.b);
            if (ir != OK) throw std::runtime_error("color_content() failed");
            
            // Set the new RGB values for color index `idx`
            ir = ::init_color(*_s.idx, r, g, b);
            if (ir != OK) throw std::runtime_error("init_color() failed");
        }
        
        _ColorIdxSwapper(const _ColorIdxSwapper& x) = delete;
        _ColorIdxSwapper(_ColorIdxSwapper&& x) {
            std::swap(_s, x._s);
        }
        
        _ColorIdxSwapper& operator =(_ColorIdxSwapper&& x) {
            std::swap(_s, x._s);
            return *this;
        }
        
        ~_ColorIdxSwapper() {
            if (_s.idx) {
                ::init_color(*_s.idx, _s.r, _s.g, _s.b);
            }
        }
        
    private:
        struct {
            std::optional<ColorIdx> idx;
            _ColorComponent r = -1;
            _ColorComponent g = -1;
            _ColorComponent b = -1;
        } _s;
    };
    
    // _ColorPairSwapper: sets the foreground/background colors of a ncurses color pair,
    // restoring the pair's original fg/bg upon destruction
    class _ColorPairSwapper {
    public:
        _ColorPairSwapper() {}
        _ColorPairSwapper(const ColorPair& pair, ColorIdx fg, ColorIdx bg) : _s{.pair=pair} {
            // Remember the previous fg/bg colors for the color pair c.idx
            int ir = ::pair_content(_s.pair->idx, &_s.fg, &_s.bg);
            if (ir != OK) throw std::runtime_error("pair_content() failed");
            
            // Set the new RGB values for the color `c.idx`
            ir = ::init_pair(_s.pair->idx, fg, bg);
            if (ir != OK) throw std::runtime_error("init_pair() failed");
        }
        
        _ColorPairSwapper(const _ColorPairSwapper& x) = delete;
        
        _ColorPairSwapper(_ColorPairSwapper&& x) {
            std::swap(_s, x._s);
        }
        
        _ColorPairSwapper& operator =(_ColorPairSwapper&& x) {
            std::swap(_s, x._s);
            return *this;
        }
        
        ~_ColorPairSwapper() {
            if (_s.pair) {
                ::init_pair(_s.pair->idx, _s.fg, _s.bg);
            }
        }
        
    private:
        struct {
            std::optional<ColorPair> pair;
            ColorIdx fg = -1;
            ColorIdx bg = -1;
        } _s;
    };
    
    // _ColorIdxInit: start outside the standard 0-7 range because we don't want
    // to clobber the standard terminal colors. This is because reading the current
    // terminal colors isn't reliable (via color_content), therefore when we
    // restore colors on exit, we won't necessarily be restoring the original
    // color. So if we're going to clobber colors, clobber the colors that are less
    // likely to be used.
    static constexpr ColorIdx _ColorIdxInit = 16;

    ColorIdx _colorIdx = _ColorIdxInit;
    ColorPairIdx _colorPairIdx = 1;
    
    std::vector<_ColorIdxSwapper> _colorIdxSwappers;
    std::vector<_ColorPairSwapper> _colorPairSwappers;
};

// Color: for clients' conveniece, Color==ColorPair, since clients don't care what a ColorPair is
using Color = ColorPair;

} // namespace UI
