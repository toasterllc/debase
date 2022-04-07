#pragma once
#include "Window.h"

namespace UI {

class Panel : public Window {
public:
    Panel() : Window(nullptr) {
        _panel = ::new_panel(*this);
        assert(_panel);
        
        // Give ourself an initial size (otherwise origin()
        // doesn't work until the size is set)
        size({1,1});
    }
    
    ~Panel() {
        ::del_panel(_panel);
        _panel = nullptr;
    }
    
    void origin(const Point& p) const {
        ::move_panel(*this, p.y, p.x);
    }
    
    bool visible() const {
        return !::panel_hidden(*this);
    }
    
    void visible(bool v) {
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

using PanelPtr = std::shared_ptr<Panel>;

} // namespace UI
