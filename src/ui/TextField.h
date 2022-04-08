#pragma once
#include <algorithm>
#include "Window.h"
#include "UTF8.h"
#include "View.h"

namespace UI {

class TextField : public View {
public:
    void layout(const Window& win) override {
        _offUpdate();
        
        if (_focus) {
            Point p = win.origin() + origin();
            ssize_t cursorOff = UTF8::Strlen(_left(), _cursor());
            _cursorState = CursorState(true, {p.x+(int)cursorOff, p.y});
        }
    }
    
    void draw(const Window& win) override {
        Window::Attr underline = win.attr(A_UNDERLINE);
        Window::Attr color;
        if (!_focus) color = win.attr(Colors().dimmed);
        win.drawLineHoriz(origin(), size().x, ' ');
        
        // Print as many runes as will fit our width
        const int width = size().x;
        auto left = _left();
        auto right = UTF8::NextN(left, value.end(), width);
        
        std::string substr(left, right);
        win.drawText(origin(), substr.c_str());
    }
    
    bool focus() const { return _focus; }
    void focus(bool x) {
        if (_focus == x) return;
        _focus = x;
        if (_cursorState) _cursorState.restore();
        drawNeeded(true);
    }
    
    std::function<void(TextField&)> requestFocus;
    std::function<void(TextField&, bool)> releaseFocus;
    std::string value;
    
private:
    static constexpr int KeySpacing = 2;
    
    std::string::iterator _left() {
        assert(_offLeft <= value.size());
        return value.begin()+_offLeft;
    }
    
    std::string::iterator _leftMin() { return value.begin(); }
    std::string::iterator _leftMax() {
        return UTF8::NextN(value.end(), value.begin(), -size().x);
    }
    
    std::string::iterator _cursor() {
        assert(_offCursor <= value.size());
        return value.begin()+_offCursor;
    }
    
    std::string::iterator _cursorMin() { return _left(); }
    std::string::iterator _cursorMax() {
        return UTF8::NextN(_cursorMin(), value.end(), size().x);
    }
    
    ssize_t _offLeftMin() { return std::distance(value.begin(), _leftMin()); }
    ssize_t _offLeftMax() { return std::distance(value.begin(), _leftMax()); }
    ssize_t _offCursorMin() { return std::distance(value.begin(), _cursorMin()); }
    ssize_t _offCursorMax() { return std::distance(value.begin(), _cursorMax()); }
    
    void _offUpdate() {
        _offLeft = std::clamp(_offLeft, _offLeftMin(), _offLeftMax());
        _offCursor = std::clamp(_offCursor, _offCursorMin(), _offCursorMax());
    }
    
    bool _handleEvent(const Window& win, const Event& ev) {
        if (ev.type == Event::Type::Mouse) {
            if (ev.mouseDown() && hitTest(win.mousePosition(ev))) {
                if (!_focus) {
                    requestFocus(*this);
                
                } else {
                    // Update the cursor position to the clicked point
                    int offX = win.mousePosition(ev).x-origin().x;
                    auto offIt = UTF8::NextN(_left(), value.end(), offX);
                    _offCursor = std::distance(value.begin(), offIt);
                }
                
                return true;
            }
        
        } else if (_focus) {
            if (ev.type == Event::Type::KeyDelete) {
                auto cursor = _cursor();
                if (cursor == value.begin()) return true;
                
                auto eraseBegin = UTF8::Prev(cursor, value.begin());
                size_t eraseSize = std::distance(eraseBegin, cursor);
                value.erase(eraseBegin, cursor);
                _offCursor -= eraseSize;
                _offUpdate();
                return true;
            
            } else if (ev.type == Event::Type::KeyFnDelete) {
                auto cursor = _cursor();
                if (cursor == value.end()) return true;
                
                auto eraseEnd = UTF8::Next(cursor, value.end());
                value.erase(cursor, eraseEnd);
                _offUpdate();
                return true;
            
            } else if (ev.type == Event::Type::KeyLeft) {
                auto cursor = _cursor();
                if (cursor == value.begin()) return true;
                
                // If the cursor's at the display-beginning, shift view left
                if (cursor == _cursorMin()) {
                    auto left = UTF8::Prev(_left(), value.begin());
                    _offLeft = std::distance(value.begin(), left);
                }
                
                auto it = UTF8::Prev(cursor, value.begin());
                _offCursor = std::distance(value.begin(), it);
                return true;
            
            } else if (ev.type == Event::Type::KeyRight) {
                auto cursor = _cursor();
                if (cursor == value.end()) return true;
                
                // If the cursor's at the display-end, shift view right
                if (cursor == _cursorMax()) {
                    auto left = UTF8::Next(_left(), value.end());
                    _offLeft = std::distance(value.begin(), left);
                }
                
                auto it = UTF8::Next(cursor, value.end());
                _offCursor = std::distance(value.begin(), it);
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
                releaseFocus(*this, false);
                return true;
            
            } else if (ev.type == Event::Type::KeyBackTab) {
                releaseFocus(*this, false);
                return true;
            
            } else if (ev.type == Event::Type::KeyReturn) {
                releaseFocus(*this, true);
                return true;
            
            } else {
                const int c = (int)ev.type;
                // Ignore the character if it's not UTF-8 (>=256), or if it's non-printable ASCII
                if (c>=256 || (c<128 && !isprint(c))) return true;
                
                // If the cursor's at the display-end, shift view right
                if (_cursor() == _cursorMax()) {
                    auto left = UTF8::Next(_left(), value.end());
                    _offLeft = std::distance(value.begin(), left);
                }
                
                value.insert(_cursor(), c);
                _offCursor++;
                _offUpdate();
                return true;
            }
        }
        
        return false;
    }
    
    ssize_t _offLeft = 0;
    ssize_t _offCursor = 0;
    bool _focus = false;
    CursorState _cursorState;
};

using TextFieldPtr = std::shared_ptr<TextField>;

} // namespace UI
