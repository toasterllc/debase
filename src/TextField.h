#pragma once
#include <algorithm>
#include "Window.h"
#include "UTF8.h"

namespace UI {

struct TextFieldOptions {
    const ColorPalette& colors;
//    bool enabled = false;
//    bool center = false;
//    bool drawBorder = false;
//    int insetX = 0;
//    bool highlight = false;
//    bool mouseActive = false;
    Rect frame;
    bool active = false;
};

class _TextField : public std::enable_shared_from_this<_TextField> {
public:
    _TextField(const TextFieldOptions& opts) : _opts(opts) {}
    
    bool hitTest(const UI::Point& p, UI::Size expand={0,0}) {
        return UI::HitTest(_opts.frame, p, expand);
    }
    
    void draw(Window win) const {
        UI::Attr color;
        if (!_opts.active) color = UI::Attr(win, _opts.colors.dimmed);
        win->drawLineHoriz(_opts.frame.point, _opts.frame.size.x);
//        win->drawRect(_opts.frame);
    }
    
    UI::Event handleEvent(const UI::Event& ev) {
        return ev;
    }
    
    const std::string& value() const {
        return _value;
    }
    
    TextFieldOptions& options() { return _opts; }
    const TextFieldOptions& options() const { return _opts; }
    
private:
    static constexpr int KeySpacing = 2;
    TextFieldOptions _opts;
    std::string _value;
};

using TextField = std::shared_ptr<_TextField>;

} // namespace UI
