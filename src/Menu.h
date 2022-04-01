#pragma once
#include <algorithm>
#include "Panel.h"
#include "Button.h"
#include "UTF8.h"

namespace UI {

class Menu : public Panel {
public:
    // Padding(): the additional width/height on top of the size of the buttons
    static constexpr Size Padding() {
        return Size{2*(_BorderSize+_InsetX), 2*_BorderSize};
    }
    
    Menu(const ColorPalette& colors) : colors(colors) {}
    
    ButtonPtr updateMouse(const Point& p) {
        Rect frame = Panel::frame();
        Point off = p-frame.point;
        bool mouseActive = HitTest(frame, p);
        
        ButtonPtr mouseOverButton = nullptr;
        for (ButtonPtr button : buttons) {
            button->mouseActive = mouseActive;
            bool hit = button->hitTest(off, {1,1});
            if (hit && !mouseOverButton) {
                button->highlight = true;
                mouseOverButton = button;
            } else {
                button->highlight = false;
            }
        }
        
        return mouseOverButton;
    }
    
    void layout() override {
        // Short-circuit if layout isn't needed
        if (!_layoutNeeded) return;
        _layoutNeeded = false;
        
        // Find the longest button to set our width
        int width = 0;
        for (ButtonPtr button : buttons) {
            width = std::max(width, button->frame.size.x);
        }
        
        width += Padding().x;
        
        const Size sizeMax = containerSize-frame().point;
        const int x = _BorderSize+_InsetX;
        int y = _BorderSize;
        int height = Padding().y;
        size_t idx = 0;
        for (ButtonPtr button : buttons) {
            int newHeight = height;
            // Add space for separator
            // If we're not the first button, add space for the separator at the top of the button
            if (idx) {
                y += _SeparatorHeight;
                newHeight += _SeparatorHeight;
            }
            
            // Set button position (after separator)
            Rect& f = button->frame;
            f.point = {x,y};
            
            // Add space for button
            y += f.size.y;
            newHeight += f.size.y;
            
            // Bail if the button won't fit in the available height
            if (allowTruncate && newHeight>sizeMax.y) break;
            
            height = newHeight;
            idx++;
        }
        _buttonCount = idx;
        
        setSize({width, height});
    }
    
    void draw() override {
        const int width = bounds().size.x;
        erase();
        
        for (size_t i=0; i<_buttonCount; i++) {
            ButtonPtr button = buttons[i];
            button->draw(*this);
            
            // Draw separator
            if (i != _buttonCount-1) {
                UI::Window::Attr color = attr(colors.menu);
                drawLineHoriz({0, button->frame.ymax()+1}, width);
            }
        }
        
        // Draw border
        {
            UI::Window::Attr color = attr(colors.menu);
            drawBorder();
            
            // Draw title
            if (!title.empty()) {
                UI::Window::Attr bold = attr(A_BOLD);
                int offX = (width-(int)UTF8::Strlen(title))/2;
                drawText({offX,0}, " %s ", title.c_str());
            }
        }
    }
    
    const ColorPalette& colors;
    Size containerSize;
    std::string title;
    std::vector<ButtonPtr> buttons;
    bool allowTruncate = false;
    
private:
    static constexpr int _BorderSize      = 1;
    static constexpr int _SeparatorHeight = 1;
    static constexpr int _InsetX          = 1;
    static constexpr int _KeySpacing      = 2;
    static constexpr int _RowHeight       = 2;
    
    bool _layoutNeeded = true;
    size_t _buttonCount = 0;
};

using MenuPtr = std::shared_ptr<Menu>;

} // namespace UI
