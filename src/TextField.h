#pragma once
#include <algorithm>
#include "Window.h"
#include "UTF8.h"
#include "CursorState.h"
#include "Control.h"

namespace UI {

class TextField : public Control {
public:
//    struct Options {
//    //    bool enabled = false;
//    //    bool center = false;
//    //    bool drawBorder = false;
//    //    int insetX = 0;
//    //    bool highlight = false;
//    //    bool mouseActive = false;
////        Rect frame;
//        std::function<void(TextField&)> requestFocus;
//        std::function<void(TextField&, bool)> releaseFocus;
//    //    bool focus = false;
//    };
//    
//    static auto Make(const ColorPalette& colors) {
//        return std::make_shared<UI::TextField>(colors);
//    }
    
    TextField(const ColorPalette& colors) : Control(colors) {}
    
    void layout() override {
        // If the cursor offset is past our width, adjust `_leftOff` so that the cursor is within our frame
        size_t cursorOff = UTF8::Strlen(_left(), _cursor());
        if (cursorOff > frame.size.x) {
            beep();
            size_t add = cursorOff-(size_t)frame.size.x;
            auto left = _left();
            while (add && left!=value.end()) {
                left = UTF8::Next(left, value.end());
                add--;
            }
            _offLeft += std::distance(_left(), left);
        
        }
//        else if () {
//            if (_cursor() == value.end()) {
//                auto left = _left();
//                if (left != value.begin()) {
//                    left = UTF8::Prev(left, value.begin());
//                    _offLeft = std::distance(value.begin(), left);
//                }
//            }
//        }
    }
    
    void draw(const _Window& win) override {
        UI::_Window::Attr underline = win.attr(A_UNDERLINE);
        UI::_Window::Attr color;
        if (!_focus) color = win.attr(colors.dimmed);
        win.drawLineHoriz(frame.point, frame.size.x, ' ');
        
        // Print as many runes as will fit our width
        const int width = frame.size.x;
        auto left = _left();
        auto right = left;
        int len = 0;
        while (len<width && right!=value.end()) {
            right = UTF8::Next(right, value.end());
            len++;
        }
        
        std::string substr(left, right);
        win.drawText(frame.point, "%s", substr.c_str());
        
        if (_focus) {
            Point p = win.frame().point + frame.point;
            
//            std::string_view value = value;
//            CursorState a = CursorState::Push(true, {win->frame().point.x, win->frame().point.y});
//            size_t lenBytes = std::distance(value.begin(), _posCursor);
//            size_t lenRunes = UTF8::Strlen(_left(), _cursor());
//            assert(lenRunes <= width); // Programmer error
            size_t cursorOff = UTF8::Strlen(_left(), _cursor());
            _cursorState = CursorState(true, {p.x+(int)cursorOff, p.y});
//            _cursorState = CursorState::Push(true, {win->frame().point.x, win->frame().point.y});
        }
        
//        if (_focus) {
//            curs_set(1);
//            wmove(*win, win->frame().point.y, win->frame().point.x);
//        }
        
//        if (_opts.focus) {
//            if (!_cursorVis) {
//                _cursorVis = CursorVisibility(true);
//            }
//        
//        } else {
//            _cursorVis = std::nullopt;
//        }
//        
//        win->drawRect(frame);
    }
    
