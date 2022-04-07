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
    
    Size sizeIntrinsic(Size constraint) override {
        // First button sets the width
        const int width = (!buttons.empty() ? buttons[0]->size().x : 0) + Padding().x;
        
        // Calculate the height by iterating over every button (until no more fit in `heightMax`, if supplied)
        int height = Padding().y;
        int rem = constraint.y;
        bool first = true;
        for (ButtonPtr button : buttons) {
            const int add = (!first ? _SeparatorHeight : 0) + button->size().y;
            // Bail if the button won't fit in the available height
            if (constraint.y && add>rem) break;
            height += add;
            rem -= add;
            first = false;
        }
        
        return {width, height};
    }
    
    bool layout() override {
        // Short-circuit if layout isn't needed
        if (!Panel::layout()) return false;
        
        const int ymax = size().y-_BorderSize;
        const int x = _BorderSize+_InsetX;
        int y = _BorderSize;
        
        buttonsVisible.clear();
        for (ButtonPtr button : buttons) {
            // Add space for separator
            // If we're not the first button, add space for the separator at the top of the button
            if (!buttonsVisible.empty()) y += _SeparatorHeight;
            
            // Set button position (after separator)
            button->position({x,y});
            
            // Add space for button
            y += button->size().y;
            
            // Set the expanded hit test size so that the menu doesn't have any dead zones
            if (buttonsVisible.empty()) button->hitTestExpand.t = 1;
            button->hitTestExpand.l = _BorderSize+_InsetX;
            button->hitTestExpand.r = _BorderSize+_InsetX;
            button->hitTestExpand.b = 1;
            button->actionButtons(Event::MouseButtons::Left|Event::MouseButtons::Right);
            
            // Bail if the bottom of the bottom extends beyond our max y
            if (y > ymax) break;
            buttonsVisible.push_back(button);
        }
        
        return true;
    }
    
    bool drawNeeded() const override {
        if (Panel::drawNeeded()) return true;
        for (ButtonPtr button : buttonsVisible) {
            if (button->drawNeeded()) return true;
        }
        return false;
    }
    
    bool draw() override {
        if (!Panel::draw()) return false;
        
        const int width = bounds().size.x;
        
        for (ButtonPtr button : buttonsVisible) {
            button->draw(*this);
            
            // Draw separator
            if (erased()) { // Performance optimization: only draw if the window was erased
                if (button != buttonsVisible.back()) {
                    Window::Attr color = attr(colors.menu);
                    drawLineHoriz({0, button->frame().ymax()+1}, width);
                }
            }
        }
        
        // Draw border
        if (erased()) { // Performance optimization: only draw if the window was erased
            Window::Attr color = attr(colors.menu);
            drawBorder();
            
            // Draw title
            if (!title.empty()) {
                Window::Attr bold = attr(A_BOLD);
                int offX = (width-(int)UTF8::Strlen(title))/2;
                drawText({offX,0}, " %s ", title.c_str());
            }
        }
        
        return true;
    }
    
    bool handleEvent(const Event& ev) override {
        auto& ts = _trackState;
        const auto duration = std::chrono::steady_clock::now()-ts.startEvent.time;
        const Size delta = ev.mouse.point-ts.startEvent.mouse.point;
        
        // Don't allow buttons to receive events until _ActivateDuration
        // has elapsed and the mouse has moved at least 1 px
        if (ev.type==Event::Type::Mouse &&
            duration>=_ActivateDuration &&
            (delta.x || delta.y)) {
            
            ts.active |= true;
        }
        
        if (ts.active) {
            // See if any of the buttons want the event
            for (ButtonPtr button : buttonsVisible) {
                bool handled = button->handleEvent(*this, ev);
                if (handled) {
                    if (dismissAction) dismissAction(*this);
                    return false;
                }
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
            } else if (ev.mouseUp(Event::MouseButtons::Left|Event::MouseButtons::Right)) {
                // Mouse-up occurred and no buttons handled it, so dismiss the menu if either:
                //   1. we haven't entered stay-open mode, and the period to allow stay-open mode has passed, or
                ///  2. we've entered stay-open mode, and the mouse-up was outside of the menu
                if ((!ts.stayOpen && duration>=_StayOpenExpiration) || (ts.stayOpen && !inside)) {
                    if (dismissAction) dismissAction(*this);
                    return false;
                }
                
                ts.stayOpen = true;
            }
        
        } else if (ev.type == Event::Type::KeyEscape) {
            if (dismissAction) dismissAction(*this);
            return false;
        }
        return true;
    }
    
    void track(const Event& ev) {
        _trackState = {};
        _trackState.startEvent = ev;
        Window::track();
    }
    
    const ColorPalette& colors;
    std::string title;
    std::vector<ButtonPtr> buttons;
    std::vector<ButtonPtr> buttonsVisible;
    std::function<void(Menu&)> dismissAction;
    
private:
    static constexpr int _BorderSize      = 1;
    static constexpr int _InsetX          = 1;
    static constexpr int _SeparatorHeight = 1;
    static constexpr int _KeySpacing      = 2;
    static constexpr int _RowHeight       = 2;
    
    static constexpr auto _ActivateDuration = std::chrono::milliseconds(150);
    static constexpr auto _StayOpenExpiration = std::chrono::milliseconds(300);
    
    size_t _buttonCount = 0;
    
    struct {
        Event startEvent;
        bool stayOpen = false;
        bool active = false;
    } _trackState;
};

using MenuPtr = std::shared_ptr<Menu>;

} // namespace UI
