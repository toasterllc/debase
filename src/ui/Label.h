#pragma once
#include "Color.h"

namespace UI {

class Label : public View {
public:
    using View::View;
    
    Size sizeIntrinsic(Size constraint) override {
        const int prefixWidth = (!_wrap ? (int)UTF8::Len(_prefix) : 0);
        const int suffixWidth = (!_wrap ? (int)UTF8::Len(_suffix) : 0);
        const int fullWidth = prefixWidth + (int)UTF8::Len(_text) + suffixWidth;
        const int width = constraint.x ? std::min(constraint.x, fullWidth) : fullWidth;
        if (!_wrap) return {width, 1};
        
        std::vector<std::string> lines = LineWrap::Wrap(SIZE_MAX, width, _text);
        return { width, (int)lines.size() };
    }
    
    void layout() override {
        // Anytime we layout, draw() needs to recreate its lines
        _lines.clear();
    }
    
    void draw() override {
        // Re-create lines
        // We do this in draw() (and not layout()) because the superview may set our text in its draw(),
        // so creating _lines in our layout() would occur before the superview set our text.
        if (_lines.empty()) {
            if (!_wrap) {
                _lines = { _text };
            } else {
                _lines = LineWrap::Wrap(size().y, size().x, _text);
            }
        }
        
        const Size s = size();
        const std::string prefix = (!_wrap ? _prefix : "");
        const std::string suffix = (!_wrap ? _suffix : "");
        const int prefixWidth = (int)UTF8::Len(prefix);
        const int suffixWidth = (int)UTF8::Len(suffix);
        
        // Draw lines
        int offY = 0;
        for (const std::string& l : _lines) {
            if (l.empty()) continue;
            if (offY >= s.y) break;
            
            // Printed line = prefix + base + suffix
            const int availBaseWidth = std::max(0, s.x-prefixWidth-suffixWidth);
            // Truncate `base` to `availBaseWidth`
            const std::string base = UTF8::Truncated(l, availBaseWidth);
            // Truncate `line` to our frame width
            const std::string line = UTF8::Truncated(prefix+base+suffix, s.x);
            const int lineWidth = (int)UTF8::Len(line);
            
            // Draw line
            Align align = _align;
            if (_centerSingleLine && _lines.size()==1) {
                align = Align::Center;
            }
            
            Point p = {0, offY};
            switch (_align) {
            case Align::Left:   break;
            case Align::Center: p.x += std::max(0, (s.x-lineWidth)/2); break;
            case Align::Right:  p.x += std::max(0, (s.x-lineWidth)); break;
            }
            
            Attr style = attr(_textAttr);
            drawText(p, line.c_str());
            offY++;
        }
    }
    
    const auto& text() const { return _text; }
    template <typename T> void text(const T& x) { _set(_text, x); }
    
    const auto& prefix() const { return _prefix; }
    template <typename T> void prefix(const T& x) { _set(_prefix, x); }
    
    const auto& suffix() const { return _suffix; }
    template <typename T> void suffix(const T& x) { _set(_suffix, x); }
    
    const auto& textAttr() const { return _textAttr; }
    template <typename T> void textAttr(const T& x) { _set(_textAttr, x); }
    
    const auto& align() const { return _align; }
    template <typename T> void align(const T& x) { _set(_align, x); }
    
    const auto& centerSingleLine() const { return _centerSingleLine; }
    template <typename T> void centerSingleLine(const T& x) { _set(_centerSingleLine, x); }
    
    const auto& wrap() const { return _wrap; }
    template <typename T> void wrap(const T& x) { _set(_wrap, x); }
    
private:
//    template <typename X, typename Y>
//    void _set(X& x, const Y& y) {
//        if (x == y) return;
//        x = y;
//        layoutNeeded(true);
//        eraseNeeded(true);
//        drawNeeded(true);
//    }
    
    std::string _text;
    std::string _prefix;
    std::string _suffix;
    int _textAttr = 0;
    Align _align = Align::Left;
    bool _centerSingleLine = false;
    bool _wrap = false;
    
    std::vector<std::string> _lines;
};

using LabelPtr = std::shared_ptr<Label>;

} // namespace UI
