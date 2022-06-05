#pragma once
#include <algorithm>
#include "Window.h"
#include "UTF8.h"
#include "View.h"

namespace UI {

class TextField : public View {
public:
    enum class UnfocusReason {
        Tab,
        Return,
        Escape,
    };
    
    bool layoutNeeded() const override { return true; }
    
    void layout() override {
        _offUpdate();
        
        if (_focusedAndEnabled()) {
            const int alignOff = _alignOff();
            const size_t cursorOff = UTF8::Len(_left(), _cursor());
            cursorState({.visible=true, .origin={alignOff+(int)cursorOff, 0}});
        }
    }
    
    void draw() override {
        const attr_t styleAttr = _attr();
        const Attr style = attr(styleAttr);
        if (styleAttr & WA_UNDERLINE) {
            drawLineHoriz({}, size().x, ' ');
        }
        
        // Print as many codepoints as will fit our width
        const int width = size().x;
        auto left = _left();
        auto right = UTF8::NextN(left, _value.end(), width);
        
        std::string substr(left, right);
        drawText({_alignOff(), 0}, substr.c_str());
    }
    
    bool handleEvent(const Event& ev) override {
        bool handled = _handleEvent(ev);
        if (!handled) return false;
        drawNeeded(true);
        return true;
    }
    
//    bool focus() const { return _focus; }
//    bool focus(bool x) {
//        if (!_set(_focus, x)) return false;
//        if (_cursorState) _cursorState.restore();
//        return true;
//    }
    
    const bool focused() const { return _focused; }
    bool focused(bool x, Align align=Align::Left) {
        const bool set = _set(_focused, x);
        if (set) eraseNeeded(true);
        
        switch (align) {
        case Align::Left:
            _offLeft = std::numeric_limits<ssize_t>::min();
            _offCursor = std::numeric_limits<ssize_t>::min();
            break;
        case Align::Right:
            _offLeft = std::numeric_limits<ssize_t>::max();
            _offCursor = std::numeric_limits<ssize_t>::max();
            break;
        default:
            abort();
        }
        _offUpdate();
        
        return set;
    }
    
    const auto& value() const { return _value; }
    template <typename T> bool value(const T& x) { return _set(_value, x); }
    
    const auto& align() const { return _align; }
    template <typename T> bool align(const T& x) { return _set(_align, x); }
    
    const auto& attrFocused() const { return _attrFocused; }
    template <typename T> bool attrFocused(const T& x) { return _setForce(_attrFocused, x); }
    
    const auto& attrUnfocused() const { return _attrUnfocused; }
    template <typename T> bool attrUnfocused(const T& x) { return _setForce(_attrUnfocused, x); }
    
    const auto& valueChangedAction() const { return _valueChangedAction; }
    template <typename T> bool valueChangedAction(const T& x) { return _setForce(_valueChangedAction, x); }
    
    const auto& focusAction() const { return _focusAction; }
    template <typename T> bool focusAction(const T& x) { return _setForce(_focusAction, x); }
    
    const auto& unfocusAction() const { return _unfocusAction; }
    template <typename T> bool unfocusAction(const T& x) { return _setForce(_unfocusAction, x); }
    
private:
    static constexpr int KeySpacing = 2;
    
    std::string::iterator _left() {
        assert(_offLeft <= (ssize_t)_value.size());
        return _value.begin()+_offLeft;
    }
    
    std::string::iterator _leftMin() { return _value.begin(); }
    std::string::iterator _leftMax() {
        return UTF8::NextN(_value.end(), _value.begin(), -size().x);
    }
    
    std::string::iterator _cursor() {
        assert(_offCursor <= (ssize_t)_value.size());
        return _value.begin()+_offCursor;
    }
    
    std::string::iterator _cursorMin() { return _left(); }
    std::string::iterator _cursorMax() {
        return UTF8::NextN(_cursorMin(), _value.end(), size().x);
    }
    
    ssize_t _offLeftMin() { return std::distance(_value.begin(), _leftMin()); }
    ssize_t _offLeftMax() { return std::distance(_value.begin(), _leftMax()); }
    ssize_t _offCursorMin() { return std::distance(_value.begin(), _cursorMin()); }
    ssize_t _offCursorMax() { return std::distance(_value.begin(), _cursorMax()); }
    
    void _offUpdate() {
        _offLeft = std::clamp(_offLeft, _offLeftMin(), _offLeftMax());
        _offCursor = std::clamp(_offCursor, _offCursorMin(), _offCursorMax());
    }
    
