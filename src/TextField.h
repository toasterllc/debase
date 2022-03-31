#pragma once
#include <algorithm>
#include "Window.h"
#include "UTF8.h"
#include "CursorState.h"

namespace UI {

class TextField {
public:
    struct Options {
        const ColorPalette& colors;
    //    bool enabled = false;
    //    bool center = false;
    //    bool drawBorder = false;
    //    int insetX = 0;
    //    bool highlight = false;
    //    bool mouseActive = false;
        Rect frame;
        std::function<void(TextField&)> requestFocus;
        std::function<void(TextField&, bool)> releaseFocus;
    //    bool focus = false;
    };
    
    static auto Make(const Options& opts) {
        return std::make_shared<UI::TextField>(opts);
    }
    
    TextField(const Options& opts) : _opts(opts) {}
    
    bool hitTest(const UI::Point& p, UI::Size expand={0,0}) {
        return UI::HitTest(_opts.frame, p, expand);
    }
    
    void draw(const _Window& win) {
        UI::_Window::Attr underline = win.attr(A_UNDERLINE);
        UI::_Window::Attr color;
        if (!_focus) color = win.attr(_opts.colors.dimmed);
        win.drawLineHoriz(_opts.frame.point, _opts.frame.size.x, ' ');
        
        // Print as many runes as will fit our width
        const int width = _opts.frame.size.x;
        auto left = _left();
        auto right = left;
        int len = 0;
        while (len<width && right!=_value.end()) {
            right = UTF8::Next(right, _value.end());
            len++;
        }
        
        std::string substr(left, right);
        win.drawText(_opts.frame.point, "%s", substr.c_str());
        
        if (_focus) {
            Point p = win.frame().point + _opts.frame.point;
            
//            std::string_view value = _value;
//            CursorState a = CursorState::Push(true, {win->frame().point.x, win->frame().point.y});
//            size_t lenBytes = std::distance(_value.begin(), _posCursor);
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
//        win->drawRect(_opts.frame);
    }
    
    UI::Event handleEvent(const UI::Event& ev) {
        if (ev.type == UI::Event::Type::Mouse) {
            if (ev.mouseDown() && HitTest(_opts.frame, ev.mouse.point)) {
                if (!_focus) {
                    _opts.requestFocus(*this);
                
                } else {
                    // Update the cursor position to the clicked point
                    int offX = ev.mouse.point.x-_opts.frame.point.x;
                    auto it = _left();
                    while (offX && it!=_value.end()) {
                        it = UTF8::Next(it, _value.end());
                        offX--;
                    }
                    _offCursor = std::distance(_value.begin(), it);
                }
                
                return {};
            }
        
        } else if (_focus) {
            if (ev.type == UI::Event::Type::KeyDelete) {
                auto cursor = _cursor();
                if (cursor != _value.begin()) {
                    if (_cursorMax() == _value.end()) {
                        auto left = _left();
                        if (left != _value.begin()) {
                            left = UTF8::Prev(left, _value.begin());
                            _offLeft = std::distance(_value.begin(), left);
                        }
                    }
                    
                    auto eraseBegin = UTF8::Prev(cursor, _value.begin());
                    size_t eraseSize = std::distance(eraseBegin, cursor);
                    _value.erase(eraseBegin, cursor);
                    _offCursor -= eraseSize;
                }
                return {};
            
            } else if (ev.type == UI::Event::Type::KeyFnDelete) {
                auto cursor = _cursor();
                if (cursor != _value.end()) {
                    if (_cursorMax() == _value.end()) {
                        auto left = _left();
                        if (left != _value.begin()) {
                            left = UTF8::Prev(left, _value.begin());
                            _offLeft = std::distance(_value.begin(), left);
                        }
                    }
                    
//                    // If the cursor's at the display-end, move it left
//                    auto cursor = _cursor();
//                    if (cursor == _cursorMax()) {
//                        cursor = UTF8::Prev(cursor, _value.end());
//                        _offCursor = std::distance(_value.begin(), cursor);
//                    }
                    
                    auto eraseEnd = UTF8::Next(cursor, _value.end());
                    _value.erase(cursor, eraseEnd);
                    
//                    auto cursor = _cursor();
                }
                return {};
            
            } else if (ev.type == UI::Event::Type::KeyLeft) {
                auto cursor = _cursor();
                if (cursor != _value.begin()) {
                    // If the cursor's at the display-beginning, shift view left
                    if (cursor == _cursorMin()) {
                        auto left = _left();
                        if (left != _value.begin()) {
                            left = UTF8::Prev(left, _value.begin());
                            _offLeft = std::distance(_value.begin(), left);
                        }
                    }
                    
                    auto it = UTF8::Prev(cursor, _value.begin());
                    _offCursor = std::distance(_value.begin(), it);
                }
                return {};
            
            } else if (ev.type == UI::Event::Type::KeyRight) {
                auto cursor = _cursor();
                if (cursor != _value.end()) {
                    // If the cursor's at the display-end, shift view right
                    if (cursor == _cursorMax()) {
                        auto left = _left();
                        if (left != _value.end()) {
                            left = UTF8::Next(left, _value.end());
                            _offLeft = std::distance(_value.begin(), left);
                        }
                    }
                    
                    auto it = UTF8::Next(cursor, _value.end());
                    _offCursor = std::distance(_value.begin(), it);
                }
                return {};
            
            } else if (ev.type == UI::Event::Type::KeyTab) {
                _opts.releaseFocus(*this, false);
                return {};
            
            } else if (ev.type == UI::Event::Type::KeyBackTab) {
                _opts.releaseFocus(*this, false);
                return {};
            
            } else if (ev.type == UI::Event::Type::KeyReturn) {
                _opts.releaseFocus(*this, true);
                return {};
            
            } else {
                // If the cursor's at the display-end, shift view right
//                auto cursor = _cursor();
//                if (cursor == _cursorMax()) {
//                    auto left = _left();
//                    if (left != _value.end()) {
//                        left = UTF8::Next(left, _value.end());
//                        _offLeft = std::distance(_value.begin(), left);
//                    }
//                    
////                    beep();
//                    
////                    _offLeftUpdate();
////                    auto left = _left();
////                    if (left != _value.end()) {
////                        left = UTF8::Next(left, _value.end());
////                        _offLeft = std::distance(_value.begin(), left);
////                    }
//                }
                
                const int width = _opts.frame.size.x;
                if (UTF8::Strlen(_left(), _cursor()) >= width) {
                    auto cursor = _cursor();
                    // If the cursor's at the display-end, shift view right
                    if (cursor == _cursorMax()) {
                        auto left = _left();
                        if (left != _value.end()) {
                            left = UTF8::Next(left, _value.end());
                            _offLeft = std::distance(_value.begin(), left);
                        }
                    }
                }
                
    //            if (!iscntrl((int)ev.type)) {
                if ((int)ev.type < 0x100) {
                    _value.insert(_cursor(), (int)ev.type);
                    _offCursor++;
                }
                
//                _offLeftUpdate();
                return {};
            }
        }
        
        return ev;
    }
    
    const std::string& value() const {
        return _value;
    }
    
    Options& options() { return _opts; }
    const Options& options() const { return _opts; }
    
    bool focus() const { return _focus; }
    void focus(bool x) {
        _focus = x;
        if (_cursorState) _cursorState.restore();
//        if (_focus && !_cursorState) _cursorState.set(true);
//        else if (!_focus && _cursorState) _cursorVis.restore();
    }
    
private:
    static constexpr int KeySpacing = 2;
    
    std::string::iterator _left() { return _value.begin()+_offLeft; }
    std::string::iterator _cursor() { return _value.begin()+_offCursor; }
    
    std::string::iterator _cursorMin() { return _left(); }
    std::string::iterator _cursorMax() {
        const int width = _opts.frame.size.x;
        auto left = _cursorMin();
        auto right = left;
        int len = 0;
        while (len<width && right!=_value.end()) {
            right = UTF8::Next(right, _value.end());
            len++;
        }
        return right;
    }
    
//    std::string::const_iterator _cleft() const { return _value.cbegin()+_offLeft; }
//    std::string::const_iterator _ccursor() const { return _value.cbegin()+_offCursor; }
//    
//    void _offLeftUpdate() {
//        const int width = _opts.frame.size.x;
//        auto right = _cursor();
//        auto left = right;
//        int len = 0;
//        while (len<width && left!=_value.begin()) {
//            left = UTF8::Prev(left, _value.begin());
//            len++;
//        }
//        _offLeft = std::distance(_value.begin(), left);
//    }
//    
//    void _offCursorUpdate() {
//        const int width = _opts.frame.size.x;
//        auto left = _left();
//        auto right = left;
//        int len = 0;
//        while (len<width && right!=_value.end()) {
//            right = UTF8::Next(right, _value.end());
//            len++;
//        }
//        _offCursor = std::distance(_value.begin(), right);
//    }
//    
//    // _cursorDisplayOffset(): returns the offset of the cursor within the
//    // text field, as visible to the user
//    size_t _cursorDisplayOffset() const {
//        return UTF8::Strlen(_cleft(), _ccursor());
//    }
    
    Options _opts;
    std::string _value;
    size_t _offLeft = 0;
    size_t _offCursor = 0;
    bool _focus = false;
    CursorState _cursorState;
//    CursorVisibility _cursorVis;
};

using TextFieldPtr = std::shared_ptr<TextField>;

} // namespace UI
