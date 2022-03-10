#pragma once
#include <optional>
#include "Panel.h"
#include "Color.h"
#include "Attr.h"

namespace UI {

class BorderedPanel : public Panel {
public:
    BorderedPanel(const Size& size) {
        _s = std::make_shared<_State>();
        setSize(size);
        _s->drawNeeded = true;
    }
    
    void setBorderColor(std::optional<Color> x) {
        if (_s->borderColor == x) return;
        _s->borderColor = x;
        _s->drawNeeded = true;
    }
    
    void draw() {
        UI::Attr attr;
        if (_s->borderColor) attr = Attr(*this, COLOR_PAIR(*_s->borderColor));
        drawBorder();
        _s->drawNeeded = false;
    }
    
    void drawIfNeeded() {
        if (_s->drawNeeded) {
            draw();
        }
    }
    
private:
    struct _State {
        std::optional<Color> borderColor;
        bool drawNeeded = false;
    };
    
    std::shared_ptr<_State> _s;
};

} // namespace UI
