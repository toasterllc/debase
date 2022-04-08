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
    
    Size sizeIntrinsic(Size constraint) override {
        // First button sets the width
        const int width = (!_buttons.empty() ? _buttons[0]->size().x : 0) + Padding().x;
        
        // Calculate the height by iterating over every button (until no more fit in `heightMax`, if supplied)
        int height = Padding().y;
        int rem = constraint.y-height;
        bool first = true;
        for (ButtonPtr button : _buttons) {
            const int add = (!first ? _SeparatorHeight : 0) + button->size().y;
            // Bail if the button won't fit in the available height
            if (constraint.y && add>rem) break;
            height += add;
            rem -= add;
            first = false;
        }
        
        return {width, height};
    }
    
    void layout() override {
        const int ymax = size().y-_BorderSize;
        const int x = _BorderSize+_InsetX;
        int y = _BorderSize;
        
        bool first = true;
        bool show = true;
        for (ButtonPtr button : _buttons) {
            // Add space for separator
            // If we're not the first button, add space for the separator at the top of the button
            if (!first) y += _SeparatorHeight;
            
            // Set button position (after separator)
            button->origin({x,y});
            
            // Add space for button
            y += button->size().y;
            
            // Set the expanded hit test size so that the menu doesn't have any dead zones
            const View::HitTestExpand hitTestExpand = {
                .t = (first ? 1 : 0),
                .l = _BorderSize+_InsetX,
                .r = _BorderSize+_InsetX,
                .b = 1,
            };
            button->hitTestExpand(hitTestExpand);
            button->actionButtons(Event::MouseButtons::Left|Event::MouseButtons::Right);
            
            // Start hiding buttons once the bottom of the bottom extends beyond our max y
            show &= (y <= ymax);
            button->visible(show);
            
            first = false;
        }
    }
    
    void draw() override {
        const int width = bounds().size.x;
        
        for (ButtonPtr button : _buttons) {
            // Draw separator
            if (erased()) { // Performance optimization: only draw if the window was erased
                if (button->visible()) {
                    Attr color = attr(Colors().menu);
                    drawLineHoriz({0, button->frame().ymax()+1}, width);
                }
            }
        }
        
        // Draw border
        if (erased()) { // Performance optimization: only draw if the window was erased
            Attr color = attr(Colors().menu);
            drawRect();
            
            // Draw title
            if (!_title.empty()) {
                Attr bold = attr(A_BOLD);
                int offX = (width-(int)UTF8::Len(_title))/2;
                drawText({offX,0}, " %s ", _title.c_str());
            }
        }
    }
    
    bool handleEvent(const Window& win, const Event& ev) override {
        auto& ts = _trackState;
        const auto duration = std::chrono::steady_clock::now()-ts.startEvent.time;
        const Size delta = win.mousePosition(ev)-win.mousePosition(ts.startEvent);
        
        // Don't allow buttons to receive events until _ActivateDuration
        // has elapsed and the mouse has moved at least 1 px
        if (ev.type==Event::Type::Mouse &&
            duration>=_ActivateDuration &&
            (delta.x || delta.y)        &&
            !ts.active                  ) {
            
            ts.active |= true;
            for (UI::ButtonPtr button : _buttons) button->interaction(true);
        }
        
        if (ev.type == Event::Type::Mouse) {
            // Update the mouseActive state for all of our buttons
            bool inside = HitTest(bounds(), win.mousePosition(ev));
            if (ts.active) {
                for (ButtonPtr button : _buttons) {
                    button->mouseActive(inside);
                }
            }
            
            // Handle mouse down
            if (ev.mouseDown() && !inside) {
                dismiss();
                return true;
            
            // Handle mouse up
            } else if (ev.mouseUp(Event::MouseButtons::Left|Event::MouseButtons::Right)) {
                // Mouse-up occurred and no buttons handled it, so dismiss the menu if either:
                //   1. we haven't entered stay-open mode, and the period to allow stay-open mode has passed, or
                ///  2. we've entered stay-open mode, and the mouse-up was outside of the menu
                if ((!ts.stayOpen && duration>=_StayOpenExpiration) || (ts.stayOpen && !inside)) {
                    dismiss();
                    return true;
                }
                
                ts.stayOpen = true;
            }
        
        } else if (ev.type == Event::Type::KeyEscape) {
            dismiss();
            return true;
        }
        
        return false;
    }
    
    void track(const Window& win, const Event& ev) override {
        _trackState = {};
        _trackState.startEvent = ev;
        
        // Disable button interaction at the very beginning, to prevent accidental clicks
        for (UI::ButtonPtr button : _buttons) {
            button->mouseActive(true);
            button->interaction(false);
        }
        
        Panel::track(win, ev);
    }
    
    const auto& title() const { return _title; }
    template <typename T> void title(const T& x) { _set(_title, x); }
    
    const auto& buttons() const { return _buttons; }
    void buttons(const std::vector<UI::ButtonPtr>& x) {
        _set(_buttons, x);
        
        // Update every button action to invoke dismiss(), and then call the original action
        for (UI::ButtonPtr button : _buttons) {
            auto existing = button->action();
            button->action([=] (Button& button) {
                dismiss();
                if (existing) existing(button);
            });
        }
        
        subviews().clear();
        for (UI::ButtonPtr button : _buttons) {
            subviews().push_back(button);
        }
        
        layoutNeeded(true);
    }
    
    void dismiss() {
        if (_dismissAction) _dismissAction(*this);
        trackStop();
    }
    
    const auto& dismissAction() const { return _dismissAction; }
    template <typename T> void dismissAction(const T& x) { _set(_dismissAction, x); }
    
//    const auto& subviews() const { return _subviews; }
//    template <typename T> void subviews(const T& x) { _set(_subviews, x); }
    
private:
    static constexpr int _BorderSize      = 1;
    static constexpr int _InsetX          = 1;
    static constexpr int _SeparatorHeight = 1;
    static constexpr int _KeySpacing      = 2;
    static constexpr int _RowHeight       = 2;
    
    static constexpr auto _ActivateDuration = std::chrono::milliseconds(150);
    static constexpr auto _StayOpenExpiration = std::chrono::milliseconds(300);
    
    std::string _title;
    std::vector<ButtonPtr> _buttons;
    std::function<void(Menu&)> _dismissAction;
    
    struct {
        Event startEvent;
        bool stayOpen = false;
        bool active = false;
    } _trackState;
};

using MenuPtr = std::shared_ptr<Menu>;

} // namespace UI