    bool _handleEvent(const Event& ev) {
        if (ev.type == Event::Type::Mouse) {
            if (enabledWindow()) {
                const bool hit = hitTest(ev.mouse.origin);
                
                if (ev.mouseDown() && hit && !_focused) {
                    if (_focusAction) _focusAction(*this);
                }
                
                if ((ev.mouseDown() && hit) || tracking()) {
                    // Update the cursor position to the clicked point
                    int offX = ev.mouse.origin.x-_alignOff();
                    auto offIt = UTF8::NextN(_left(), _value.end(), offX);
                    _offCursor = std::distance(_value.begin(), offIt);
                }
                
                if (ev.mouseDown() && hit && !tracking()) {
                    // Track mouse
                    // Only allow tracking if the callout above (_focusAction()) didn't consume events (ie by
                    // calling track() within its stack frame).
                    // If it did, then it may have consumed a mouse-up event, which will break our tracking.
                    // So in that case, just don't track until the next mouse down.
                    const bool trackAllowed = !screen().eventsSince(ev);
                    if (trackAllowed) track(ev);
                    return true;
                
                } else if (ev.mouseUp() && tracking()) {
                    trackStop();
                    return true;
                }
            }
        
        } else if (_focusedAndEnabled()) {
            if (ev.type == Event::Type::KeyDelete) {
                auto cursor = _cursor();
                if (cursor == _value.begin()) return true;
                
                auto eraseBegin = UTF8::Prev(cursor, _value.begin());
                size_t eraseSize = std::distance(eraseBegin, cursor);
                _value.erase(eraseBegin, cursor);
                _offCursor -= eraseSize;
                _offUpdate();
                
                if (_valueChangedAction) _valueChangedAction(*this);
                return true;
            
            } else if (ev.type == Event::Type::KeyFnDelete) {
                auto cursor = _cursor();
                if (cursor == _value.end()) return true;
                
                auto eraseEnd = UTF8::Next(cursor, _value.end());
                _value.erase(cursor, eraseEnd);
                _offUpdate();
                
                if (_valueChangedAction) _valueChangedAction(*this);
                return true;
            
            } else if (ev.type == Event::Type::KeyLeft) {
                auto cursor = _cursor();
                if (cursor == _value.begin()) return true;
                
                // If the cursor's at the display-beginning, shift view left
                if (cursor == _cursorMin()) {
                    auto left = UTF8::Prev(_left(), _value.begin());
                    _offLeft = std::distance(_value.begin(), left);
                }
                
                auto it = UTF8::Prev(cursor, _value.begin());
                _offCursor = std::distance(_value.begin(), it);
                return true;
            
            } else if (ev.type == Event::Type::KeyRight) {
                auto cursor = _cursor();
                if (cursor == _value.end()) return true;
                
                // If the cursor's at the display-end, shift view right
                if (cursor == _cursorMax()) {
                    auto left = UTF8::Next(_left(), _value.end());
                    _offLeft = std::distance(_value.begin(), left);
                }
                
                auto it = UTF8::Next(cursor, _value.end());
                _offCursor = std::distance(_value.begin(), it);
                return true;
            
            } else if (ev.type == Event::Type::KeyUp) {
                _offLeft = _offLeftMin();
                _offCursor = _offCursorMin();
                _offUpdate();
                return true;
            
            } else if (ev.type == Event::Type::KeyDown) {
                _offLeft = _offLeftMax();
                _offCursor = _offCursorMax();
                _offUpdate();
                return true;
            
            } else if (ev.type == Event::Type::KeyTab) {
                if (_unfocusAction) _unfocusAction(*this, UnfocusReason::Tab);
                return true;
            
            } else if (ev.type == Event::Type::KeyBackTab) {
                if (_unfocusAction) _unfocusAction(*this, UnfocusReason::Tab);
                return true;
            
            } else if (ev.type == Event::Type::KeyReturn) {
                if (_unfocusAction) _unfocusAction(*this, UnfocusReason::Return);
                return true;
            
            } else if (ev.type == Event::Type::KeyEscape) {
                if (_unfocusAction) _unfocusAction(*this, UnfocusReason::Escape);
                return true;
            
            } else {
                const int c = (int)ev.type;
                // Ignore the character if it's not UTF-8 (>=256), or if it's non-printable ASCII
                if (c>=256 || (c<128 && !isprint(c))) return false;
                
                // If the cursor's at the display-end, shift view right
                if (_cursor() == _cursorMax()) {
                    auto left = UTF8::Next(_left(), _value.end());
                    _offLeft = std::distance(_value.begin(), left);
                }
                
                _value.insert(_cursor(), c);
                _offCursor++;
                _offUpdate();
                
                if (_valueChangedAction) _valueChangedAction(*this);
                return true;
            }
        }
        
        return false;
    }
    
    bool _focusedAndEnabled() const {
        return _focused && enabledWindow();
    }
    
    attr_t _attr() const {
        return (_focusedAndEnabled() ? _attrFocused : _attrUnfocused);
    }
    
    int _alignOff() const {
        const size_t len = UTF8::Len(_value);
        const int width = size().x;
        switch (_align) {
        case Align::Left:
            return 0;
        case Align::Center:
            if ((int)len < width) return (width-(int)len)/2;
            return 0;
        case Align::Right:
            // Unsupported
            return 0;
        default:
            abort();
        }
    }
    
    std::string _value;
    Align _align = Align::Left;
    attr_t _attrFocused = WA_UNDERLINE;
    attr_t _attrUnfocused = WA_UNDERLINE | colors().dimmed;
    
    std::function<void(TextField&)> _valueChangedAction;
    std::function<void(TextField&)> _focusAction;
    std::function<void(TextField&, UnfocusReason)> _unfocusAction;
    
    ssize_t _offLeft = 0;
    ssize_t _offCursor = 0;
    bool _focused = false;
    CursorState _cursorState;
};

using TextFieldPtr = std::shared_ptr<TextField>;

} // namespace UI
