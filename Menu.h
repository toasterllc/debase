#pragma once
#include <algorithm>
#include "Panel.h"
#include "Button.h"
#include "UTF8.h"

namespace UI {

struct MenuOptions {
    const ColorPalette& colors;
    std::string title;
    std::vector<Button> buttons;
};

class _Menu : public _Panel, public std::enable_shared_from_this<_Menu> {
public:
    _Menu(const MenuOptions& opts) : _opts(opts) {
        // Find the longest button to set our width
        int width = 0;
        for (Button button : _opts.buttons) {
//            int w = (int)UTF8::Strlen(opts.label) + (int)UTF8::Strlen(opts.key);
            width = std::max(width, button->options().frame.size.x);
        }
        
        width += 2*(_BorderSize+_InsetX);
        
//        buttonWidth += KeySpacing;
        
//        const ButtonOptions& opts, const ColorPalette& colors, Window win, Rect frame
        
        const int x = _BorderSize+_InsetX;
        int y = _BorderSize;
        int height = 2*_BorderSize;
        for (Button button : _opts.buttons) {
//            const int x = BorderSize;
//            const int y = BorderSize + (int)idx*RowHeight;
            
            ButtonOptions& opts = button->options();
//            opts.insetX = InsetX;
//            opts.colors = colors;
            opts.frame.point = {x,y};
//            opts.hitTestExpand = 1;
            
            y += opts.frame.size.y+_SeparatorHeight;
            height += opts.frame.size.y;
            if (button != _opts.buttons.back()) {
                height += _SeparatorHeight;
            }
        }
        
//        int w = width + 2*(BorderSize+InsetX);
//        int h = (RowHeight*(int)buttons.size())-1 + 2;
        setSize({width, height});
//        _drawNeeded = true;
//        
//        _buttons = _opts.buttons;
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
    
    Button hitTest(const Point& p) {
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
        
        size_t idx = 0;
        for (Button button : _opts.buttons) {
            button->draw(shared_from_this());
            
//            // Draw separator
//            if (button != _buttons.back()) {
//                int len = w;
//                Point p = {0, button->options().frame.ymax()+1};
//                cchar_t c = {
//                    .chars = L"═",
//                };
//                mvwhline_set(*this, p.y, p.x, &c, len);
////                UI::Attr attr(shared_from_this(), _colors.menu);
////                drawLineHoriz({0, button->options().frame.ymax()+1}, w);
//            }
            
            // Draw separator
            if (idx != _opts.buttons.size()-1) {
                UI::Attr attr(shared_from_this(), _opts.colors.menu);
//                UI::Attr attr(shared_from_this(), _colors.menu | (!idx ? A_UNDERLINE|A_BOLD : 0));
                drawLineHoriz({0, button->options().frame.ymax()+1}, width);
            }
            
            
//            // Draw separator
//            if (idx != _buttons.size()-1) {
//                UI::Attr attr(shared_from_this(), _colors.menu | (!idx ? A_UNDERLINE|A_BOLD : 0));
//                drawLineHoriz({0, button->options().frame.ymax()+1}, w);
//            }
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
            
            idx++;
        }
        
        // Draw border
        {
            UI::Attr attr(shared_from_this(), _opts.colors.menu);
            drawBorder();
            
            // Draw title
            if (!_opts.title.empty()) {
                int offX = (width-(int)UTF8::Strlen(_opts.title))/2;
                drawText({offX,0}, " %s ", _opts.title.c_str());
            }
        }
        
//        _drawNeeded = false;
    }
    
    const MenuOptions& options() { return _opts; }
    
//    const std::vector<Button>& buttons() const { return _buttons; }
//    
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
    
    MenuOptions _opts;
    
//    const ColorPalette& _colors;
//    std::string _title;
//    std::vector<Button> _buttons;
//    bool _drawNeeded = false;
};

using Menu = std::shared_ptr<_Menu>;

} // namespace UI
