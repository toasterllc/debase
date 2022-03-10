#pragma once
#include <optional>
#include "Panel.h"
#include "Color.h"
#include "Attr.h"

namespace UI {

class _BorderedPanel : public _Panel, public std::enable_shared_from_this<_BorderedPanel> {
public:
    _BorderedPanel(const Size& size) {
        setSize(size);
        _drawNeeded = true;
    }
    
    void setBorderColor(std::optional<Color> x) {
        if (_borderColor == x) return;
        _borderColor = x;
        _drawNeeded = true;
    }
    
    void draw() {
        UI::Attr attr;
        if (_borderColor) attr = Attr(shared_from_this(), COLOR_PAIR(*_borderColor));
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

using BorderedPanel = std::shared_ptr<_BorderedPanel>;

} // namespace UI
