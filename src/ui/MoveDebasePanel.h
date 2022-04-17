#pragma once
#include <optional>
#include <ctype.h>
#include "Panel.h"
#include "Color.h"
#include "ModalPanel.h"
#include "TextField.h"
#include "LabelTextField.h"
#include "LinkButton.h"

namespace UI {

class MoveDebasePanel : public ModalPanel {
public:
    MoveDebasePanel() {
        message()->centerSingleLine(false);
        
        _okButton->label()->text            ("OK");
        _okButton->drawBorder               (true);
        
        _dismissButton->label()->text        ("Cancel");
        _dismissButton->label()->textAttr    (A_NORMAL);
        _dismissButton->drawBorder           (true);
        _dismissButton->action               (std::bind(&MoveDebasePanel::_actionDismiss, this));
    }
    
    // MARK: - ModalPanel Overrides
    int contentHeight(int width) const override {
        const int purchaseMessageHeight = (_purchaseMessage->visible() ? _purchaseMessage->sizeIntrinsic({width, ConstraintNone}).y : 0);
        return
            (purchaseMessageHeight ? purchaseMessageHeight + _ElementSpacingY : 0) +
            (purchaseMessageHeight ? _PurchaseLinkHeight + _ElementSpacingY + _ElementSpacingY : 0) +
            _FieldHeight + _ElementSpacingY +
            _FieldHeight + _ElementSpacingY +
            _okButton->sizeIntrinsic(ConstraintNoneSize).y;
    }
    
    // MARK: - View Overrides
    
    void layout() override {
        ModalPanel::layout();
        
        const Rect cf = contentFrame();
        int offY = cf.t();
        
        if (purchaseMessageVisible()) {
            _purchaseMessage->sizeToFit({cf.w(), ConstraintNone});
            _purchaseMessage->origin({cf.origin.x, offY});
            offY += _purchaseMessage->frame().h() + _ElementSpacingY;
            
            _purchaseLink->frame({{cf.origin.x, offY}, {cf.w(), _PurchaseLinkHeight}});
            offY += _PurchaseLinkHeight + _ElementSpacingY + _ElementSpacingY;
        }
        
        _email->frame({{cf.origin.x, offY}, {cf.size.x, _FieldHeight}});
        offY += _FieldHeight+_ElementSpacingY;
        _code->frame({{cf.origin.x, offY}, {cf.size.x, _FieldHeight}});
        offY += _FieldHeight+_ElementSpacingY;
        
        const int okButtonWidth = (int)UTF8::Len(_okButton->label()->text()) + _ButtonPaddingX;
        _okButton->frame({{cf.r()-okButtonWidth, offY}, {okButtonWidth, _ButtonHeight}});
        _okButton->enabled(_okButtonEnabled());
        
        const int dismissButtonWidth = (int)UTF8::Len(_dismissButton->label()->text()) + _ButtonPaddingX;
        _dismissButton->frame({{_okButton->frame().l()-_ButtonSpacingX-dismissButtonWidth, offY}, {dismissButtonWidth, _ButtonHeight}});
        _dismissButton->visible((bool)dismissAction());
        _dismissButton->enabled(_dismissButtonEnabled());
    }
    
    bool handleEvent(const Event& ev) override {
        // Dismiss upon mouse-up
        if (ev.type == Event::Type::KeyEscape) {
            _actionDismiss();
        }
        return true;
    }
    
    auto& moveButton() { return _moveButton; }
    auto& dismissButton() { return _dismissButton; }
    
private:
    static constexpr int _ElementSpacingY       = 1;
    static constexpr int _PurchaseLinkHeight    = 1;
    static constexpr int _FieldHeight           = 1;
    static constexpr int _ButtonPaddingX        = 8;
    static constexpr int _ButtonHeight          = 3;
    static constexpr int _ButtonSpacingX        = 1;
    
    ButtonPtr _moveButton       = subviewCreate<Button>();
    ButtonPtr _dismissButton    = subviewCreate<Button>();
};

using MoveDebasePanelPtr = std::shared_ptr<MoveDebasePanel>;

} // namespace UI
