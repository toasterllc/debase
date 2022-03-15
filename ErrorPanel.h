#pragma once
#include <optional>
#include "Panel.h"
#include "Color.h"
#include "Attr.h"

namespace UI {

class _ErrorPanel : public _Panel, public std::enable_shared_from_this<_ErrorPanel> {
public:
    _ErrorPanel(int width) {
        setSize(size);
        _drawNeeded = true;
    }
    
    void draw() {
        {
            UI::Attr attr(shared_from_this(), Color::Error);
            drawBorder();
        }
        
        _drawNeeded = false;
    }
    
    void drawIfNeeded() {
        if (_drawNeeded) {
            draw();
        }
    }
    
private:
    bool _drawNeeded = false;
};

using ErrorPanel = std::shared_ptr<_ErrorPanel>;

} // namespace UI
