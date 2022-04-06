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
        const size_t labelLen = UTF8::Strlen(_label);
        const size_t keyLen = UTF8::Strlen(_key);
        const int borderSize = (_drawBorder ? 1 : 0);
        const int insetX = _insetX+borderSize;
        const int availWidth = f.size.x-2*insetX;
        const int labelWidth = std::min((int)labelLen, availWidth);
        const int keyWidth = std::max(0, std::min((int)keyLen, availWidth-labelWidth-KeySpacing));
        const int textWidth = labelWidth + (!_key.empty() ? KeySpacing : 0) + keyWidth;
        
        if (_drawBorder) {
            Window::Attr color = win.attr(_enabled ? colors.normal : colors.dimmed);
            win.drawRect(f);
        }
        
        int offY = (f.size.y-1)/2;
        
        // Draw label
        Point plabel;
        Point pkey;
        if (_center) {
            int leftX = insetX + std::max(0, ((f.size.x-2*insetX)-textWidth)/2);
            plabel = f.point + Size{leftX, offY};
            pkey = plabel + Size{labelWidth+KeySpacing, 0};
        
        } else {
            plabel = f.point + Size{insetX, offY};
            pkey = f.point + Size{f.size.x-keyWidth-insetX, offY};
        }
        
        {
            Window::Attr bold;
            Window::Attr color;
            if (_enabled)                 bold = win.attr(A_BOLD);
            if (_highlighted && _enabled) color = win.attr(colors.menu);
            else if (!_enabled)           color = win.attr(colors.dimmed);
            win.drawText(plabel, labelWidth, _label.c_str());
        }
        
        // Draw key
        {
            Window::Attr color = win.attr(colors.dimmed);
            win.drawText(pkey, keyWidth, _key.c_str());
        }
        
        return true;
    }
    
    bool handleEvent(const Window& win, const Event& ev) override {
        if (ev.type == Event::Type::Mouse) {
            if (hitTest(ev.mouse.point)) {
                highlighted(true);
                
                // We allow both mouse-down and mouse-up events to trigger tracking.
                // Mouse-up events are to allow Menu to use this function for context
                // menus: right-mouse-down opens the menu, while the right-mouse-up
                // triggers the Button action via this function.
                if (ev.mouseDown(_actionButtons) || ev.mouseUp(_actionButtons)) {
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
    
    const auto& label() const { return _label; }
    template <typename T> void label(const T& x) { _set(_label, x); }
    
    const auto& key() const { return _key; }
    template <typename T> void key(const T& x) { _set(_key, x); }
    
    const auto& enabled() const { return _enabled; }
    template <typename T> void enabled(const T& x) { _set(_enabled, x); }
    
    const auto& highlighted() const { return _highlighted; }
    template <typename T> void highlighted(const T& x) { _set(_highlighted, x); }
    
    const auto& mouseActive() const { return _mouseActive; }
    template <typename T> void mouseActive(const T& x) { _set(_mouseActive, x); }
    
    const auto& center() const { return _center; }
    template <typename T> void center(const T& x) { _set(_center, x); }
    
    const auto& drawBorder() const { return _drawBorder; }
    template <typename T> void drawBorder(const T& x) { _set(_drawBorder, x); }
    
    const auto& insetX() const { return _insetX; }
    template <typename T> void insetX(const T& x) { _set(_insetX, x); }
    
    const auto& action() const { return _action; }
    template <typename T> void action(const T& x) { _setAlways(_action, x); }
    
    const auto& actionButtons() const { return _actionButtons; }
    template <typename T> void actionButtons(const T& x) { _set(_actionButtons, x); }
    
    const auto& actionTrigger() const { return _actionTrigger; }
    template <typename T> void actionTrigger(const T& x) { _set(_actionTrigger, x); }
    
private:
    static constexpr int KeySpacing = 2;
    
    Event _trigger(const Event& ev) {
        if ((_actionTrigger==ActionTrigger::MouseDown && ev.mouseDown(_actionButtons)) ||
            (_actionTrigger==ActionTrigger::MouseUp && ev.mouseUp(_actionButtons))) {
            
            if (_enabled && hitTest(ev.mouse.point) && _action) {
                _action(*this);
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
            
            if ((_actionTrigger==ActionTrigger::MouseDown && ev.mouseDown(_actionButtons)) ||
                (_actionTrigger==ActionTrigger::MouseUp && ev.mouseUp(_actionButtons))) {
                
                if (_enabled && hitTest(ev.mouse.point) && _action) {
                    _action(*this);
                }
                break;
            }
            
            ev = win.nextEvent();
        }
    }
    
    std::string _label;
    std::string _key;
    bool _enabled = false;
    bool _highlighted = false;
    bool _mouseActive = false;
    bool _center = false;
    bool _drawBorder = false;
    int _insetX = 0;
    std::function<void(Button&)> _action;
    Event::MouseButtons _actionButtons = Event::MouseButtons::Left;
    ActionTrigger _actionTrigger = ActionTrigger::MouseUp;
};

using ButtonPtr = std::shared_ptr<Button>;

} // namespace UI