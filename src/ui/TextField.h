#pragma once
#include <algorithm>
#include "Window.h"
#include "UTF8.h"
#include "View.h"

namespace UI {

class TextField : public View {
public:
    bool layoutNeeded() const override { return true; }
    
    void layout() override {
        _offUpdate();
        
        if (_focusAndEnabled()) {
            const ssize_t cursorOff = UTF8::Len(_left(), _cursor());
            cursorState({.visible=true, .origin={(int)cursorOff, 0}});
        }
    }
    
    void draw() override {
        Attr underline = attr(A_UNDERLINE);
        Attr color;
        if (!_focusAndEnabled()) color = attr(colors().dimmed);
        drawLineHoriz({}, size().x, ' ');
        
        // Print as many runes as will fit our width
        const int width = size().x;
        auto left = _left();
        auto right = UTF8::NextN(left, _value.end(), width);
        
        std::string substr(left, right);
        drawText({}, substr.c_str());
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
    
    const auto& focus() const { return _focus; }
    template <typename T> bool focus(const T& x) { return _set(_focus, x); }
    
    const auto& value() const { return _value; }
    template <typename T> bool value(const T& x) { return _set(_value, x); }
    
    const auto& valueChanged() const { return _valueChanged; }
    template <typename T> bool valueChanged(const T& x) { return _setForce(_valueChanged, x); }
    
    const auto& requestFocus() const { return _requestFocus; }
    template <typename T> bool requestFocus(const T& x) { return _setForce(_requestFocus, x); }
    
    const auto& releaseFocus() const { return _releaseFocus; }
    template <typename T> bool releaseFocus(const T& x) { return _setForce(_releaseFocus, x); }
    
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
                
                if (ev.mouseDown() && hit && !_focus) {
                    if (_requestFocus) _requestFocus(*this);
                }
                
                if ((ev.mouseDown() && hit) || tracking()) {
                    // Update the cursor position to the clicked point
                    int offX = ev.mouse.origin.x;
                    auto offIt = UTF8::NextN(_left(), _value.end(), offX);
                    _offCursor = std::distance(_value.begin(), offIt);
                }
                
                if (ev.mouseDown() && hit && !tracking()) {
                    // Track mouse
                    track(ev);
                    return true;
                
                } else if (ev.mouseUp() && tracking()) {
                    trackStop();
                    return true;
                }
            }
        
        } else if (_focusAndEnabled()) {
            if (ev.type == Event::Type::KeyDelete) {
                auto cursor = _cursor();
                if (cursor == _value.begin()) return true;
                
                auto eraseBegin = UTF8::Prev(cursor, _value.begin());
                size_t eraseSize = std::distance(eraseBegin, cursor);
                _value.erase(eraseBegin, cursor);
                _offCursor -= eraseSize;
                _offUpdate();
                
                if (_valueChanged) _valueChanged(*this);
                return true;
            
            } else if (ev.type == Event::Type::KeyFnDelete) {
                auto cursor = _cursor();
                if (cursor == _value.end()) return true;
                
                auto eraseEnd = UTF8::Next(cursor, _value.end());
                _value.erase(cursor, eraseEnd);
                _offUpdate();
                
                if (_valueChanged) _valueChanged(*this);
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
                if (_releaseFocus) _releaseFocus(*this, false);
                return true;
            
            } else if (ev.type == Event::Type::KeyBackTab) {
                if (_releaseFocus) _releaseFocus(*this, false);
                return true;
            
            } else if (ev.type == Event::Type::KeyReturn) {
                if (_releaseFocus) _releaseFocus(*this, true);
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
                
                if (_valueChanged) _valueChanged(*this);
                return true;
            }
        }
        
        return false;
    }
    
    bool _focusAndEnabled() const {
        return _focus && enabledWindow();
    }
    
    std::string _value;
    std::function<void(TextField&)> _valueChanged;
    std::function<void(TextField&)> _requestFocus;
    std::function<void(TextField&, bool)> _releaseFocus;
    
    ssize_t _offLeft = 0;
    ssize_t _offCursor = 0;
    bool _focus = false;
    CursorState _cursorState;
};

using TextFieldPtr = std::shared_ptr<TextField>;

} // namespace UI
