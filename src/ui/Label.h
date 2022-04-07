#pragma once
#include "Color.h"

namespace UI {

class Label : public Control {
public:
    
    Size sizeIntrinsic(int width) {
        if (!_wrap) return {width, 1};
        std::vector<std::string> lines = LineWrap::Wrap(SIZE_MAX, size().x, _text);
        return { width, (int)lines.size() };
    }
    
    bool layout(const Window& win) override {
        if (!Control::layout(win)) return false;
        if (_wrap) {
            _lines = { _text };
        } else {
            _lines = LineWrap::Wrap(SIZE_MAX, size().x, _text);
        }
        return true;
    }
    
    bool draw(const Window& win) override {
        if (!Control::draw(win)) return false;
        
        const Rect f = frame();
        
        // Draw lines
        if (win.erased()) {
            int offY = 0;
            for (const std::string& line : _lines) {
                const size_t lineLen = UTF8::Strlen(line);
                const int lineWidth = std::min((int)lineLen, f.size.x);
                
                // Draw line
                Point p = f.point + Size{0, offY};
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
                win.drawText(p, lineWidth, line.c_str());
                offY++;
            }
        }
        
        return true;
    }
    
    const auto& text() const { return _text; }
    template <typename T> void text(const T& x) { _set(_text, x); }
    
    const auto& color() const { return _color; }
    template <typename T> void color(const T& x) { _set(_color, x); }
    
    const auto& attr() const { return _attr; }
    template <typename T> void attr(const T& x) { _set(_attr, x); }
    
    const auto& align() const { return _align; }
    template <typename T> void align(const T& x) { _set(_align, x); }
    
    const auto& wrap() const { return _wrap; }
    template <typename T> void wrap(const T& x) { _set(_wrap, x); }
    
private:
    std::string _text;
    Color _color;
    int _attr = 0;
    Align _align = Align::Left;
    bool _wrap = false;
    
    std::vector<std::string> _lines;
};

} // namespace UI