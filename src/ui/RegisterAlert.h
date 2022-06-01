#pragma once
#include <optional>
#include <ctype.h>
#include "Panel.h"
#include "Color.h"
#include "Alert.h"
#include "TextField.h"
#include "LabelTextField.h"
#include "LinkButton.h"

namespace UI {

class RegisterAlert : public Alert {
public:
    RegisterAlert() {
        message()->centerSingleLine(false);
        
        _buyMessage->text("To buy a license, please visit:");
        _buyMessage->wrap(true);
        
        _buyLink->label()->text(ToasterDisplayURL);
        _buyLink->link(DebaseBuyURL);
        
        auto valueChangedAction = [&] (TextField& field) { _fieldValueChanged(field); };
        auto focusAction        = [&] (TextField& field) { _fieldFocus(field); };
        auto unfocusAction      = [&] (TextField& field, TextField::UnfocusReason reason) { _fieldUnfocus(field, reason); };
        
        _email->label()->text                   ("       Email: ");
        _email->label()->textAttr               (colors().menu|WA_BOLD);
        _email->spacingX                        (3);
        _email->textField()->valueChangedAction (valueChangedAction);
        _email->textField()->focusAction        (focusAction);
        _email->textField()->unfocusAction      (unfocusAction);
        _email->textField()->focused            (true);
        
        _code->label()->text                    ("License Code: ");
        _code->label()->textAttr                (colors().menu|WA_BOLD);
        _code->spacingX                         (3);
        _code->textField()->valueChangedAction  (valueChangedAction);
        _code->textField()->focusAction         (focusAction);
        _code->textField()->unfocusAction       (unfocusAction);
    }
    
    // MARK: - Alert Overrides
    int contentHeight(int width) const override {
        const int buyMessageHeight = (_buyMessage->visible() ? _buyMessage->sizeIntrinsic({width, ConstraintNone}).y : 0);
        return
            (buyMessageHeight ? buyMessageHeight + _ElementSpacingY : 0) +
            (buyMessageHeight ? _BuyLinkHeight + _ElementSpacingY + _ElementSpacingY : 0) +
            _FieldHeight + _ElementSpacingY +
            _FieldHeight;
    }
    
//    bool suppressEvents(bool x) override {
//        if (Alert::suppressEvents(x)) {
//            focus(!Alert::suppressEvents() ? _email : nullptr);
//            return true;
//        }
//        return false;
//    }
    
    LabelTextFieldPtr focus() const {
        if (_email->textField()->focused()) return _email;
        else if (_code->textField()->focused()) return _code;
        return nullptr;
    }
    
    bool focus(LabelTextFieldPtr field) {
        _email->textField()->focused(false);
        _code->textField()->focused(false);
        if (field) field->textField()->focused(true);
        return true;
    }
    
    // MARK: - Alert Overrides
//    bool suppressEvents(bool x) override {
//        if (!Alert::suppressEvents(x)) return false;
//        if (Alert::suppressEvents()) {
//            // Save focused field
//            _focusPrev = focus();
//            focus(nullptr);
//        } else {
//            // Restore focused field
//            focus(_focusPrev);
//            _focusPrev = nullptr;
//        }
//        return true;
//    }
    
    // MARK: - View Overrides
    void layout() override {
        Alert::layout();
        const Rect cf = contentFrame();
        
        int offY = cf.t();
        
        if (buyMessageVisible()) {
            _buyMessage->sizeToFit({cf.w(), ConstraintNone});
            _buyMessage->origin({cf.origin.x, offY});
            offY += _buyMessage->frame().h() + _ElementSpacingY;
            
            _buyLink->frame({{cf.origin.x, offY}, {cf.w(), _BuyLinkHeight}});
            offY += _BuyLinkHeight + _ElementSpacingY + _ElementSpacingY;
        }
        
        _email->frame({{cf.origin.x, offY}, {cf.size.x, _FieldHeight}});
        offY += _FieldHeight+_ElementSpacingY;
        _code->frame({{cf.origin.x, offY}, {cf.size.x, _FieldHeight}});
        offY += _FieldHeight+_ElementSpacingY;
    }
    
//    bool handleEvent(const Event& ev) override {
//        // Dismiss upon mouse-up
//        if (ev.type == Event::Type::KeyEscape) {
//            dismiss();
//        }
//        return true;
//    }
    
    auto& email() { return _email->textField(); }
    auto& code() { return _code->textField(); }
    
    bool buyMessageVisible() const { return _buyMessage->visible(); }
    void buyMessageVisible(bool x) {
        _buyMessage->visible(x);
        _buyLink->visible(x);
        layoutNeeded(true);
    }
    
private:
    static constexpr int _ElementSpacingY   = 1;
    static constexpr int _BuyLinkHeight     = 1;
    static constexpr int _FieldHeight       = 1;
    
    void _fieldValueChanged(TextField& field) {
        // Update OK button enabled
        layoutNeeded(true);
    }
    
    void _fieldFocus(TextField& field) {
        _email->textField()->focused(false);
        _code->textField()->focused(false);
        field.focused(true);
    }
    
    void _fieldUnfocus(TextField& field, TextField::UnfocusReason reason) {
        switch (reason) {
        case TextField::UnfocusReason::Tab: {
            _email->textField()->focused(false);
            _code->textField()->focused(false);
            
            if (&field == _email->textField().get()) {
                _code->textField()->focused(true);
            } else if (&field == _code->textField().get()) {
                _email->textField()->focused(true);
            }
            break;
        }
        
        case TextField::UnfocusReason::Return: {
            const bool fieldsFilled = !_email->textField()->value().empty() && !_code->textField()->value().empty();
            if (fieldsFilled) {
                if (okButton()->action()) {
                    okButton()->action()(*okButton());
                }
            
            } else {
                _email->textField()->focused(false);
                _code->textField()->focused(false);
                
                if (field.value().empty()) {
                    field.focused(true);
                
                } else {
                    if (_email->textField()->value().empty())     _email->textField()->focused(true);
                    else if (_code->textField()->value().empty()) _code->textField()->focused(true);
                }
            }
            break;
        }
        
        case TextField::UnfocusReason::Escape: {
            dismiss();
            break;
        }}
    }
    
    virtual bool okButtonEnabled() const override {
        return
            !_code->textField()->value().empty()    &&
            !_email->textField()->value().empty()   ;
    }
    
    LabelPtr _buyMessage   = subviewCreate<Label>();
    LinkButtonPtr _buyLink = subviewCreate<LinkButton>();
    
    LabelTextFieldPtr _email    = subviewCreate<LabelTextField>();
    LabelTextFieldPtr _code     = subviewCreate<LabelTextField>();
    
    LabelTextFieldPtr _focusPrev;
};

using RegisterAlertPtr = std::shared_ptr<RegisterAlert>;

} // namespace UI
