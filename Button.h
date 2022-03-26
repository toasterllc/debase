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
    int hitTestExpand = 0;
    Rect frame;
};

class Button {
public:
    Button(const ButtonOptions& opts) : _opts(opts) {}
    
//    void setHighlight(bool highlight) {
//        if (highlight == _highlight) return;
//        _highlight = highlight;
//        _drawNeeded = true;
//    }
    
    bool hitTest(const UI::Point& p) {
        Rect rect = Inset(_opts.frame, {-_opts.hitTestExpand, -_opts.hitTestExpand});
        return !Empty(Intersection(rect, {p, {1,1}}));
    }
    
//    bool updateMouse(const Point& p) {
//        _highlight = hitTest(p);
//        return _highlight;
//    }
    
    void draw(Window win) const {
        size_t labelLen = UTF8::Strlen(_opts.label);
        size_t keyLen = UTF8::Strlen(_opts.key);
        
        if (_opts.drawBorder) {
            UI::Attr attr(win, (_opts.enabled ? _opts.colors.normal : _opts.colors.subtitleText));
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
            UI::Attr attr;
            if (_highlight && _opts.enabled) attr = UI::Attr(win, _opts.colors.menu|A_BOLD);
            else if (!_opts.enabled)         attr = UI::Attr(win, _opts.colors.subtitleText);
            else                             attr = UI::Attr(win, A_NORMAL);
            win->drawText(plabel, "%s", _opts.label.c_str());
        }
        
        // Draw button key
        {
            UI::Attr attr(win, _opts.colors.subtitleText);
            win->drawText(pkey, "%s", _opts.key.c_str());
        }
    }
    
    bool highlight() const { return _highlight; }
    void highlight(bool x) { _highlight = x; }
    
    const ButtonOptions& opts() const {
        return _opts;
    }
    
    Rect frame() const {
        return _opts.frame;
    }
    
private:
    static constexpr int KeySpacing = 2;
    ButtonOptions _opts;
    bool _highlight = false;
};

} // namespace UI
