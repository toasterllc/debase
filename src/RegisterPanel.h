#pragma once
#include <optional>
#include <ctype.h>
#include "Panel.h"
#include "Color.h"
#include "MessagePanel.h"
#include "TextField.h"
#include "MakeShared.h"

namespace UI {

class _RegisterPanel : public _MessagePanel {
public:
    _RegisterPanel(const MessagePanelOptions& opts) : _MessagePanel(_AdjustedOptions(opts)) {
        Size s = size();
        int offY = s.y-_FieldsExtraHeight-1;
        int fieldWidth = s.x-2*_FieldLabelInsetX-_FieldLabelWidth;
        
        _email = TextField::Make({
            .colors = opts.colors,
            .frame = {{_FieldValueInsetX, offY}, {fieldWidth, 1}},
            .wantsFocus = [&] (TextField& field) {
                _fieldWantsFocus(field);
            },
        });
        offY += 2;
        
        _code = TextField::Make({
            .colors = opts.colors,
            .frame = {{_FieldValueInsetX, offY}, {fieldWidth, 1}},
            .wantsFocus = [&] (TextField& field) {
                _fieldWantsFocus(field);
            },
        });
        offY += 2;
        
        _email->focus(true);
    }
    
    void draw() override {
        const MessagePanelOptions& opts = options();
        int offY = size().y-_FieldsExtraHeight-1;
        _MessagePanel::draw();
        
        // Draw email field
        {
            Attr color = attr(opts.color|A_BOLD);
            drawText({_FieldLabelInsetX, offY}, "%s", "Email: ");
        }
        
        _email->draw(*this);
        offY += 2;
        
        // Draw code field
        {
            Attr color = attr(opts.color|A_BOLD);
            drawText({_FieldLabelInsetX, offY}, "%s", "Code: ");
        }
        
        _code->draw(*this);
        offY += 2;
        
//        char buf[128];
//        wgetnstr(*this, buf, sizeof(buf));
//        mvwgetnstr(*this, offY-2, _MessageInsetX, buf, sizeof(buf));
    }
    
    virtual Event handleEvent(const Event& ev) override {
        // Let caller handle window resize
        if (ev.type == Event::Type::WindowResize) return ev;
        // Let caller handle Ctrl-C/D
        if (ev.type == Event::Type::KeyCtrlC) return ev;
        if (ev.type == Event::Type::KeyCtrlD) return ev;
        
        Event e = ev;
        if (e) e = _email->handleEvent(ev);
        if (e) e = _code->handleEvent(ev);
        drawNeeded(true);
        return {};
        
        
//        if (ev.type == Event::Type::Mouse) {
//            
//        } else if (iscntrl((int)ev.type)) {
//            
//        } else if (ev.type == Event::Type::WindowResize) {
//            
//        } else {
////            if (_active) {
////                _activePos = _active->insert(_activePos, (int)ev.type);
////                _activePos++;
////            }
//            
//            drawNeeded(true);
////            if (_active)
//        }
        
//        switch (ev.type) {
//        case Event::Type::Mouse: {
//            if (ev.mouseUp() && !HitTest(frame(), {ev.mouse.x, ev.mouse.y})) {
//                // Dismiss when clicking outside of the panel
//                return false;
//            }
//            return true;
//        }
//        
//        case Event::Type::KeyEscape: {
//            // Dismiss when clicking outside of the panel
//            return false;
//        }
//        
//        default:
//            return true;
//        }
//        
//        return {};
    }
    
private:
    static constexpr int _FieldsExtraHeight = 5;
    static constexpr int _FieldLabelInsetX  = _MessageInsetX;
    static constexpr int _FieldLabelWidth   = 10;
    static constexpr int _FieldValueInsetX  = _FieldLabelInsetX+_FieldLabelWidth;
    
    static MessagePanelOptions _AdjustedOptions(const MessagePanelOptions& x) {
        MessagePanelOptions opts = x;
        opts.extraHeight = _FieldsExtraHeight;
        return opts;
    }
    
    void _fieldWantsFocus(TextField& field) {
        _email->focus(false);
        _code->focus(false);
        field.focus(true);
    }
    
    TextFieldPtr _email;
    TextFieldPtr _code;
};

using RegisterPanel = std::shared_ptr<_RegisterPanel>;

} // namespace UI
