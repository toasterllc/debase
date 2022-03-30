#pragma once
#include <optional>
#include <ctype.h>
#include "Panel.h"
#include "Color.h"
#include "Attr.h"
#include "MessagePanel.h"

namespace UI {

class _RegisterPanel : public _MessagePanel {
public:
    _RegisterPanel(const MessagePanelOptions& opts) : _MessagePanel(_AdjustedOptions(opts)) {}
    
    void draw() override {
        const MessagePanelOptions& opts = options();
        int offY = size().y-_FieldsExtraHeight-1;
        _MessagePanel::draw();
        
        {
            {
                UI::Attr attr(shared_from_this(), opts.color|A_BOLD);
                drawText({_FieldLabelInsetX, offY}, "%s", "Email: ");
            }
            
//            {
//                drawText({_FieldValueInsetX, offY}, "%s", "aaaa_aaaa_aaaa_aaaa_aaaa_aaaa_aaaa_aaaa_");
//            }
            
            {
                drawText({_FieldValueInsetX, offY}, "%s", _email.c_str());
            }
            
            offY += 2;
        }
        
        {
            {
                UI::Attr attr(shared_from_this(), opts.color|A_BOLD);
                drawText({_FieldLabelInsetX, offY}, "%s", "Code: ");
            }
            
            {
                drawText({_FieldValueInsetX, offY}, "%s", _code.c_str());
            }
            
            offY += 2;
        }
        
//        char buf[128];
//        wgetnstr(*this, buf, sizeof(buf));
//        mvwgetnstr(*this, offY-2, _MessageInsetX, buf, sizeof(buf));
    }
    
    virtual bool handleEvent(const UI::Event& ev) override {
        if (ev.type == UI::Event::Type::Mouse) {
            
        } else if (iscntrl((int)ev.type)) {
            
        } else if (ev.type == UI::Event::Type::WindowResize) {
            
        } else {
            if (_active) {
                _activePos = _active->insert(_activePos, (int)ev.type);
                _activePos++;
            }
            
            drawNeeded(true);
//            if (_active)
        }
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
        return true;
    }
    
private:
    static constexpr int _FieldsExtraHeight = 5;
    static constexpr int _FieldLabelInsetX = _MessageInsetX;
    static constexpr int _FieldValueInsetX = _FieldLabelInsetX+8;
    
    static MessagePanelOptions _AdjustedOptions(const MessagePanelOptions& x) {
        MessagePanelOptions opts = x;
        opts.extraHeight = _FieldsExtraHeight;
        return opts;
    }
    
    std::string _email;
    std::string _code;
    
    std::string* _active = &_email;
    std::string::iterator _activePos = _email.begin();
};

using RegisterPanel = std::shared_ptr<_RegisterPanel>;

} // namespace UI
