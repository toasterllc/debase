#pragma once
#include <algorithm>
#include "Window.h"
#include "UTF8.h"
#include "Control.h"

namespace UI {

class Button : public Control {
public:
//    static auto Make(const ColorPalette& colors) {
//        return std::make_shared<Button>(colors);
//    }
    
    enum ActionTrigger {
        MouseUp,
        MouseDown,
    };
    
    Button(const ColorPalette& colors) : Control(colors) {}
    
    bool draw(const Window& win) override {
        if (!Control::draw(win)) return false;
        
        const Rect f = frame();
        size_t labelLen = UTF8::Strlen(label);
        size_t keyLen = UTF8::Strlen(key);
        
        if (drawBorder) {
            Window::Attr color = win.attr(enabled ? colors.normal : colors.dimmed);
            win.drawRect(f);
        }
        
        int offY = (f.size.y-1)/2;
        
        // Draw button name
        Point plabel;
        Point pkey;
        if (center) {
            int textWidth = (int)labelLen + (int)keyLen + (!key.empty() ? KeySpacing : 0);
            int leftX = insetX + ((f.size.x-2*insetX)-textWidth)/2;
            int rightX = f.size.x-leftX;
            
            plabel = f.point + Size{leftX, offY};
            pkey = f.point + Size{rightX-(int)keyLen, offY};
        
        } else {
            plabel = f.point + Size{insetX, offY};
            pkey = f.point + Size{f.size.x-(int)keyLen-insetX, offY};
        }
        
        {
            Window::Attr bold;
            Window::Attr color;
            if (enabled)                 bold = win.attr(A_BOLD);
            if (_highlighted && enabled) color = win.attr(colors.menu);
            else if (!enabled)           color = win.attr(colors.dimmed);
            win.drawText(plabel, "%s", label.c_str());
        }
        
        // Draw button key
        {
            Window::Attr color = win.attr(colors.dimmed);
            win.drawText(pkey, "%s", key.c_str());
        }
        
        return true;
    }
    
    bool handleEvent(const Window& win, const Event& ev) override {
        if (ev.type == Event::Type::Mouse && enabled) {
            if (hitTest(ev.mouse.point)) {
                highlighted(true);
                
                // We allow both mouse-down and mouse-up events to trigger tracking.
                // Mouse-up events are to allow Menu to use this function for context
                // menus: right-mouse-down opens the menu, while the right-mouse-up
                // triggers the Button action via this function.
                if (ev.mouseDown(actionButtons) || ev.mouseUp(actionButtons)) {
                    // Track mouse
                    _trackMouse(win, ev);
                    return true;
                }
            } else {
                highlighted(false);
            }
        }
        return false;
    }
    
    bool highlighted() { return _highlighted; }
    void highlighted(bool x) {
        if (_highlighted == x) return;
        _highlighted = x;
        drawNeeded(true);
    }
    
    bool mouseActive() { return _mouseActive; }
    void mouseActive(bool x) {
        if (_mouseActive == x) return;
        _mouseActive = x;
        drawNeeded(true);
    }
    
    std::string label;
    std::string key;
    bool enabled = false;
    bool center = false;
    bool drawBorder = false;
    int insetX = 0;
    std::function<void(Button&)> action;
    Event::MouseButtons actionButtons = Event::MouseButtons::Left;
    ActionTrigger actionTrigger = ActionTrigger::MouseUp;
    
private:
    static constexpr int KeySpacing = 2;
    
    Event _trigger(const Event& ev) {
        if ((actionTrigger==ActionTrigger::MouseDown && ev.mouseDown(actionButtons)) ||
            (actionTrigger==ActionTrigger::MouseUp && ev.mouseUp(actionButtons))) {
            
            if (enabled && hitTest(ev.mouse.point) && action) {
                action(*this);
            }
            // Consume event
            return {};
        }
        return ev;
    }
    
    void _trackMouse(const Window& win, const Event& mouseDownEvent) {
        Event ev = mouseDownEvent;
        
        for (;;) {
            if (ev.type == Event::Type::Mouse) {
                highlighted(hitTest(ev.mouse.point));
            }
            
            draw(win);
            
            if ((actionTrigger==ActionTrigger::MouseDown && ev.mouseDown(actionButtons)) ||
                (actionTrigger==ActionTrigger::MouseUp && ev.mouseUp(actionButtons))) {
                
                if (enabled && hitTest(ev.mouse.point) && action) {
                    action(*this);
                }
                break;
            }
            
            ev = win.nextEvent();
        }
    }
    
    bool _highlighted = false;
    bool _mouseActive = false;
};

using ButtonPtr = std::shared_ptr<Button>;

} // namespace UI
