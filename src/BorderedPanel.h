#pragma once
#include <optional>
#include "Panel.h"
#include "Color.h"

namespace UI {

class BorderedPanel : public Panel {
public:
    BorderedPanel(const Size& s) {
        size(s);
    }
    
    void setBorderColor(std::optional<Color> x) {
        if (_borderColor == x) return;
        _borderColor = x;
        drawNeeded(true);
    }
    
    bool draw() override {
        if (!Panel::draw()) return false;
        
        Window::Attr color;
        if (_borderColor) color = attr(*_borderColor);
        drawBorder();
        
        return true;
    }
    
private:
    std::optional<Color> _borderColor;
};

using BorderedPanelPtr = std::shared_ptr<BorderedPanel>;

} // namespace UI
