#pragma once
#include <algorithm>
#include "Window.h"
#include "UTF8.h"
#include "View.h"
#include "Label.h"
#include <os/log.h>

namespace UI {

class Button : public View {
public:
    enum ActionTrigger {
        MouseUp,
        MouseDown,
    };
    
    Button() {
        _label->align(Align::Center);
        _label->attr(A_BOLD);
        _key->align(Align::Right);
    }
    
    void layout(const Window& win) override {
        const Rect f = frame();
        _label->frame({f.origin+Size{0, (f.size.y-1)/2}, {f.size.x, 1}});
        _key->frame({f.origin+Size{0, (f.size.y-1)/2}, {f.size.x, 1}});
        
//        _label->frame(f);
//        _key->frame(f);
//        
//        _label->layout(win);
//        _key->layout(win);
//        
//        
//        const Size textFieldSize = {f.size.x-labelSize.x-_spacingX, 1};
//        _textField.frame({f.origin+Size{labelSize.x+_spacingX, 0}, textFieldSize});
//        
//        _label->layout(win);
//        _textField.layout(win);
    }
    
    void draw(const Window& win) override {
        const Rect f = frame();
        
        // Draw border
        if (_drawBorder) {
            Window::Attr color = win.attr(_enabled ? Colors().normal : Colors().dimmed);
            win.drawRect(f);
        }
        
        // Update label styles
        int attr = 0;
        if (_enabled)                 attr |= A_BOLD;
        if (_highlighted && _enabled) attr |= Colors().menu;
        else if (!_enabled)           attr |= Colors().dimmed;
        _label->attr(attr);
        _key->attr(Colors().dimmed);
        
//        // Draw labels
//        _key->draw(win);
//        _label->draw(win);
    }
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
//    bool draw(const Window& win) override {
//        if (!View::draw(win)) return false;
//        
//        const Rect f = frame();
//        const size_t labelLen = UTF8::Strlen(_label);
//        const size_t keyLen = UTF8::Strlen(_key);
//        const int borderSize = (_drawBorder ? 1 : 0);
//        const int insetX = _insetX+borderSize;
//        const int availWidth = f.size.x-2*insetX;
//        const int labelWidth = std::min((int)labelLen, availWidth);
//        const int keyWidth = std::max(0, std::min((int)keyLen, availWidth-labelWidth-KeySpacing));
//        const int textWidth = labelWidth + (!_key->empty() ? KeySpacing : 0) + keyWidth;
//        
//        if (_drawBorder) {
//            Window::Attr color = win.attr(_enabled ? Colors().normal : Colors().dimmed);
//            win.drawRect(f);
//        }
//        
//        int offY = (f.size.y-1)/2;
//        
//        // Draw label
//        Point plabel;
//        Point pkey;
//        if (_center) {
//            int leftX = insetX + std::max(0, ((f.size.x-2*insetX)-textWidth)/2);
//            plabel = f.origin + Size{leftX, offY};
//            pkey = plabel + Size{labelWidth+KeySpacing, 0};
//        
//        } else {
//            plabel = f.origin + Size{insetX, offY};
//            pkey = f.origin + Size{f.size.x-keyWidth-insetX, offY};
//        }
//        
//        {
//            Window::Attr bold;
//            Window::Attr color;
//            if (_enabled)                 bold = win.attr(A_BOLD);
//            if (_highlighted && _enabled) color = win.attr(Colors().menu);
//            else if (!_enabled)           color = win.attr(Colors().dimmed);
//            win.drawText(plabel, labelWidth, _label->c_str());
//        }
//        
//        // Draw key
//        {
//            Window::Attr color = win.attr(Colors().dimmed);
//            win.drawText(pkey, keyWidth, _key->c_str());
//        }
//        
//        return true;
//    }
    
    bool handleEvent(const Window& win, const Event& ev) override {
        if (ev.type == Event::Type::Mouse) {
            if (hitTest(ev.mouse.point)) {
                highlighted(true);
                
                os_log(OS_LOG_DEFAULT, "BUTTON: ev.mouseDown(_actionButtons)=%d ev.mouseUp(_actionButtons)=%d",
                    ev.mouseDown(_actionButtons), ev.mouseUp(_actionButtons));
                
                if (ev.mouseUp(_actionButtons)) {
                    os_log(OS_LOG_DEFAULT, "BUTTON: MOUSE UP!");
                }
                
                // Trigger action
                if ((_actionTrigger==ActionTrigger::MouseDown && ev.mouseDown(_actionButtons)) ||
                    (_actionTrigger==ActionTrigger::MouseUp && ev.mouseUp(_actionButtons))) {
                    
                    if (_enabled && _action) {
                        trackStop(); // Cleanup ourself before calling out
                        _action(*this);
                    }
                    
                    return true;
                
                // Start tracking
                } else {
                    // We allow both mouse-down and mouse-up events to trigger tracking.
                    // Mouse-up events are to allow Menu to use this function for context
                    // menus: right-mouse-down opens the menu, while the right-mouse-up
                    // triggers the Button action via this function.
                    if (ev.mouseDown(_actionButtons) || ev.mouseUp(_actionButtons)) {
                        // Track mouse
                        track(win, ev);
                        return true;
                    }
                }
            
            } else {
                highlighted(false);
            }
        }
        return false;
    }
    
    auto& label() { return _label; }
    auto& key() { return _key; }
    
    const auto& enabled() const { return _enabled; }
    template <typename T> void enabled(const T& x) { _set(_enabled, x); }
    
    const auto& highlighted() const { return _highlighted; }
    template <typename T> void highlighted(const T& x) { _set(_highlighted, x); }
    
    const auto& mouseActive() const { return _mouseActive; }
    template <typename T> void mouseActive(const T& x) { _set(_mouseActive, x); }
    
    const auto& drawBorder() const { return _drawBorder; }
    template <typename T> void drawBorder(const T& x) { _set(_drawBorder, x); }
    
    const auto& action() const { return _action; }
    template <typename T> void action(const T& x) { _setAlways(_action, x); }
    
    const auto& actionButtons() const { return _actionButtons; }
    template <typename T> void actionButtons(const T& x) { _set(_actionButtons, x); }
    
    const auto& actionTrigger() const { return _actionTrigger; }
    template <typename T> void actionTrigger(const T& x) { _set(_actionTrigger, x); }
    
private:
    static constexpr int KeySpacing = 2;
    
    LabelPtr _label = createSubview<Label>();
    LabelPtr _key = createSubview<Label>();
    
    bool _enabled = false;
    bool _highlighted = false;
    bool _mouseActive = false;
    bool _drawBorder = false;
    std::function<void(Button&)> _action;
    Event::MouseButtons _actionButtons = Event::MouseButtons::Left;
    ActionTrigger _actionTrigger = ActionTrigger::MouseUp;
};

using ButtonPtr = std::shared_ptr<Button>;

} // namespace UI
