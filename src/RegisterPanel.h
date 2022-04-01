#pragma once
#include <optional>
#include <ctype.h>
#include "Panel.h"
#include "Color.h"
#include "ModalPanel.h"
#include "TextField.h"
#include "MakeShared.h"
#include <os/log.h>

namespace UI {

class RegisterPanel : public ModalPanel {
public:
    RegisterPanel(const ColorPalette& colors) :
    ModalPanel(colors), _email(colors), _code(colors) {
        extraHeight = _FieldsExtraHeight;
        
        auto requestFocus = [&] (TextField& field) { _fieldRequestFocus(field); };
        auto releaseFocus = [&] (TextField& field, bool done) { _fieldReleaseFocus(field, done); };
        
        _email.requestFocus = requestFocus;
        _email.releaseFocus = releaseFocus;
        
        _code.requestFocus = requestFocus;
        _code.releaseFocus = releaseFocus;
        
        _email.focus(true);
    }
    
    void layout() override {
        ModalPanel::layout();
        
        Size s = size();
        int fieldWidth = s.x-2*_FieldLabelInsetX-_FieldLabelWidth;
        int offY = s.y-_FieldsExtraHeight-1;
        _email.frame = {{_FieldValueInsetX, offY}, {fieldWidth, 1}};
        _email.layout(*this);
        offY += 2;
        _code.frame = {{_FieldValueInsetX, offY}, {fieldWidth, 1}};
        _code.layout(*this);
        offY += 2;
    }
    
    void draw() override {
//        drawNeeded = true;
        if (!drawNeeded) return;
        
        os_log(OS_LOG_DEFAULT, "RegisterPanel::draw()");
        
        ModalPanel::draw();
        
        int offY = size().y-_FieldsExtraHeight-1;
        
        // Draw email field
        {
            Attr style = attr(color|A_BOLD);
            drawText({_FieldLabelInsetX, offY}, "%s", "Email: ");
        }
        
        _email.draw(*this);
        offY += 2;
        
        // Draw code field
        {
            Attr style = attr(color|A_BOLD);
            drawText({_FieldLabelInsetX, offY}, "%s", "Code: ");
        }
        
        _code.draw(*this);
        offY += 2;
    }
    
    bool handleEvent(const Event& ev) override {
//        // Let caller handle window resize
//        if (ev.type == Event::Type::WindowResize) return ev;
//        // Let caller handle Ctrl-C/D
//        if (ev.type == Event::Type::KeyCtrlC) return ev;
//        if (ev.type == Event::Type::KeyCtrlD) return ev;
        
        bool handled = false;
        if (!handled) handled |= _email.handleEvent(*this, ev);
        if (!handled) handled |= _code.handleEvent(*this, ev);
        // If we handled an event, we need to draw
        if (handled) drawNeeded = true;
        return true;
    }
    
private:
    static constexpr int _FieldsExtraHeight = 5;
    static constexpr int _FieldLabelInsetX  = _MessageInsetX;
    static constexpr int _FieldLabelWidth   = 10;
    static constexpr int _FieldValueInsetX  = _FieldLabelInsetX+_FieldLabelWidth;
    
    void _fieldRequestFocus(TextField& field) {
        _email.focus(false);
        _code.focus(false);
        field.focus(true);
    }
    
    void _fieldReleaseFocus(TextField& field, bool done) {
        _email.focus(false);
        _code.focus(false);
        
        bool fieldsFilled = !_email.value.empty() && !_code.value.empty();
        if (done) {
            if (fieldsFilled) {
                beep();
            
            } else {
                if (field.value.empty()) {
                    field.focus(true);
                
                } else {
                    if (_email.value.empty())     _email.focus(true);
                    else if (_code.value.empty()) _code.focus(true);
                }
            }
        
        } else {
            // Tab behavior
            if (&field == &_email) {
                _code.focus(true);
            } else if (&field == &_code) {
                _email.focus(true);
            }
        }
    }
    
    TextField _email;
    TextField _code;
};

using RegisterPanelPtr = std::shared_ptr<RegisterPanel>;

} // namespace UI
