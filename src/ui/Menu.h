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
    
    Menu() {
        borderColor(colors().menu);
        
        _title->inhibitErase(true);
        _title->align(Align::Center);
        _title->textAttr(colors().menu|WA_BOLD);
        _title->prefix(" ");
        _title->suffix(" ");
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
        _title->frame({{}, {size().x, 1}});
        
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
            const Edges hitTestInset = {
                .l = -(_BorderSize+_InsetX),
                .r = -(_BorderSize+_InsetX),
                .t = (first ? -1 : 0),
                .b = -1,
            };
            button->hitTestInset(hitTestInset);
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
                    Attr color = attr(colors().menu);
                    drawLineHoriz({0, button->frame().b()}, width);
                }
            }
        }
    }
    
    bool handleEvent(const Event& ev) override {
        auto& ts = _trackState;
        const auto duration = std::chrono::steady_clock::now()-ts.startEvent.time;
        const Size delta = ev.mouse.origin-ts.startEvent.mouse.origin;
        
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
            bool inside = hitTest(ev.mouse.origin);
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
    
    void track(const Event& ev, Deadline deadline=Forever) override {
        _trackState = {};
        _trackState.startEvent = ev;
        
        // Disable button interaction at the very beginning, to prevent accidental clicks
        for (UI::ButtonPtr button : _buttons) {
            button->mouseActive(true);
            button->interaction(false);
        }
        
        Panel::track(ev, deadline);
    }
    
    const auto& title() const { return _title; }
    template <typename T> bool title(const T& x) { return _set(_title, x); }
    
    const auto& buttons() const { return _buttons; }
    bool buttons(const std::vector<UI::ButtonPtr>& x) {
        _setForce(_buttons, x);
        
        // Update every button action to invoke dismiss(), and then call the original action
        for (UI::ButtonPtr button : _buttons) {
            auto existing = button->action();
            button->action([=] (Button& button) {
                dismiss();
                if (existing) existing(button);
            });
        }
        
        subviews({_title});
        for (UI::ButtonPtr button : _buttons) {
            subviewAdd(button);
        }
        
        layoutNeeded(true);
        return true;
    }
    
    void dismiss() {
        if (_dismissAction) _dismissAction(*this);
        trackStop();
    }
    
    const auto& dismissAction() const { return _dismissAction; }
    template <typename T> bool dismissAction(const T& x) { return _set(_dismissAction, x); }
    
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
    
    LabelPtr _title = subviewCreate<Label>();
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
