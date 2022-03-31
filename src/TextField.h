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
        std::function<void(TextField&)> wantsFocus;
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
        
        win.drawText(_opts.frame.point, "%s", _value.c_str());
        
        if (_focus) {
            Point p = win.frame().point + _opts.frame.point;
            
//            std::string_view value = _value;
//            CursorState a = CursorState::Push(true, {win->frame().point.x, win->frame().point.y});
//            size_t lenBytes = std::distance(_value.begin(), _posCursor);
            size_t lenRunes = UTF8::Strlen(_left(), _cursor());
            
            _cursorState = CursorState(true, {p.x+(int)lenRunes, p.y});
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
                _opts.wantsFocus(*this);
                return {};
            }
        
        } else if (_focus) {
            if (ev.type == UI::Event::Type::KeyDelete) {
                auto cursor = _cursor();
                if (cursor != _value.begin()) {
                    auto eraseBegin = UTF8::Prev(cursor, _value.begin());
                    size_t eraseSize = std::distance(eraseBegin, cursor);
                    _value.erase(eraseBegin, cursor);
                    _offCursor -= eraseSize;
                }
                return {};
            
            } else if (ev.type == UI::Event::Type::KeyFnDelete) {
                auto cursor = _cursor();
                if (cursor != _value.end()) {
                    auto eraseEnd = UTF8::Next(cursor, _value.end());
                    _value.erase(cursor, eraseEnd);
                }
                return {};
            
            } else if (ev.type == UI::Event::Type::KeyLeft) {
                auto cursor = _cursor();
                if (cursor != _value.begin()) {
                    auto it = UTF8::Prev(cursor, _value.begin());
                    _offCursor = std::distance(_value.begin(), it);
                }
                return {};
            
            } else if (ev.type == UI::Event::Type::KeyRight) {
                auto cursor = _cursor();
                if (cursor != _value.end()) {
                    auto it = UTF8::Next(cursor, _value.end());
                    _offCursor = std::distance(_value.begin(), it);
                }
                return {};
            
            } else {
    //            if (!iscntrl((int)ev.type)) {
                _value.insert(_cursor(), (int)ev.type);
                _offCursor++;
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
