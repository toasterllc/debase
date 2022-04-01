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
    
    Button(const ColorPalette& colors) : Control(colors) {}
    
    void draw(const Window& win) override {
        if (!drawNeeded) return;
        Control::draw(win);
        
        size_t labelLen = UTF8::Strlen(label);
        size_t keyLen = UTF8::Strlen(key);
        
        if (drawBorder) {
            Window::Attr color = win.attr(enabled ? colors.normal : colors.dimmed);
            win.drawRect(frame);
        }
        
        int offY = (frame.size.y-1)/2;
        
        // Draw button name
        Point plabel;
        Point pkey;
        if (center) {
            int textWidth = (int)labelLen + (int)keyLen + (!key.empty() ? KeySpacing : 0);
            int leftX = insetX + ((frame.size.x-2*insetX)-textWidth)/2;
            int rightX = frame.size.x-leftX;
            
            plabel = frame.point + Size{leftX, offY};
            pkey = frame.point + Size{rightX-(int)keyLen, offY};
        
        } else {
            plabel = frame.point + Size{insetX, offY};
            pkey = frame.point + Size{frame.size.x-(int)keyLen-insetX, offY};
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
    }
    
    Event handleEvent(const Window& win, const Event& ev, Event::MouseButtons sensitive) {
        if (ev.type == Event::Type::Mouse && enabled) {
            if (hitTest(ev.mouse.point)) {
                highlighted(true);
                
                // We allow both mouse-down and mouse-up events to trigger tracking.
                // Mouse-up events are to allow Menu to use this function for context
                // menus: right-mouse-down opens the menu, while the right-mouse-up
                // triggers the Button action via this function.
                if (ev.mouseDown(sensitive) || ev.mouseUp(sensitive)) {
                    // Track mouse
                    _trackMouse(win, ev, sensitive);
                    return {};
                }
            } else {
                highlighted(false);
            }
        }
        return ev;
    }
    
    Event handleEvent(const Window& win, const Event& ev) override {
        return handleEvent(win, ev, Event::MouseButtons::Left);
    }
    
    bool highlighted() { return _highlighted; }
    void highlighted(bool x) {
        if (_highlighted == x) return;
        _highlighted = x;
        drawNeeded = true;
    }
    
    bool mouseActive() { return _mouseActive; }
    void mouseActive(bool x) {
        if (_mouseActive == x) return;
        _mouseActive = x;
        drawNeeded = true;
    }
    
    std::string label;
    std::string key;
    bool enabled = false;
    bool center = false;
    bool drawBorder = false;
    int insetX = 0;
    std::function<void(Button&)> action;
    
private:
    static constexpr int KeySpacing = 2;
    
    Event _trigger(const Event& ev, Event::MouseButtons sensitive) {
        if (ev.mouseUp(sensitive)) {
            if (enabled && hitTest(ev.mouse.point) && action) {
                action(*this);
            }
            // Consume event
            return {};
        }
        return ev;
    }
    
    void _trackMouse(const Window& win, const Event& mouseDownEvent, Event::MouseButtons sensitive) {
        Event ev = mouseDownEvent;
        
        for (;;) {
            if (ev.type == Event::Type::Mouse) {
                highlighted(hitTest(ev.mouse.point));
            }
            
            draw(win);
            ev = _trigger(ev, sensitive);
            if (!ev) break;
            
            ev = win.nextEvent();
        }
    }
    
    bool _highlighted = false;
    bool _mouseActive = false;
};

using ButtonPtr = std::shared_ptr<Button>;

} // namespace UI
