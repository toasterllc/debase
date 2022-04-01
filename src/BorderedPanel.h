#pragma once
#include <optional>
#include "Panel.h"
#include "Color.h"

namespace UI {

class BorderedPanel : public Panel {
public:
    BorderedPanel(const Size& size) {
        setSize(size);
    }
    
    void setBorderColor(std::optional<Color> x) {
        if (_borderColor == x) return;
        _borderColor = x;
        drawNeeded = true;
    }
    
    void draw() {
        if (!drawNeeded) return;
        Panel::draw();
        
        Window::Attr color;
        if (_borderColor) color = attr(*_borderColor);
        drawBorder();
    }
    
private:
    std::optional<Color> _borderColor;
};

using BorderedPanelPtr = std::shared_ptr<BorderedPanel>;

} // namespace UI
