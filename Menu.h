#pragma once
#include <algorithm>
#include "Panel.h"

namespace UI {

struct MenuButton {
    std::string name;
    std::string key;
};

class _Menu : public _Panel, public std::enable_shared_from_this<_Menu> {
public:
    template<size_t T_ButtonCount>
    _Menu(const MenuButton*(&buttons)[T_ButtonCount]) : _buttons(buttons), _buttonCount(T_ButtonCount) {
        static_assert(T_ButtonCount>0, "Must have at least 1 button");
        
        // Find the longest button to set our width
        int width = 0;
        for (size_t i=0; i<_buttonCount; i++) {
            int w = (int)_buttons[i]->name.size() + (int)_buttons[i]->key.size();
            width = std::max(width, w);
        }
        
        int w = width + KeySpacing + 2*(BorderSize+InsetX);
        int h = (RowHeight*(int)_buttonCount)-1 + 2;
        setSize({w, h});
        _drawNeeded = true;
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
//        ButtonPtr highlightButton = nullptr;
//        
//        if (p) {
//            Rect frame = rect();
//            Rect bounds = {{}, frame.size};
//            Point off = *p-frame.point;
//            
//            if (!Empty(Intersection(bounds, {off, {1,1}}))) {
//                size_t idx = std::clamp((size_t)0, _buttons.size()-1, (size_t)(off.y / RowHeight));
//                highlightButton = _buttons[idx];
//            }
//        }
//        
//        if (_highlightButton != highlightButton) {
//            _highlightButton = highlightButton;
//            _drawNeeded = true;
//        }
//        
//        return _highlightButton;
//    }
    
    const MenuButton* updateMousePosition(const Point& p) {
        Rect frame = rect();
        Size inset = {BorderSize+InsetX, BorderSize};
        Rect innerBounds = Inset({{}, frame.size}, inset);
        Point off = p-frame.point;
        
        const MenuButton* highlightButton = nullptr;
        if (!Empty(Intersection(innerBounds, {off, {1,1}}))) {
            off -= innerBounds.point;
            size_t idx = std::min(_buttonCount-1, (size_t)(off.y / RowHeight));
            highlightButton = _buttons[idx];
        }
        
        if (_highlightButton != highlightButton) {
            _highlightButton = highlightButton;
            _drawNeeded = true;
        }
        
        return _highlightButton;
    }
    
    void draw() {
        erase();
        
        const int w = rect().size.x;
        size_t idx = 0;
        for (size_t i=0; i<_buttonCount; i++) {
//            ::wmove(*this, p.y, p.x);
//            ::vw_printw(*this, fmt, args);
            
//            UI::Attr attr(shared_from_this(), _buttons[i]==_highlightButton ? A_UNDERLINE : A_NORMAL);
            
            int y = 1 + (int)idx*RowHeight;
            
            // Draw button name
            {
//                UI::Attr attr(shared_from_this(), Colors::Menu);
                UI::Attr attr(shared_from_this(), _buttons[i]==_highlightButton ? Colors::Menu|A_BOLD : A_NORMAL);
                drawText({BorderSize+InsetX, y}, "%s", _buttons[i]->name.c_str());
            }
            
//            wchar_t str[] = L"⌫";
//            mvwaddchnstr(*this, y, InsetX, (chtype*)str, 1);
            
//            drawText({InsetX, y}, "⌫");
//            drawText({InsetX, y}, "%s", button.name.c_str());
            
            // Draw button key
            {
                UI::Attr attr(shared_from_this(), Colors::SubtitleText);
                drawText({w-BorderSize-InsetX-(int)_buttons[i]->key.size(), y}, "%s", _buttons[i]->key.c_str());
            }
            
            // Draw separator
            if (i != _buttonCount-1) {
                UI::Attr attr(shared_from_this(), Colors::Menu);
                drawLineHoriz({0,y+1}, w);
            }
            
//            if (_buttons[i] == _highlightButton) {
//                drawLineHoriz({0,y}, 10);
//            }
            
            idx++;
        }
        
        // Draw border
        {
            UI::Attr attr(shared_from_this(), Colors::Menu);
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
    static constexpr int BorderSize = 1;
    static constexpr int InsetX     = 1;
    static constexpr int KeySpacing = 2;
    static constexpr int RowHeight  = 2;
    const MenuButton**const _buttons = nullptr;
    const size_t _buttonCount = 0;
    const MenuButton* _highlightButton = nullptr;
    bool _drawNeeded = false;
};

using Menu = std::shared_ptr<_Menu>;

} // namespace UI
