#pragma once
#include <algorithm>
#include "Window.h"

namespace UI {

struct ButtonOptions {
    std::string label;
    std::string key;
    bool enabled = false;
    std::optional<Color> borderColor;
    int insetX = 0;
};

class Button {
public:
    Button(const ButtonOptions& opts, const ColorPalette& colors, Rect frame) :
    _opts(opts), _colors(colors), _frame(frame) {
        _drawNeeded = true;
    }
    
//    void setHighlight(bool highlight) {
//        if (highlight == _highlight) return;
//        _highlight = highlight;
//        _drawNeeded = true;
//    }
    
    bool updateMousePosition(const Point& p) {
        bool highlight = !Empty(Intersection(_frame, {p, {1,1}}));
        if (_highlight != highlight) {
            _highlight = highlight;
            _drawNeeded = true;
        }
        return _highlight;
    }
    
    void draw(Window win) {
        if (_opts.borderColor) {
            UI::Attr attr(win, *_opts.borderColor);
            win->drawRect(_frame);
        }
        
        // Draw button name
        {
            UI::Attr attr;
            
            if (_highlight && _opts.enabled) {
                attr = UI::Attr(win, _colors.menu|A_BOLD);
            
            } else if (!_opts.enabled) {
                attr = UI::Attr(win, _colors.subtitleText);
            
            } else {
                attr = UI::Attr(win, A_NORMAL);
            }
            
            win->drawText(_frame.point+Size{_opts.insetX,0}, "%s", _opts.label.c_str());
        }
        
        // Draw button key
        {
            UI::Attr attr(win, _colors.subtitleText);
            Point p = _frame.point+Size{_frame.size.x-(int)_opts.key.size()-_opts.insetX,0};
            win->drawText(p, "%s", _opts.key.c_str());
        }
        
        _drawNeeded = false;
    }
    
    void drawIfNeeded(Window win) {
        if (_drawNeeded) {
            draw(win);
        }
    }
    
    const ButtonOptions& opts() const {
        return _opts;
    }
    
    Rect frame() const {
        return _frame;
    }
    
private:
    ButtonOptions _opts;
    ColorPalette _colors;
    Rect _frame;
    bool _highlight = false;
    bool _drawNeeded = false;
};

} // namespace UI
