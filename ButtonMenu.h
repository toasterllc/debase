#pragma once
#include <algorithm>
#include "Panel.h"

namespace UI {

struct Button {
    std::string name;
    std::string key;
};

class _ButtonMenu : public _Panel, public std::enable_shared_from_this<_ButtonMenu> {
public:
    _ButtonMenu(const std::vector<Button>& buttons) : _buttons(buttons) {
        // Require at least 1 button
        assert(!buttons.empty());
        
        // Find the longest button to set our width
        int width = 0;
        for (const Button& button : _buttons) {
            int w = (int)button.name.size() + (int)button.key.size();
            width = std::max(width, w);
        }
        
        int w = width + KeySpacing + 2*InsetX;
        int h = (2*(int)_buttons.size())-1 + 2;
        setSize({w, h});
        _drawNeeded = true;
    }
    
    void draw() {
        const int w = rect().size.x;
        int y = 1;
        for (const Button& button : _buttons) {
//            ::wmove(*this, p.y, p.x);
//            ::vw_printw(*this, fmt, args);
            
            drawText({InsetX, y}, "%s", button.name.c_str());
            
//            wchar_t str[] = L"⌫";
//            mvwaddchnstr(*this, y, InsetX, (chtype*)str, 1);
            
//            drawText({InsetX, y}, "⌫");
//            drawText({InsetX, y}, "%s", button.name.c_str());
            drawText({w-InsetX-(int)button.key.size(), y}, "%s", button.key.c_str());
            drawLineHoriz({0,y+1}, w);
            y += 2;
        }
        
        drawBorder();
        
        _drawNeeded = false;
    }
    
    void drawIfNeeded() {
        if (_drawNeeded) {
            draw();
        }
    }
    
private:
    static constexpr int InsetX     = 2;
    static constexpr int KeySpacing = 2;
    std::vector<Button> _buttons;
    bool _drawNeeded = false;
};

using ButtonMenu = std::shared_ptr<_ButtonMenu>;

} // namespace UI
