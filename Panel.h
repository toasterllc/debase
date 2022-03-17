#pragma once
#include "Window.h"

namespace UI {

class _Panel : public _Window {
public:
    _Panel() {
        _panel = ::new_panel(*this);
        assert(_panel);
    }
    
    ~_Panel() {
        ::del_panel(_panel);
        _panel = nullptr;
    }
    
    void setPosition(const Point& p) {
        ::move_panel(*this, p.y, p.x);
    }
    
    bool visible() const {
        return !::panel_hidden(*this);
    }
    
    void setVisible(bool v) {
        if (_visible == v) return;
        _visible = v;
        if (_visible) ::show_panel(*this);
        else          ::hide_panel(*this);
    }
    
    void orderFront() {
        ::top_panel(*this);
    }
    
    void orderBack() {
        ::bottom_panel(*this);
    }
    
    operator PANEL*() const { return _panel; }
    
private:
    PANEL* _panel = nullptr;
    Point _pos;
    bool _visible = true;
};

using Panel = std::shared_ptr<_Panel>;

} // namespace UI
