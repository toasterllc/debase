#pragma once
#include <optional>
#include "Panel.h"
#include "Color.h"

class ShadowPanel : public Panel {
public:
    ShadowPanel(const Size& size) {
        setSize(size);
        _drawNeeded = true;
    }
    
    void setOutlineColor(std::optional<Color> x) {
        if (_outlineColor == x) return;
        _outlineColor = x;
        _drawNeeded = true;
    }
    
    void draw() {
        Window::Attr attr;
        if (_outlineColor) attr = setAttr(COLOR_PAIR(*_outlineColor));
        drawBorder();
        _drawNeeded = false;
    }
    
    void drawIfNeeded() {
        if (_drawNeeded) {
            draw();
        }
    }
    
private:
    std::optional<Color> _outlineColor;
    bool _drawNeeded = false;
};
