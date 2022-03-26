#pragma once
#include <algorithm>
#include "Panel.h"
#include "Button.h"
#include "UTF8.h"

namespace UI {

class _Menu : public _Panel, public std::enable_shared_from_this<_Menu> {
public:
    _Menu(const ColorPalette& colors, const std::vector<Button>& buttons) : _colors(colors) {
        // Find the longest button to set our width
        int width = 0;
        for (Button button : buttons) {
//            int w = (int)UTF8::Strlen(opts.label) + (int)UTF8::Strlen(opts.key);
            width = std::max(width, button->options().frame.size.x);
        }
        
        width += 2*(_BorderSize+_InsetX);
        
//        buttonWidth += KeySpacing;
        
//        const ButtonOptions& opts, const ColorPalette& colors, Window win, Rect frame
        
        const int x = _BorderSize+_InsetX;
        int y = _BorderSize;
        int height = 2*_BorderSize;
        for (Button button : buttons) {
//            const int x = BorderSize;
//            const int y = BorderSize + (int)idx*RowHeight;
            
            ButtonOptions& opts = button->options();
//            opts.insetX = InsetX;
//            opts.colors = colors;
            opts.frame.point = {x,y};
//            opts.hitTestExpand = 1;
            
            y += opts.frame.size.y+_SeparatorHeight;
            height += opts.frame.size.y;
            if (button != buttons.back()) {
                height += _SeparatorHeight;
            }
        }
        
//        int w = width + 2*(BorderSize+InsetX);
//        int h = (RowHeight*(int)buttons.size())-1 + 2;
        setSize({width, height});
//        _drawNeeded = true;
//        
        _buttons = buttons;
    }
    
    
//    _Menu(const std::vector<ButtonPtr>& buttons) : _buttons(buttons) {
//        // Require at least 1 button
//        assert(!buttons.empty());
//        
//        // Find the longest button to set our width
//        int width = 0;
//        for (ButtonPtr button : _buttons) {
//            int w = (int)button->name.size() + (int)button->key.size();
//            width = std::max(width, w);
//        }
//        
//        int w = width + KeySpacing + 2*InsetX;
//        int h = (RowHeight*(int)_buttons.size())-1 + 2;
//        setSize({w, h});
//        _drawNeeded = true;
//    }
    
//    std::optional<Button> hitTest() {
//        
//    }
    
//    ButtonPtr updateMousePosition(std::optional<Point> p) {
//        ButtonPtr mouseOverButton = nullptr;
//        
//        if (p) {
//            Rect frame = rect();
//            Rect bounds = {{}, frame.size};
//            Point off = *p-frame.point;
//            
//            if (!Empty(Intersection(bounds, {off, {1,1}}))) {
//                size_t idx = std::clamp((size_t)0, _buttons.size()-1, (size_t)(off.y / RowHeight));
//                mouseOverButton = _buttons[idx];
//            }
//        }
//        
//        if (_mouseOverButton != mouseOverButton) {
//            _mouseOverButton = mouseOverButton;
//            _drawNeeded = true;
//        }
//        
//        return _mouseOverButton;
//    }
    
    const Button hitTest(const Point& p) {
        Rect frame = _Panel::frame();
        Point off = p-frame.point;
        
        Button mouseOverButton = nullptr;
        for (Button button : _buttons) {
            bool hit = button->hitTest(off, {1,1});
            if (hit && !mouseOverButton) {
                button->options().highlight = true;
                mouseOverButton = button;
            } else {
                button->options().highlight = false;
            }
        }
        
        return mouseOverButton;
    }
    
    void draw() {
        erase();
        
        const int w = bounds().size.x;
        for (Button button : _buttons) {
            button->draw(shared_from_this());
            
            // Draw separator
            if (button != _buttons.back()) {
                UI::Attr attr(shared_from_this(), _colors.menu);
                drawLineHoriz({0, button->options().frame.ymax()+1}, w);
            }
//            
////        for (size_t i=0; i<_buttonCount; i++) {
////            ::wmove(*this, p.y, p.x);
////            ::vw_printw(*this, fmt, args);
//            
////            UI::Attr attr(shared_from_this(), _buttons[i]==_mouseOverButton ? A_UNDERLINE : A_NORMAL);
//            
//            int y = 1 + (int)idx*RowHeight;
//            
//            // Draw button name
//            {
////                UI::Attr attr(shared_from_this(), _colors.menu);
//                UI::Attr attr;
//                
//                if (&button==_mouseOverButton && button.opts().enabled) {
//                    attr = UI::Attr(shared_from_this(), _colors.menu|A_BOLD);
//                
//                } else if (!button.enabled) {
//                    attr = UI::Attr(shared_from_this(), _colors.subtitleText);
//                
//                } else {
//                    attr = UI::Attr(shared_from_this(), A_NORMAL);
//                }
//                
//                drawText({BorderSize+InsetX, y}, "%s", button.name.c_str());
//            }
//            
////            wchar_t str[] = L"⌫";
////            mvwaddchnstr(*this, y, InsetX, (chtype*)str, 1);
//            
////            drawText({InsetX, y}, "⌫");
////            drawText({InsetX, y}, "%s", button.name.c_str());
//            
//            // Draw button key
//            {
//                UI::Attr attr(shared_from_this(), _colors.subtitleText);
//                drawText({w-BorderSize-InsetX-(int)button.key.size(), y}, "%s", button.key.c_str());
//            }
//            
//            // Draw separator
//            
//            if (&button != &_buttons.back()) {
//                UI::Attr attr(shared_from_this(), _colors.menu);
//                drawLineHoriz({0,y+1}, w);
//            }
//            
////            if (_buttons[i] == _mouseOverButton) {
////                drawLineHoriz({0,y}, 10);
////            }
//            
//            idx++;
        }
        
        // Draw border
        {
            UI::Attr attr(shared_from_this(), _colors.menu);
            drawBorder();
        }
        
//        _drawNeeded = false;
    }
    
//    void drawIfNeeded() {
//        if (_drawNeeded) {
//            draw();
//        }
//    }
    
private:
    static constexpr int _BorderSize      = 1;
    static constexpr int _SeparatorHeight = 1;
    static constexpr int _InsetX          = 1;
    static constexpr int _KeySpacing      = 2;
    static constexpr int _RowHeight       = 2;
    const ColorPalette& _colors;
    std::vector<Button> _buttons;
//    bool _drawNeeded = false;
};

using Menu = std::shared_ptr<_Menu>;

} // namespace UI
