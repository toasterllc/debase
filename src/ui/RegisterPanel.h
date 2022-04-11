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
    RegisterPanel() {
    
        auto requestFocus = [&] (TextField& field) { _fieldRequestFocus(field); };
        auto releaseFocus = [&] (TextField& field, bool done) { _fieldReleaseFocus(field, done); };
        
        _email->label()->text           ("Email: ");
        _email->label()->textAttr       (Colors().menu|A_BOLD);
        _email->spacingX                (3);
        _email->textField()->requestFocus = requestFocus;
        _email->textField()->releaseFocus = releaseFocus;
        _email->textField()->focus(true);
        
        _code->label()->text            (" Code: ");
        _code->label()->textAttr        (Colors().menu|A_BOLD);
        _code->spacingX                 (3);
        _code->textField()->requestFocus = requestFocus;
        _code->textField()->releaseFocus = releaseFocus;
        
        _okButton->label()->text        ("OK");
        _okButton->enabled              (true);
        _okButton->drawBorder           (true);
        
        _cancelButton->label()->text    ("Cancel");
        _cancelButton->enabled          (true);
        _cancelButton->drawBorder       (true);
    }
    
    Size sizeIntrinsic(Size constraint) override {
        Size s = ModalPanel::sizeIntrinsic(constraint);
        s.y += _ContentSpacingTop + _FieldHeight + _FieldSpacingY + _FieldHeight + _FieldSpacingY + _ButtonHeight + _ContentSpacingBottom;
        return s;
    }
    
    void layout() override {
        ModalPanel::layout();
        
        const Rect cf = contentFrame();
        int offY = cf.t();
        
        offY += _ContentSpacingTop;
        
        _email->frame({{cf.origin.x, offY}, {cf.size.x, _FieldHeight}});
        offY += _FieldHeight+_FieldSpacingY;
        _code->frame({{cf.origin.x, offY}, {cf.size.x, _FieldHeight}});
        offY += _FieldHeight+_FieldSpacingY;
        
        _okButton->frame({{cf.r()-_ButtonWidth,offY}, {_ButtonWidth, _ButtonHeight}});
        _cancelButton->frame({{_okButton->frame().l()-_ButtonSpacingX-_ButtonWidth,offY}, {_ButtonWidth, _ButtonHeight}});
    }
    
    auto& okButton() { return _okButton; }
    auto& cancelButton() { return _cancelButton; }
    
private:
    static constexpr int _FieldSpacingY         = 1;
    static constexpr int _FieldHeight           = 1;
    static constexpr int _ButtonWidth           = 10;
    static constexpr int _ButtonHeight          = 3;
    static constexpr int _ButtonSpacingX        = 1;
    static constexpr int _ContentSpacingTop     = 0;
    static constexpr int _ContentSpacingBottom  = 1;
    
    static constexpr int _FieldsExtraHeight = 5;
    static constexpr int _FieldLabelInsetX  = 0;
    static constexpr int _FieldLabelWidth   = 10;
    static constexpr int _FieldValueInsetX  = _FieldLabelWidth;
    
    void _fieldRequestFocus(TextField& field) {
        _email->textField()->focus(false);
        _code->textField()->focus(false);
        field.focus(true);
    }
    
    void _fieldReleaseFocus(TextField& field, bool done) {
        _email->textField()->focus(false);
        _code->textField()->focus(false);
        
        bool fieldsFilled = !_email->textField()->value.empty() && !_code->textField()->value.empty();
        if (done) {
            if (fieldsFilled) {
                beep();
            
            } else {
                if (field.value.empty()) {
                    field.focus(true);
                
                } else {
                    if (_email->textField()->value.empty())     _email->textField()->focus(true);
                    else if (_code->textField()->value.empty()) _code->textField()->focus(true);
                }
            }
        
        } else {
            // Tab behavior
            if (&field == _email->textField().get()) {
                _code->textField()->focus(true);
            } else if (&field == _code->textField().get()) {
                _email->textField()->focus(true);
            }
        }
    }
    
    LabelTextFieldPtr _email    = subviewCreate<LabelTextField>();
    LabelTextFieldPtr _code     = subviewCreate<LabelTextField>();
    ButtonPtr _okButton         = subviewCreate<Button>();
    ButtonPtr _cancelButton     = subviewCreate<Button>();
};

using RegisterPanelPtr = std::shared_ptr<RegisterPanel>;

} // namespace UI
