#pragma once
#include "Color.h"

namespace UI {

class Label : public View {
public:
    using View::View;
    
    Size sizeIntrinsic(Size constraint) override {
        const int textWidth = (int)UTF8::Len(_text);
        const int width = constraint.x ? std::min(constraint.x, textWidth) : textWidth;
        if (!_wrap) return {width, 1};
        std::vector<std::string> lines = LineWrap::Wrap(SIZE_MAX, width, _text);
        return { width, (int)lines.size() };
    }
    
    void layout(const Window& win) override {
        if (!_wrap) {
            _lines = { _text };
        } else {
            _lines = LineWrap::Wrap(size().y, size().x, _text);
        }
    }
    
    void draw(const Window& win) override {
        const Rect f = frame();
        const int prefixWidth = (int)UTF8::Len(_prefix);
        const int suffixWidth = (int)UTF8::Len(_suffix);
        
        // Draw lines
        int offY = 0;
        for (const std::string& l : _lines) {
            if (l.empty()) continue;
            if (offY >= f.size.y) break;
            
            // Printed line = prefix + base + suffix
            const int availBaseWidth = std::max(0, f.size.x-prefixWidth-suffixWidth);
            // Truncate `base` to `availBaseWidth`
            const std::string base = UTF8::Truncated(l, availBaseWidth);
            // Truncate `line` to our frame width
            const std::string line = UTF8::Truncated(_prefix+base+_suffix, f.size.x);
            const int lineWidth = (int)UTF8::Len(line);
            
            // Draw line
            Align align = _align;
            if (_centerSingleLine && _lines.size()==1) {
                align = Align::Center;
            }
            
            Point p = f.origin + Size{0, offY};
            switch (_align) {
            case Align::Left:
                break;
            case Align::Center:
                p.x += std::max(0, (f.size.x-lineWidth)/2);
                break;
            case Align::Right:
                p.x += std::max(0, (f.size.x-lineWidth));
                break;
            }
            
            Window::Attr style = win.attr(_attr);
            win.drawText(p, line.c_str());
            offY++;
        }
    }
    
    const auto& text() const { return _text; }
    template <typename T> void text(const T& x) { _set(_text, x); }
    
    const auto& prefix() const { return _prefix; }
    template <typename T> void prefix(const T& x) { _set(_prefix, x); }
    
    const auto& suffix() const { return _suffix; }
    template <typename T> void suffix(const T& x) { _set(_suffix, x); }
    
    const auto& attr() const { return _attr; }
    template <typename T> void attr(const T& x) { _set(_attr, x); }
    
    const auto& align() const { return _align; }
    template <typename T> void align(const T& x) { _set(_align, x); }
    
    const auto& centerSingleLine() const { return _centerSingleLine; }
    template <typename T> void centerSingleLine(const T& x) { _set(_centerSingleLine, x); }
    
    const auto& wrap() const { return _wrap; }
    template <typename T> void wrap(const T& x) { _set(_wrap, x); }
    
private:
    std::string _text;
    std::string _prefix;
    std::string _suffix;
    int _attr = 0;
    Align _align = Align::Left;
    bool _centerSingleLine = false;
    bool _wrap = false;
    
    std::vector<std::string> _lines;
};

using LabelPtr = std::shared_ptr<Label>;

} // namespace UI
