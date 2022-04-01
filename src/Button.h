#pragma once
#include <algorithm>
#include "Window.h"
#include "UTF8.h"
#include "Control.h"

namespace UI {

class Button : public Control {
public:
//    static auto Make(const ColorPalette& colors) {
//        return std::make_shared<UI::Button>(colors);
//    }
    
    Button(const ColorPalette& colors) : Control(colors) {}
    
    virtual void draw(const _Window& win) override {
        size_t labelLen = UTF8::Strlen(label);
        size_t keyLen = UTF8::Strlen(key);
        
        if (drawBorder) {
            UI::_Window::Attr color = win.attr(enabled ? colors.normal : colors.dimmed);
            win.drawRect(frame);
        }
        
        int offY = (frame.size.y-1)/2;
        
        // Draw button name
        Point plabel;
        Point pkey;
        if (center) {
            int textWidth = (int)labelLen + (int)keyLen + (!key.empty() ? KeySpacing : 0);
            int leftX = insetX + ((frame.size.x-2*insetX)-textWidth)/2;
            int rightX = frame.size.x-leftX;
            
            plabel = frame.point + Size{leftX, offY};
            pkey = frame.point + Size{rightX-(int)keyLen, offY};
        
        } else {
            plabel = frame.point + Size{insetX, offY};
            pkey = frame.point + Size{frame.size.x-(int)keyLen-insetX, offY};
        }
        
        {
            UI::_Window::Attr bold;
            UI::_Window::Attr color;
            if (enabled)                      bold = win.attr(A_BOLD);
            if (highlight && enabled) color = win.attr(colors.menu);
            else if (!enabled)                color = win.attr(colors.dimmed);
            win.drawText(plabel, "%s", label.c_str());
        }
        
        // Draw button key
        {
            UI::_Window::Attr color = win.attr(colors.dimmed);
            win.drawText(pkey, "%s", key.c_str());
        }
    }
    
    std::string label;
    std::string key;
    bool enabled = false;
    bool center = false;
    bool drawBorder = false;
    int insetX = 0;
    bool highlight = false;
    bool mouseActive = false;
    
private:
    static constexpr int KeySpacing = 2;
};

using ButtonPtr = std::shared_ptr<Button>;

} // namespace UI
