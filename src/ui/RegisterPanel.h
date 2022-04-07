#pragma once
#include <optional>
#include <ctype.h>
#include "Panel.h"
#include "Color.h"
#include "ModalPanel.h"
#include "TextField.h"
#include "LabelTextField.h"

namespace UI {

class RegisterPanel : public ModalPanel {
public:
    RegisterPanel(const ColorPalette& colors) :
    ModalPanel(colors), _email(colors), _code(colors), _okButton(colors), _cancelButton(colors) {
    
        auto requestFocus = [&] (TextField& field) { _fieldRequestFocus(field); };
        auto releaseFocus = [&] (TextField& field, bool done) { _fieldReleaseFocus(field, done); };
        
        _email.label().text ("Email: ");
        _code.label().text  (" Code: ");
        
        _email.spacingX     (3);
        _code.spacingX      (3);
        
        _email.label().attr (colors.menu|A_BOLD);
        _code.label().attr  (colors.menu|A_BOLD);
        
        _email.textField().requestFocus = requestFocus;
        _email.textField().releaseFocus = releaseFocus;
        
        _code.textField().requestFocus = requestFocus;
        _code.textField().releaseFocus = releaseFocus;
        
        _email.textField().focus(true);
    }
    
    Size sizeIntrinsic(Size constraint) override {
        Size s = ModalPanel::sizeIntrinsic(constraint);
        s.y += 2*(_FieldSpacingY+_FieldHeight) + _ContentSpacingBottom;
        return s;
    }
    
    bool layoutNeeded() const override { return true; }
    
    bool layout() override {
        if (!ModalPanel::layout()) return false;
        
        const Rect rect = contentRect();
        int offY = message().frame().ymax()+1;
        
        offY += _FieldSpacingY;
        _email.frame({{rect.origin.x, offY}, {rect.size.x, _FieldHeight}});
        offY += _FieldHeight+_FieldSpacingY;
        _code.frame({{rect.origin.x, offY}, {rect.size.x, _FieldHeight}});
        
        _email.layout(*this);
        _code.layout(*this);
        return true;
    }
    
    bool drawNeeded() const override {
        if (ModalPanel::drawNeeded()) return true;
        if (_email.drawNeeded()) return true;
        if (_code.drawNeeded()) return true;
        return false;
    }
    
    bool draw() override {
        if (!ModalPanel::draw()) return false;
        
        _email.draw(*this);
        _code.draw(*this);
        return true;
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
        if (handled) ModalPanel::drawNeeded(true);
        return true;
    }
    
private:
    static constexpr int _FieldSpacingY         = 1;
    static constexpr int _FieldHeight           = 1;
    static constexpr int _ContentSpacingBottom  = 0;
    
    static constexpr int _FieldsExtraHeight = 5;
    static constexpr int _FieldLabelInsetX  = 0;
    static constexpr int _FieldLabelWidth   = 10;
    static constexpr int _FieldValueInsetX  = _FieldLabelWidth;
    
    void _fieldRequestFocus(TextField& field) {
        _email.textField().focus(false);
        _code.textField().focus(false);
        field.focus(true);
    }
    
    void _fieldReleaseFocus(TextField& field, bool done) {
        _email.textField().focus(false);
        _code.textField().focus(false);
        
        bool fieldsFilled = !_email.textField().value.empty() && !_code.textField().value.empty();
        if (done) {
            if (fieldsFilled) {
                beep();
            
            } else {
                if (field.value.empty()) {
                    field.focus(true);
                
                } else {
                    if (_email.textField().value.empty())     _email.textField().focus(true);
                    else if (_code.textField().value.empty()) _code.textField().focus(true);
                }
            }
        
        } else {
            // Tab behavior
            if (&field == &_email.textField()) {
                _code.textField().focus(true);
            } else if (&field == &_code.textField()) {
                _email.textField().focus(true);
            }
        }
    }
    
    LabelTextField _email;
    LabelTextField _code;
    Button _okButton;
    Button _cancelButton;
};

using RegisterPanelPtr = std::shared_ptr<RegisterPanel>;

} // namespace UI
