#pragma once
#include <algorithm>
#include "Window.h"
#include "UTF8.h"

namespace UI {

struct ButtonOptions {
    ColorPalette colors;
    std::string label;
    std::string key;
    bool enabled = false;
    bool center = false;
    bool drawBorder = false;
    int insetX = 0;
    bool highlight = false;
    bool mouseActive = false;
    Rect frame;
};

class _Button : public std::enable_shared_from_this<_Button> {
public:
    _Button() {}
    
//    void setHighlight(bool highlight) {
//        if (highlight == _highlight) return;
//        _highlight = highlight;
//        _drawNeeded = true;
//    }
    
    bool hitTest(const UI::Point& p, UI::Size expand={0,0}) {
        return UI::HitTest(_opts.frame, p, expand);
    }
    
//    bool updateMouse(const Point& p) {
//        _highlight = hitTest(p);
//        return _highlight;
//    }
    
    virtual void draw(Window win) const {
        size_t labelLen = UTF8::Strlen(_opts.label);
        size_t keyLen = UTF8::Strlen(_opts.key);
        
        if (_opts.drawBorder) {
            UI::Attr attr(win, (_opts.enabled ? _opts.colors.normal : _opts.colors.dimmed));
            win->drawRect(_opts.frame);
        }
        
        int offY = (_opts.frame.size.y-1)/2;
        
        // Draw button name
        Point plabel;
        Point pkey;
        if (_opts.center) {
            int textWidth = (int)labelLen + (int)keyLen + (!_opts.key.empty() ? KeySpacing : 0);
            int leftX = _opts.insetX + ((_opts.frame.size.x-2*_opts.insetX)-textWidth)/2;
            int rightX = _opts.frame.size.x-leftX;
            
            plabel = _opts.frame.point + Size{leftX, offY};
            pkey = _opts.frame.point + Size{rightX-(int)keyLen, offY};
        
        } else {
            plabel = _opts.frame.point + Size{_opts.insetX, offY};
            pkey = _opts.frame.point + Size{_opts.frame.size.x-(int)keyLen-_opts.insetX, offY};
        }
        
        {
            UI::Attr bold;
            UI::Attr color;
            if (_opts.enabled)                    bold = UI::Attr(win, A_BOLD);
            if (_opts.highlight && _opts.enabled) color = UI::Attr(win, _opts.colors.menu);
            else if (!_opts.enabled)              color = UI::Attr(win, _opts.colors.dimmed);
            win->drawText(plabel, "%s", _opts.label.c_str());
        }
        
        // Draw button key
        {
            UI::Attr attr(win, _opts.colors.dimmed);
            win->drawText(pkey, "%s", _opts.key.c_str());
        }
    }
    
    ButtonOptions& options() { return _opts; }
    const ButtonOptions& options() const { return _opts; }
    
private:
    static constexpr int KeySpacing = 2;
    ButtonOptions _opts;
};

using Button = std::shared_ptr<_Button>;

} // namespace UI
