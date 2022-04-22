#pragma once
#include "Window.h"

namespace UI {

class Panel : public Window {
public:
    Panel() {
        _panel = ::new_panel(*this);
        assert(_panel);
//        // Give ourself an initial size (otherwise origin()
//        // doesn't work until the size is set)
//        size({1,1});
    }
    
    ~Panel() {
        ::del_panel(_panel);
        _panel = nullptr;
    }
    
    Point windowOrigin() const override { return Window::windowOrigin(); }
    bool windowOrigin(const Point& p) override {
        if (p == windowOrigin()) return false;
        ::move_panel(*this, p.y, p.x);
        return true;
    }
    
    using Window::layout;
    void layout(GraphicsState gstate) override {
        if (!visible()) return;
        
        // If panels need to be ordered during this layout pass, do so now
        if (gstate.orderPanels) ::top_panel(*this);
        
        Window::layout(gstate);
    }
    
//    void origin(const Point& p) override {
//        const Point off = treeOrigin();
//        ::move_panel(*this, off.y+p.y, off.x+p.x);
//    }
    
    bool visible() const override {
        return !::panel_hidden(*this);
    }
    
    bool visible(bool v) override {
        if (visible() == v) return false;
        if (v) {
            ::show_panel(*this);
            screen().orderPanelsNeeded(true);
        
        } else {
            ::hide_panel(*this);
        }
        return true;
    }
    
    void addedToSuperview(View& superview) override {
        Window::addedToSuperview(superview);
        screen().orderPanelsNeeded(true);
    }
    
//    void orderFront() {
//        ::top_panel(*this);
//    }
//    
//    void orderBack() {
//        ::bottom_panel(*this);
//    }
    
    operator PANEL*() const { return _panel; }
    
private:
//    void _orderFront() {
//        ::top_panel(*this);
//    }
    
    PANEL* _panel = nullptr;
    Point _pos;
};

using PanelPtr = std::shared_ptr<Panel>;

} // namespace UI
