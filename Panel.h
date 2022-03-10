#pragma once
#include "lib/ncurses/include/panel.h"
#include "Window.h"

namespace UI {

class Panel : public Window {
public:
    Panel() : Window(Window::New) {
        _s = std::make_shared<_State>(*this);
    }
    
    void setPosition(const Point& p) {
        ::move_panel(*this, p.y, p.x);
    }
    
    bool visible() const {
        return !::panel_hidden(*this);
    }
    
    void setVisible(bool v) {
        if (v) ::show_panel(*this);
        else ::hide_panel(*this);
    }
    
    void orderFront() {
        ::top_panel(*this);
    }
    
    void orderBack() {
        ::bottom_panel(*this);
    }
    
    operator PANEL*() const { return _s->panel; }
    
private:
    class _State {
    public:
        _State(Window win) : win(win) {
            panel = ::new_panel(win);
            assert(panel);
        }
        
        ~_State() {
            ::del_panel(panel);
            panel = nullptr;
        }
        
        Window win; // Hold onto `win` explicitly since our panel needs it
        PANEL* panel = nullptr;
    };
    
    std::shared_ptr<_State> _s;
};

} // namespace UI
