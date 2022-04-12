#pragma once
#include "Color.h"

namespace UI {

class Label : public View {
public:
    using View::View;
    
    Size sizeIntrinsic(Size constraint) override {
        assert(constraint.x >= 0);
        assert(constraint.y >= 0);
        
        if (!_wrap) {
            const int prefixWidth = (int)UTF8::Len(_prefix);
            const int suffixWidth = (int)UTF8::Len(_suffix);
            const int fullWidth = prefixWidth + (int)UTF8::Len(_text) + suffixWidth;
            const int width = constraint.x!=ConstraintNone ? std::min(constraint.x, fullWidth) : fullWidth;
            return {width, 1};
        }
        
        const int width = constraint.x!=ConstraintNone ? constraint.x : (int)UTF8::Len(_text);
        const LineWrap::Options opts = {
            .width = (size_t)width,
            .allowEmptyLines = _allowEmptyLines,
        };
        std::vector<std::string> lines = LineWrap::Wrap(opts, _text);
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
        if (_lines.empty() && !_text.empty()) {
            if (!_wrap) {
                _lines = { _text };
            } else {
                const LineWrap::Options opts = {
                    .width = (size_t)size().x,
                    .height = (size_t)size().y,
                    .allowEmptyLines = _allowEmptyLines,
                };
                _lines = LineWrap::Wrap(opts, _text);
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
//            if (l.empty()) continue;
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
    template <typename T> bool text(const T& x) { return _set(_text, x); }
    
    const auto& prefix() const { return _prefix; }
    template <typename T> bool prefix(const T& x) { return _set(_prefix, x); }
    
    const auto& suffix() const { return _suffix; }
    template <typename T> bool suffix(const T& x) { return _set(_suffix, x); }
    
    const auto& textAttr() const { return _textAttr; }
    template <typename T> bool textAttr(const T& x) { return _set(_textAttr, x); }
    
    const auto& align() const { return _align; }
    template <typename T> bool align(const T& x) { return _set(_align, x); }
    
    const auto& centerSingleLine() const { return _centerSingleLine; }
    template <typename T> bool centerSingleLine(const T& x) { return _set(_centerSingleLine, x); }
    
    const auto& wrap() const { return _wrap; }
    template <typename T> bool wrap(const T& x) { return _set(_wrap, x); }
    
    const auto& allowEmptyLines() const { return _allowEmptyLines; }
    template <typename T> bool allowEmptyLines(const T& x) { return _set(_allowEmptyLines, x); }
    
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
    bool _allowEmptyLines = false;
    
    std::vector<std::string> _lines;
};

using LabelPtr = std::shared_ptr<Label>;

} // namespace UI
