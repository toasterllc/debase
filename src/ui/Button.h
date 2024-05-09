#pragma once
#include <algorithm>
#include <functional>
#include "Window.h"
#include "UTF8.h"
#include "View.h"
#include "Label.h"

namespace UI {

class Button : public View {
public:
    struct ActionTrigger : Bitfield<uint8_t> {
        static constexpr Bit MouseDown  = 1<<0;
        static constexpr Bit MouseUp    = 1<<1;
        using Bitfield::Bitfield;
    };
    
    // MARK: - Creation
    Button() {
        _label->align(Align::Center);
        _label->textAttr(WA_BOLD);
        
        _key->textAttr(colors().dimmed);
        
        _highlightColor = colors().menu;
    }
    
    // MARK: - Accessors
    auto& label() { return _label; }
    auto& key() { return _key; }
    
    const auto& highlighted() const { return _highlighted; }
    template <typename T> bool highlighted(const T& x) { return _set(_highlighted, x); }
    
    const auto& mouseActive() const { return _mouseActive; }
    template <typename T> bool mouseActive(const T& x) { return _set(_mouseActive, x); }
    
    const auto& bordered() const { return _bordered; }
    template <typename T> bool bordered(const T& x) { return _set(_bordered, x); }
    
    const auto& highlightColor() const { return _highlightColor; }
    template <typename T> bool highlightColor(const T& x) { return _set(_highlightColor, x); }
    
    const auto& action() const { return _action; }
    template <typename T> bool action(const T& x) { return _setForce(_action, x); }
    
    const auto& actionButtons() const { return _actionButtons; }
    template <typename T> bool actionButtons(const T& x) { return _set(_actionButtons, x); }
    
    const auto& actionTrigger() const { return _actionTrigger; }
    template <typename T> bool actionTrigger(const T& x) { return _set(_actionTrigger, x); }
    
    // MARK: - View Overrides
    Size sizeIntrinsic(Size constraint) override {
        const int keyWidth = _key->sizeIntrinsic(ConstraintNoneSize).x;
        const int w = (bordered() ? 4 : 0) + _label->sizeIntrinsic(ConstraintNoneSize).x + (keyWidth ? KeySpacingX : 0) + keyWidth;
        const int h = (bordered() ? 3 : 1);
        return {w, h};
    }
    
    void layout() override {
        const Rect r = Inset(bounds(), Size{(bordered() ? 2 : 0), 0});
        _key->sizeToFit(ConstraintNoneSize);
        
        _label->frame({{r.l(), r.my()}, {r.w()-_key->size().x, 1}});
        _key->origin({r.r()-_key->size().x, r.my()});
    }
    
    void draw() override {
        // Update our border color for View's bordered() pass
        if (bordered()) borderColor(enabledWindow() ? _highlightColor : colors().dimmed);
        
        if (!_labelDefaultAttr) _labelDefaultAttr = _label->textAttr();
        
        // Update label styles
        attr_t attr = *_labelDefaultAttr;
        if (_highlighted && enabledWindow()) {
            if (bordered()) borderColor(_highlightColor);
            attr |= WA_BOLD|_highlightColor;
        } else if (!enabledWindow()) {
            if (bordered()) borderColor(colors().dimmed);
            attr |= colors().dimmed;
        } else {
            if (bordered()) borderColor(colors().normal);
        }
        _label->textAttr(attr);
    }
    
    bool handleEvent(const Event& ev) override {
        // Only consider mouse events
        if (ev.type != Event::Type::Mouse) return false;
        
//        if (!ev.mouseDown() && !ev.mouseUp()) return false; // Ignore mouse-moved for debugging
        
        const bool hit = hitTest(ev.mouse.origin);
        highlighted(hit);
        
        if (hit && enabledWindow()) {
            if (ev.mouseDown(_actionButtons)) {
                _actionTriggerState = 0;
                _actionTriggerState |= ActionTrigger::MouseDown;
            } else if (ev.mouseUp(_actionButtons)) {
                _actionTriggerState |= ActionTrigger::MouseUp;
            }
        }
        
//                  ud
//        trigger = 01
//        state   = 11
//        
//        trigger = 10
//        state   = 11
//        
//        trigger = 11
//        state   = 10
        
//        const bool trigger = (_actionTriggerState == _actionTrigger);
        const bool trigger = ((_actionTriggerState & _actionTrigger) == _actionTrigger);
        
        // Cleanup ourself on mouse-up
        if (ev.mouseUp(_actionButtons)) {
            _actionTriggerState = 0;
            trackStop();
        }
        
        // Trigger action
        bool handled = false;
        if (trigger) {
            // Reset _actionTriggerState when we trigger to prevent further triggers
            // until the next mouse-down.
            // This is necessary in the case where _actionTrigger==MouseDown. If we didn't
            // clear _actionTriggerState upon triggering, then we'd trigger again on
            // MouseUp, because _actionTriggerState still had the MouseDown bit set.
            _actionTriggerState = 0;
            if (_action) {
                _action(*this);
            }
            handled = true;
        }
        
        if (hit && ev.mouseDown(_actionButtons) && !screen().eventSince(ev)) {
            // Track mouse
            track();
            handled = true;
        }
        
        return handled;
    }
    
private:
    static constexpr int KeySpacingX = 2;
    
    LabelPtr _label = subviewCreate<Label>();
    LabelPtr _key = subviewCreate<Label>();
    
    bool _highlighted = false;
    bool _mouseActive = false;
    bool _bordered = false;
    Color _highlightColor;
    std::optional<attr_t> _labelDefaultAttr;
    std::function<void(Button&)> _action;
    Event::MouseButtons _actionButtons = Event::MouseButtons::Left;
    ActionTrigger _actionTrigger = ActionTrigger::MouseDown | ActionTrigger::MouseUp;
    ActionTrigger _actionTriggerState = 0;
};

using ButtonPtr = std::shared_ptr<Button>;

} // namespace UI
