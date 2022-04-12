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
        _label->textAttr(A_BOLD);
        
        _key->textAttr(Colors().dimmed);
    }
    
    void layout() override {
        const Rect r = Inset(bounds(), {(_drawBorder?1:0), 0});
        _key->sizeToFit();
        
        _label->frame({{r.l(), r.my()}, {r.w()-_key->size().x, 1}});
        _key->origin({r.r()-_key->size().x, r.my()});
        
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
    
    void draw() override {
        // Update our border color for View's drawBorder() pass
        if (_drawBorder) borderColor(_enabled ? Colors().normal : Colors().dimmed);
        
        if (!_labelDefaultAttr) _labelDefaultAttr = _label->textAttr();
        
        // Update label styles
        int attr = *_labelDefaultAttr;
        if (_highlighted && _enabled) attr |= A_BOLD|Colors().menu;
        else if (!_enabled)           attr |= Colors().dimmed;
        _label->textAttr(attr);
        
//        // Draw labels
//        _key->draw(win);
//        _label->draw(win);
    }
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
//    bool draw() override {
//        if (!View::draw(win)) return false;
//        
//        const Rect f = frame();
//        const size_t labelLen = UTF8::Len(_label);
//        const size_t keyLen = UTF8::Len(_key);
//        const int borderSize = (_drawBorder ? 1 : 0);
//        const int insetX = _insetX+borderSize;
//        const int availWidth = f.size.x-2*insetX;
//        const int labelWidth = std::min((int)labelLen, availWidth);
//        const int keyWidth = std::max(0, std::min((int)keyLen, availWidth-labelWidth-KeySpacing));
//        const int textWidth = labelWidth + (!_key->empty() ? KeySpacing : 0) + keyWidth;
//        
//        if (_drawBorder) {
//            Attr color = attr(_enabled ? Colors().normal : Colors().dimmed);
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
//            Attr bold;
//            Attr color;
//            if (_enabled)                 bold = attr(A_BOLD);
//            if (_highlighted && _enabled) color = attr(Colors().menu);
//            else if (!_enabled)           color = attr(Colors().dimmed);
//            drawText(plabel, labelWidth, _label->c_str());
//        }
//        
//        // Draw key
//        {
//            Attr color = attr(Colors().dimmed);
//            drawText(pkey, keyWidth, _key->c_str());
//        }
//        
//        return true;
//    }
    
    bool handleEvent(const Event& ev) override {
        // Only consider mouse events
        if (ev.type != Event::Type::Mouse) return false;
        
        const bool hit = hitTest(ev.mouse.origin);
        const bool mouseDownTriggered = (_actionTrigger==ActionTrigger::MouseDown && ev.mouseDown(_actionButtons));
        const bool mouseUpTriggered = (_actionTrigger==ActionTrigger::MouseUp && ev.mouseUp(_actionButtons));
        highlighted(hit);
        
        // Trigger action
        if ((hit || tracking()) && (mouseDownTriggered || mouseUpTriggered)) {
            
            // Cleanup ourself before calling out
            trackStop();
            
            if (hit && _enabled && _action) {
                _action(*this);
            }
            
            return true;
        }
        
        // We allow both mouse-down and mouse-up events to trigger tracking.
        // Mouse-up events are to allow Menu to use this function for context
        // menus: right-mouse-down opens the menu, while the right-mouse-up
        // triggers the Button action via this function.
        if (hit && (ev.mouseDown(_actionButtons) || ev.mouseUp(_actionButtons))) {
            // Track mouse
            track(ev);
            return true;
        }
        
        return false;
    }
    
    auto& label() { return _label; }
    auto& key() { return _key; }
    
    const auto& enabled() const { return _enabled; }
    template <typename T> bool enabled(const T& x) { return _set(_enabled, x); }
    
    const auto& highlighted() const { return _highlighted; }
    template <typename T> bool highlighted(const T& x) { return _set(_highlighted, x); }
    
    const auto& mouseActive() const { return _mouseActive; }
    template <typename T> bool mouseActive(const T& x) { return _set(_mouseActive, x); }
    
    const auto& drawBorder() const { return _drawBorder; }
    template <typename T> bool drawBorder(const T& x) { return _set(_drawBorder, x); }
    
    const auto& action() const { return _action; }
    template <typename T> bool action(const T& x) { return _setForce(_action, x); }
    
    const auto& actionButtons() const { return _actionButtons; }
    template <typename T> bool actionButtons(const T& x) { return _set(_actionButtons, x); }
    
    const auto& actionTrigger() const { return _actionTrigger; }
    template <typename T> bool actionTrigger(const T& x) { return _set(_actionTrigger, x); }
    
private:
    static constexpr int KeySpacing = 2;
    
    LabelPtr _label = subviewCreate<Label>();
    LabelPtr _key = subviewCreate<Label>();
    
    bool _enabled = false;
    bool _highlighted = false;
    bool _mouseActive = false;
    bool _drawBorder = false;
    std::optional<int> _labelDefaultAttr;
    std::function<void(Button&)> _action;
    Event::MouseButtons _actionButtons = Event::MouseButtons::Left;
    ActionTrigger _actionTrigger = ActionTrigger::MouseUp;
};

using ButtonPtr = std::shared_ptr<Button>;

} // namespace UI
