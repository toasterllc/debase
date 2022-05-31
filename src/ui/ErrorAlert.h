#pragma once
#include "Alert.h"

namespace UI {

class ErrorAlert : public Alert {
public:
    ErrorAlert() {
        color            (colors().error);
        title()->text    ("Error");
        
        _supportMessage->text("For support, please contact:");
        _supportMessage->wrap(true);
        
        _supportLink->label()->text(ToasterSupportEmail);
        _supportLink->link("mailto:" _ToasterSupportEmail);
        
        showSupportMessage(false);
    }
    
    // MARK: - Alert Overrides
    int contentHeight(int width) const override {
        if (!_showSupportMessage) return 0;
        
        const int supportMessageHeight = _supportMessage->sizeIntrinsic({width, ConstraintNone}).y;
        return
            supportMessageHeight + _ElementSpacingY +
            _SupportLinkHeight;
    }
    
    // MARK: - View Overrides
    void layout() override {
        Alert::layout();
        const Rect cf = contentFrame();
        
        int offY = cf.t();
        
        _supportMessage->sizeToFit({cf.w(), ConstraintNone});
        _supportMessage->origin({cf.origin.x, offY});
        offY += _supportMessage->frame().h() + _ElementSpacingY;
        
        _supportLink->frame({{cf.origin.x, offY}, {cf.w(), _SupportLinkHeight}});
        offY += _SupportLinkHeight;
    }
    
    const auto& showSupportMessage() const { return _showSupportMessage; }
    template <typename T> bool showSupportMessage(const T& x) {
        _setForce(_showSupportMessage, x);
        _supportMessage->visible(_showSupportMessage);
        _supportLink->visible(_showSupportMessage);
        return true;
    }
    
private:
    static constexpr int _ElementSpacingY       = 1;
    static constexpr int _SupportLinkHeight     = 1;
    
    LabelPtr _supportMessage    = subviewCreate<Label>();
    LinkButtonPtr _supportLink  = subviewCreate<LinkButton>();
    bool _showSupportMessage = false;
};

using ErrorAlertPtr = std::shared_ptr<ErrorAlert>;

} // namespace UI
