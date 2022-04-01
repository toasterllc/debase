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
            if (enabled)                bold = win.attr(A_BOLD);
            if (_highlight && enabled)  color = win.attr(colors.menu);
            else if (!enabled)          color = win.attr(colors.dimmed);
            win.drawText(plabel, "%s", label.c_str());
        }
        
        // Draw button key
        {
            Window::Attr color = win.attr(colors.dimmed);
            win.drawText(pkey, "%s", key.c_str());
        }
    }
    
    Event handleEvent(const Window& win, const Event& ev) override {
        if (ev.type == Event::Type::Mouse) {
            bool hit = HitTest(frame, ev.mouse.point);
            if (enabled && hit) {
                highlight(true);
                
                if (ev.mouseDown()) {
                    // Track mouse
                    _trackMouse(win, ev);
                    return {};
                }
            } else {
                highlight(false);
            }
        }
        return ev;
    }
    
    Event trigger(const Event& ev, Event::MouseButtons buttons=Event::MouseButtons::Left) {
        if (enabled && HitTest(frame, ev.mouse.point) && ev.mouseUp(buttons)) {
            if (action) action(*this);
            // Consume event
            return {};
        }
        return ev;
    }
    
    bool highlight() { return _highlight; }
    void highlight(bool x) {
        if (_highlight == x) return;
        _highlight = x;
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
    
    void _trackMouse(const Window& win, const Event& mouseDownEvent) {
        Event ev = mouseDownEvent;
        
        for (;;) {
            if (ev.type == Event::Type::Mouse) {
                highlight(HitTest(frame, ev.mouse.point));
            }
            
            draw(win);
            ev = trigger(win.nextEvent());
            if (!ev) break;
        }
    }
    
    bool _highlight = false;
    bool _mouseActive = false;
};

using ButtonPtr = std::shared_ptr<Button>;

} // namespace UI
