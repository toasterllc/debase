#pragma once
#include "Window.h"

namespace UI {

class Screen : public Window {
public:
    using Window::Window;
    
    Point origin() const override {
        return { getbegx((WINDOW*)(*this)), getbegy((WINDOW*)(*this)) };
    }
    
    void origin(const Point& x) override {
        // Can't set screen origin
        abort();
    }
    
    Size size() const override {
        return { getmaxx((WINDOW*)(*this)), getmaxy((WINDOW*)(*this)) };
    }
    
    void size(const Size& x) override {
        // Can't set screen size
        abort();
    }
    
    Size windowSize() const override { return Window::windowSize(); }
    void windowSize(const Size& s) override {} // Ignore attempts to set screen size
};

using ScreenPtr = std::shared_ptr<Screen>;

} // namespace UI
