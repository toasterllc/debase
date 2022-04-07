#pragma once
#include <optional>
#include <ctype.h>
#include "Panel.h"
#include "Color.h"
#include "ModalPanel.h"
#include "TextField.h"
#include <os/log.h>

namespace UI {

class WelcomePanel : public ModalPanel {
public:
    WelcomePanel() {
        message()->align(Align::Center);
        
        _trialButton->label()->text      ("Start Free Trial");
        _trialButton->enabled            (true);
        _trialButton->drawBorder         (true);
        
        _registerButton->label()->text   ("Register");
        _registerButton->enabled         (true);
        _registerButton->drawBorder      (true);
    }
    
    Size sizeIntrinsic(Size constraint) override {
        Size s = ModalPanel::sizeIntrinsic(constraint);
        s.y += 2*_ButtonHeight+_ContentSpacingTop;
        return s;
    }
    
//    bool layoutNeeded() const override { return true; }
    
    void draw(const Window& win) override {
        drawBorder();
    }
    
    void layout(const Window& win) override {
        const Rect rect = contentRect();
        
        int offY = message()->frame().ymax()+1+_ContentSpacingTop;
        _trialButton->frame({{rect.origin.x, offY}, {rect.size.x, _ButtonHeight}});
//        _trialButton->layout(*this);
        offY += _ButtonHeight;
        
        _registerButton->frame({{rect.origin.x, offY}, {rect.size.x, _ButtonHeight}});
//        _registerButton->layout(*this);
        offY += _ButtonHeight;
    }
    
    auto& trialButton() { return _trialButton; }
    auto& registerButton() { return _registerButton; }
    
private:
    static constexpr int _ContentSpacingTop = 1;
    static constexpr int _ButtonHeight      = 3;
    
    ButtonPtr _trialButton     = createSubview<Button>();
    ButtonPtr _registerButton  = createSubview<Button>();
};

using WelcomePanelPtr = std::shared_ptr<WelcomePanel>;

} // namespace UI
