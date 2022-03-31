#pragma once
#include <optional>
#include <ctype.h>
#include "Panel.h"
#include "Color.h"
#include "Attr.h"
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
        
        _email = MakeShared<UI::TextField>(UI::TextFieldOptions{
            .colors = opts.colors,
            .frame = {{_FieldValueInsetX, offY}, {fieldWidth, 1}},
        });
        offY += 2;
        
        _code = MakeShared<UI::TextField>(UI::TextFieldOptions{
            .colors = opts.colors,
            .frame = {{_FieldValueInsetX, offY}, {fieldWidth, 1}},
        });
        offY += 2;
        
        _email->active(true);
    }
    
    void draw() override {
        const MessagePanelOptions& opts = options();
        int offY = size().y-_FieldsExtraHeight-1;
        _MessagePanel::draw();
        
        // Draw email field
        {
            UI::Attr attr(shared_from_this(), opts.color|A_BOLD);
            drawText({_FieldLabelInsetX, offY}, "%s", "Email: ");
        }
        
        _email->draw(shared_from_this());
        offY += 2;
        
        // Draw code field
        {
            UI::Attr attr(shared_from_this(), opts.color|A_BOLD);
            drawText({_FieldLabelInsetX, offY}, "%s", "Code: ");
        }
        
        _code->draw(shared_from_this());
        offY += 2;
        
//        char buf[128];
//        wgetnstr(*this, buf, sizeof(buf));
//        mvwgetnstr(*this, offY-2, _MessageInsetX, buf, sizeof(buf));
    }
    
    virtual UI::Event handleEvent(const UI::Event& ev) override {
        // Let caller handle escape key
        if (ev.type == UI::Event::Type::WindowResize) return ev;
        // Let caller handle Ctrl-C/D
        if (ev.type == UI::Event::Type::KeyCtrlC) return ev;
        if (ev.type == UI::Event::Type::KeyCtrlD) return ev;
        
        UI::Event e = ev;
        if (e) e = _email->handleEvent(shared_from_this(), ev);
        if (e) e = _code->handleEvent(shared_from_this(), ev);
        drawNeeded(true);
        return {};
        
        
//        if (ev.type == UI::Event::Type::Mouse) {
//            
//        } else if (iscntrl((int)ev.type)) {
//            
//        } else if (ev.type == UI::Event::Type::WindowResize) {
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
//        case UI::Event::Type::Mouse: {
//            if (ev.mouseUp() && !HitTest(frame(), {ev.mouse.x, ev.mouse.y})) {
//                // Dismiss when clicking outside of the panel
//                return false;
//            }
//            return true;
//        }
//        
//        case UI::Event::Type::KeyEscape: {
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
    
    UI::TextField _email;
    UI::TextField _code;
};

using RegisterPanel = std::shared_ptr<_RegisterPanel>;

} // namespace UI
