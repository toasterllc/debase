#pragma once
#include <algorithm>
#include "Panel.h"
#include "Button.h"
#include "UTF8.h"

namespace UI {

struct MenuOptions {
    const ColorPalette& colors;
    Window parentWindow;
    Point position;
    std::string title;
    std::vector<Button> buttons;
    bool allowTruncate = false;
};

class _Menu : public _Panel, public std::enable_shared_from_this<_Menu> {
public:
    // Padding(): the additional width/height on top of the size of the buttons
    static constexpr Size Padding() {
        return Size{2*(_BorderSize+_InsetX), 2*_BorderSize};
    }
    
    _Menu(const MenuOptions& opts) : _opts(opts) {
        // Find the longest button to set our width
        int width = 0;
        for (Button button : _opts.buttons) {
            width = std::max(width, button->options().frame.size.x);
        }
        
        width += Padding().x;
        
        const Size sizeMax = _opts.parentWindow->size()-_opts.position;
        const int x = _BorderSize+_InsetX;
        int y = _BorderSize;
        int height = Padding().y;
        size_t idx = 0;
        for (Button button : _opts.buttons) {
            ButtonOptions& opts = button->options();
            int newHeight = height;
            // Add space for separator
            // If we're not the first button, add space for the separator at the top of the button
            if (idx) {
                y += _SeparatorHeight;
                newHeight += _SeparatorHeight;
            }
            
            // Set button position (after separator)
            opts.frame.point = {x,y};
            
            // Add space for button
            y += opts.frame.size.y;
            newHeight += opts.frame.size.y;
            
            // Bail if the button won't fit in the available height
            if (_opts.allowTruncate && newHeight>sizeMax.y) break;
            
            height = newHeight;
            idx++;
        }
        _buttonCount = idx;
        
        setSize({width, height});
        setPosition(_opts.position);
    }
    
    Button updateMouse(const Point& p) {
        Rect frame = _Panel::frame();
        Point off = p-frame.point;
        bool mouseActive = HitTest(frame, p);
        
        Button mouseOverButton = nullptr;
        for (Button button : _opts.buttons) {
            ButtonOptions& opts = button->options();
            opts.mouseActive = mouseActive;
            bool hit = button->hitTest(off, {1,1});
            if (hit && !mouseOverButton) {
                opts.highlight = true;
                mouseOverButton = button;
            } else {
                opts.highlight = false;
            }
        }
        
        return mouseOverButton;
    }
    
    void draw() {
        const int width = bounds().size.x;
        erase();
        
        for (size_t i=0; i<_buttonCount; i++) {
            Button button = _opts.buttons[i];
            button->draw(*this);
            
            // Draw separator
            if (i != _buttonCount-1) {
                UI::_Window::Attr color = attr(_opts.colors.menu);
                drawLineHoriz({0, button->options().frame.ymax()+1}, width);
            }
        }
        
        // Draw border
        {
            UI::_Window::Attr color = attr(_opts.colors.menu);
            drawBorder();
            
            // Draw title
            if (!_opts.title.empty()) {
                UI::_Window::Attr bold = attr(A_BOLD);
                int offX = (width-(int)UTF8::Strlen(_opts.title))/2;
                drawText({offX,0}, " %s ", _opts.title.c_str());
            }
        }
    }
    
    const MenuOptions& options() { return _opts; }
    
private:
    static constexpr int _BorderSize      = 1;
    static constexpr int _SeparatorHeight = 1;
    static constexpr int _InsetX          = 1;
    static constexpr int _KeySpacing      = 2;
    static constexpr int _RowHeight       = 2;
    
    MenuOptions _opts;
    size_t _buttonCount = 0;
};

using Menu = std::shared_ptr<_Menu>;

} // namespace UI
