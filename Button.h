#pragma once
#include <algorithm>
#include "Window.h"

namespace UI {

struct ButtonOptions {
    ColorPalette colors;
    std::string label;
    std::string key;
    bool enabled = false;
    std::optional<Color> borderColor;
    int insetX = 0;
    Rect frame;
};

class Button {
public:
    Button(const ButtonOptions& opts) :
    _opts(opts) {
        _drawNeeded = true;
    }
    
//    void setHighlight(bool highlight) {
//        if (highlight == _highlight) return;
//        _highlight = highlight;
//        _drawNeeded = true;
//    }
    
    bool updateMousePosition(const Point& p) {
        bool highlight = !Empty(Intersection(_opts.frame, {p, {1,1}}));
        if (_highlight != highlight) {
            _highlight = highlight;
            _drawNeeded = true;
        }
        return _highlight;
    }
    
    void draw(Window win) {
        if (_opts.borderColor) {
            UI::Attr attr(win, *_opts.borderColor);
            win->drawRect(_opts.frame);
        }
        
        // Draw button name
        {
            UI::Attr attr;
            
            if (_highlight && _opts.enabled) {
                attr = UI::Attr(win, _opts.colors.menu|A_BOLD);
            
            } else if (!_opts.enabled) {
                attr = UI::Attr(win, _opts.colors.subtitleText);
            
            } else {
                attr = UI::Attr(win, A_NORMAL);
            }
            
            win->drawText(_opts.frame.point+Size{_opts.insetX,0}, "%s", _opts.label.c_str());
        }
        
        // Draw button key
        {
            UI::Attr attr(win, _opts.colors.subtitleText);
            Point p = _opts.frame.point+Size{_opts.frame.size.x-(int)_opts.key.size()-_opts.insetX,0};
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
        return _opts.frame;
    }
    
private:
    ButtonOptions _opts;
    bool _highlight = false;
    bool _drawNeeded = false;
};

} // namespace UI