    UI::Event handleEvent(const UI::Event& ev) {
        if (ev.type == UI::Event::Type::Mouse) {
            if (ev.mouseDown() && HitTest(frame, ev.mouse.point)) {
                if (!_focus) {
                    requestFocus(*this);
                
                } else {
                    // Update the cursor position to the clicked point
                    int offX = ev.mouse.point.x-frame.point.x;
                    auto it = _left();
                    while (offX && it!=value.end()) {
                        it = UTF8::Next(it, value.end());
                        offX--;
                    }
                    _offCursor = std::distance(value.begin(), it);
                }
                
                return {};
            }
        
        } else if (_focus) {
            if (ev.type == UI::Event::Type::KeyDelete) {
                auto cursor = _cursor();
                if (cursor != value.begin()) {
                    if (_cursorMax() == value.end()) {
                        auto left = _left();
                        if (left != value.begin()) {
                            left = UTF8::Prev(left, value.begin());
                            _offLeft = std::distance(value.begin(), left);
                        }
                    }
                    
                    auto eraseBegin = UTF8::Prev(cursor, value.begin());
                    size_t eraseSize = std::distance(eraseBegin, cursor);
                    value.erase(eraseBegin, cursor);
                    _offCursor -= eraseSize;
                }
                return {};
            
            } else if (ev.type == UI::Event::Type::KeyFnDelete) {
                auto cursor = _cursor();
                if (cursor != value.end()) {
                    if (_cursorMax() == value.end()) {
                        auto left = _left();
                        if (left != value.begin()) {
                            left = UTF8::Prev(left, value.begin());
                            _offLeft = std::distance(value.begin(), left);
                        }
                    }
                    
//                    // If the cursor's at the display-end, move it left
//                    auto cursor = _cursor();
//                    if (cursor == _cursorMax()) {
//                        cursor = UTF8::Prev(cursor, value.end());
//                        _offCursor = std::distance(value.begin(), cursor);
//                    }
                    
                    auto eraseEnd = UTF8::Next(cursor, value.end());
                    value.erase(cursor, eraseEnd);
                    
//                    auto cursor = _cursor();
                }
                return {};
            
            } else if (ev.type == UI::Event::Type::KeyLeft) {
                auto cursor = _cursor();
                if (cursor != value.begin()) {
                    // If the cursor's at the display-beginning, shift view left
                    if (cursor == _cursorMin()) {
                        auto left = _left();
                        if (left != value.begin()) {
                            left = UTF8::Prev(left, value.begin());
                            _offLeft = std::distance(value.begin(), left);
                        }
                    }
                    
                    auto it = UTF8::Prev(cursor, value.begin());
                    _offCursor = std::distance(value.begin(), it);
                }
                return {};
            
            } else if (ev.type == UI::Event::Type::KeyRight) {
                auto cursor = _cursor();
                if (cursor != value.end()) {
                    // If the cursor's at the display-end, shift view right
                    if (cursor == _cursorMax()) {
                        auto left = _left();
                        if (left != value.end()) {
                            left = UTF8::Next(left, value.end());
                            _offLeft = std::distance(value.begin(), left);
                        }
                    }
                    
                    auto it = UTF8::Next(cursor, value.end());
                    _offCursor = std::distance(value.begin(), it);
                }
                return {};
            
            } else if (ev.type == UI::Event::Type::KeyTab) {
                releaseFocus(*this, false);
                return {};
            
            } else if (ev.type == UI::Event::Type::KeyBackTab) {
                releaseFocus(*this, false);
                return {};
            
            } else if (ev.type == UI::Event::Type::KeyReturn) {
                releaseFocus(*this, true);
                return {};
            
            } else {
                // If the cursor's at the display-end, shift view right
//                auto cursor = _cursor();
//                if (cursor == _cursorMax()) {
//                    auto left = _left();
//                    if (left != value.end()) {
//                        left = UTF8::Next(left, value.end());
//                        _offLeft = std::distance(value.begin(), left);
//                    }
//                    
////                    beep();
//                    
////                    _offLeftUpdate();
////                    auto left = _left();
////                    if (left != value.end()) {
////                        left = UTF8::Next(left, value.end());
////                        _offLeft = std::distance(value.begin(), left);
////                    }
//                }
                
                const int width = frame.size.x;
                if (UTF8::Strlen(_left(), _cursor()) >= width) {
                    auto cursor = _cursor();
                    // If the cursor's at the display-end, shift view right
                    if (cursor == _cursorMax()) {
                        auto left = _left();
                        if (left != value.end()) {
                            left = UTF8::Next(left, value.end());
                            _offLeft = std::distance(value.begin(), left);
                        }
                    }
                }
                
                const int c = (int)ev.type;
                // If this is a valid UTF8 value...
                if (c < 256) {
                    // Add the character if it's non-ASCII (therefore UTF8), or if it is ASCII and it's printable
                    if (c>=128 || isprint(c)) {
                        value.insert(_cursor(), c);
                        _offCursor++;
                    }
                }
                
                return {};
            }
        }
        
        return ev;
    }
    
    bool focus() const { return _focus; }
    void focus(bool x) {
        _focus = x;
        if (_cursorState) _cursorState.restore();
//        if (_focus && !_cursorState) _cursorState.set(true);
//        else if (!_focus && _cursorState) _cursorVis.restore();
    }
    
    std::function<void(TextField&)> requestFocus;
    std::function<void(TextField&, bool)> releaseFocus;
    std::string value;
    
private:
    static constexpr int KeySpacing = 2;
    
    std::string::iterator _left() {
        return value.begin()+_offLeft;
    }
    
    std::string::iterator _leftMin() {
        return value.begin();
    }
    
    std::string::iterator _leftMax() {
        const int width = frame.size.x;
        auto it = value.end();
        while (width && it!=value.begin()) {
            it = UTF8::Prev(it, value.begin());
            width--;
        }
        return it;
    }
    
    std::string::iterator _cursor() { return value.begin()+_offCursor; }
    
    std::string::iterator _cursorMin() { return _left(); }
    std::string::iterator _cursorMax() {
        const int width = frame.size.x;
        auto left = _cursorMin();
        auto right = left;
        int len = 0;
        while (len<width && right!=value.end()) {
            right = UTF8::Next(right, value.end());
            len++;
        }
        return right;
    }
    
//    std::string::const_iterator _cleft() const { return value.cbegin()+_offLeft; }
//    std::string::const_iterator _ccursor() const { return value.cbegin()+_offCursor; }
//    
//    void _offLeftUpdate() {
//        const int width = frame.size.x;
//        auto right = _cursor();
//        auto left = right;
//        int len = 0;
//        while (len<width && left!=value.begin()) {
//            left = UTF8::Prev(left, value.begin());
//            len++;
//        }
//        _offLeft = std::distance(value.begin(), left);
//    }
//    
//    void _offCursorUpdate() {
//        const int width = frame.size.x;
//        auto left = _left();
//        auto right = left;
//        int len = 0;
//        while (len<width && right!=value.end()) {
//            right = UTF8::Next(right, value.end());
//            len++;
//        }
//        _offCursor = std::distance(value.begin(), right);
//    }
//    
//    // _cursorDisplayOffset(): returns the offset of the cursor within the
//    // text field, as visible to the user
//    size_t _cursorDisplayOffset() const {
//        return UTF8::Strlen(_cleft(), _ccursor());
//    }
    
    size_t _offLeft = 0;
    size_t _offCursor = 0;
    bool _focus = false;
    CursorState _cursorState;
//    CursorVisibility _cursorVis;
};

using TextFieldPtr = std::shared_ptr<TextField>;

} // namespace UI
