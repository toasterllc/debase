#pragma once
#include "lib/ncurses/include/panel.h"

class Panel : public Window {
public:
    Panel() : Window(Window::New) {
        _s.panel = ::new_panel(*this);
        assert(_s.panel);
    }
    
    // Move constructor: use move assignment operator
    Panel(Panel&& x) { *this = std::move(x); }
    
    // Move assignment operator
    Panel& operator=(Panel&& x) {
        printf("MOVE PANEL\n");
        Window::operator=(std::move(x));
        _s = std::move(x._s);
        x._s = {};
        return *this;
    }
    
    ~Panel() {
        ::del_panel(*this);
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
    
    operator PANEL*() const { return _s.panel; }
    
private:
    struct {
        PANEL* panel = nullptr;
    } _s = {};
};
