#pragma once
#include <optional>
#include "Panel.h"
#include "Color.h"

class BorderedPanel : public Panel {
public:
    BorderedPanel(const Size& size) {
        setSize(size);
        _drawNeeded = true;
    }
    
    void setBorderColor(std::optional<Color> x) {
        if (_borderColor == x) return;
        _borderColor = x;
        _drawNeeded = true;
    }
    
    void draw() {
        Window::Attr attr;
        if (_borderColor) attr = setAttr(COLOR_PAIR(*_borderColor));
        drawBorder();
        _drawNeeded = false;
    }
    
    void drawIfNeeded() {
        if (_drawNeeded) {
            draw();
        }
    }
    
private:
    std::optional<Color> _borderColor;
    bool _drawNeeded = false;
};
