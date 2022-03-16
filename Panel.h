#pragma once
#include "lib/ncurses/include/panel.h"
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
    
    std::optional<Point> adjustedPosition(const Point& p) const {
        int x = p.x;
        int y = p.y;
        int ir = ::mvwinadjpos(*this, y, x, &y, &x);
        if (ir != OK) return std::nullopt;
        return Point{x,y};
    }
    
    bool validPosition(const Point& p) const {
        std::optional<Point> pa = adjustedPosition(p);
        if (!pa) return false;
        return p == *pa;
    }
    
    void setPosition(const Point& p) {
        std::optional<Point> pa = adjustedPosition(p);
        if (!pa || _pos==*pa) return;
        ::move_panel(*this, pa->y, pa->x);
        _pos = *pa;
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
