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
    
    Size sizeIntrinsic() override {
        // First button sets the width
        const int width = (!buttons.empty() ? buttons[0]->frame.size.x : 0) + Padding().x;
        
        // Calculate the height by iterating over every button until no more fit in `sizeMax`
        const Size sizeMax = containerSize-frame().point;
        int height = Padding().y;
        int rem = sizeMax.y;
        bool first = true;
        for (ButtonPtr button : buttons) {
            const int add = (!first ? _SeparatorHeight : 0) + button->frame.size.y;
            // Bail if the button won't fit in the available height
            if (allowTruncate && add>rem) break;
            height += add;
            rem -= add;
            first = false;
        }
        
        return {width, height};
    }
    
    void layout() override {
        // Short-circuit if layout isn't needed
        if (!layoutNeeded) return;
        Panel::layout();
        
        const int ymax = size().y-_BorderSize;
        const int x = _BorderSize+_InsetX;
        int y = _BorderSize;
        
        buttonsVisible.clear();
        for (ButtonPtr button : buttons) {
            // Add space for separator
            // If we're not the first button, add space for the separator at the top of the button
            if (!buttonsVisible.empty()) y += _SeparatorHeight;
            
            // Set button position (after separator)
            Rect& f = button->frame;
            f.point = {x,y};
            
            // Add space for button
            y += f.size.y;
            
            // Set the expanded hit test size so that the menu doesn't have any dead zones
            if (buttonsVisible.empty()) button->hitTestExpand.t = 1;
            button->hitTestExpand.b = 1;
            
            // Bail if the bottom of the bottom extends beyond our max y
            if (allowTruncate && y>ymax) break;
            buttonsVisible.push_back(button);
        }
    }
    
    void draw() override {
        drawNeeded |= _drawNeeded();
        if (!drawNeeded) return;
        
        os_log(OS_LOG_DEFAULT, "Menu::draw()");
        
        Panel::draw();
        
        const int width = bounds().size.x;
        
        for (ButtonPtr button : buttonsVisible) {
            button->draw(*this);
            
            // Draw separator
            if (button != buttonsVisible.back()) {
                Window::Attr color = attr(colors.menu);
                drawLineHoriz({0, button->frame.ymax()+1}, width);
            }
        }
        
        // Draw border
        {
            Window::Attr color = attr(colors.menu);
            drawBorder();
            
            // Draw title
            if (!title.empty()) {
                Window::Attr bold = attr(A_BOLD);
                int offX = (width-(int)UTF8::Strlen(title))/2;
                drawText({offX,0}, " %s ", title.c_str());
            }
        }
    }
    
    bool handleEvent(const Event& ev) override {
        auto& md = _mouseDown;
        // See if any of the buttons want the event
        for (ButtonPtr button : buttonsVisible) {
            bool handled = button->handleEvent(*this, ev, md.sensitive);
            if (handled) {
                if (dismissAction) dismissAction(*this);
                return false;
            }
        }
        
        if (ev.type == Event::Type::Mouse) {
            // Update the mouseActive state for all of our buttons
            bool inside = HitTest(bounds(), ev.mouse.point);
            for (ButtonPtr button : buttonsVisible) {
                button->mouseActive(inside);
            }
            
            // Handle mouse down
            if (ev.mouseDown() && !inside) {
                if (dismissAction) dismissAction(*this);
                return false;
            
            // Handle mouse up
            } else if (ev.mouseUp(md.sensitive)) {
                auto duration = std::chrono::steady_clock::now()-md.mouseDownTime;
                
                // Mouse-up occurred and no buttons handled it, so dismiss the menu if either:
                //   1. we haven't entered stay-open mode, and the period to allow stay-open mode has passed, or
                ///  2. we've entered stay-open mode, and the mouse-up was outside of the menu
                if ((!md.stayOpen && duration>=_StayOpenExpiration) || (md.stayOpen && !inside)) {
                    if (dismissAction) dismissAction(*this);
                    return false;
                }
                
                // Start listening for left mouse up
                md.sensitive |= Event::MouseButtons::Left;
                md.stayOpen = true;
            }
        
        } else if (ev.type == Event::Type::KeyEscape) {
            if (dismissAction) dismissAction(*this);
            return false;
        }
        return true;
    }
    
    void track(const Event& mouseDownEvent) {
        _mouseDown.mouseDownTime = std::chrono::steady_clock::now();
        _mouseDown.ev = mouseDownEvent;
        _mouseDown.sensitive = Event::MouseButtons::Right;
        _mouseDown.stayOpen = false;
        Window::track();
    }
    
    const ColorPalette& colors;
    Size containerSize;
    std::string title;
    bool allowTruncate = false;
    std::vector<ButtonPtr> buttons;
    std::vector<ButtonPtr> buttonsVisible;
    std::function<void(Menu&)> dismissAction;
    
private:
    static constexpr int _BorderSize      = 1;
    static constexpr int _SeparatorHeight = 1;
    static constexpr int _InsetX          = 1;
    static constexpr int _KeySpacing      = 2;
    static constexpr int _RowHeight       = 2;
    
    static constexpr auto _StayOpenExpiration = std::chrono::milliseconds(300);
    
    bool _drawNeeded() const {
        for (ButtonPtr button : buttonsVisible) {
            if (button->drawNeeded) return true;
        }
        return false;
    }
    
    size_t _buttonCount = 0;
    
    struct {
        std::chrono::steady_clock::time_point mouseDownTime = {};
        UI::Event ev;
        Event::MouseButtons sensitive;
        bool stayOpen = false;
    } _mouseDown;
};

using MenuPtr = std::shared_ptr<Menu>;

} // namespace UI
