#pragma once
#include <optional>
#include <ctype.h>
#include "Panel.h"
#include "Color.h"
#include "Alert.h"
#include "TextField.h"
#include <os/log.h>

namespace UI {

class WelcomePanel : public Alert {
public:
    WelcomePanel() {
        message()->align(Align::Center);
        message()->textAttr(colors().menu|A_BOLD);
        
        borderColor(colors().menu);
        
        _trialButton->label()->text      ("Start Free Trial");
        _trialButton->enabled            (true);
        _trialButton->bordered           (true);
        
        _registerButton->label()->text   ("Register");
        _registerButton->enabled         (true);
        _registerButton->bordered        (true);
    }
    
    // MARK: - Alert Overrides
    int contentHeight(int width) const override {
        return 2*_ButtonHeight+1; // Add 1 line for the bottom spacing, because it looks more balanced
    }
    
//    bool layoutNeeded() const override { return true; }
    
    // MARK: - View Overrides
    void layout() override {
        Alert::layout();
        const Rect cf = contentFrame();
        
        int offY = cf.t();
        _trialButton->frame({{cf.origin.x, offY}, {cf.size.x, _ButtonHeight}});
//        _trialButton->layout(*this);
        offY += _ButtonHeight;
        
        _registerButton->frame({{cf.origin.x, offY}, {cf.size.x, _ButtonHeight}});
//        _registerButton->layout(*this);
        offY += _ButtonHeight;
    }
    
    auto& trialButton() { return _trialButton; }
    auto& registerButton() { return _registerButton; }
    
private:
    static constexpr int _ButtonHeight = 3;
    
    ButtonPtr _trialButton     = subviewCreate<Button>();
    ButtonPtr _registerButton  = subviewCreate<Button>();
};

using WelcomePanelPtr = std::shared_ptr<WelcomePanel>;

} // namespace UI
