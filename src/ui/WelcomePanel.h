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
    WelcomePanel(const ColorPalette& colors) :
    ModalPanel(colors), _trialButton(colors), _registerButton(colors) {
        message().align             (Align::Center);
        
        _trialButton.label          ("Start Free Trial");
        _trialButton.enabled        (true);
        _trialButton.insetX         (1);
        _trialButton.center         (true);
        _trialButton.drawBorder     (true);
        
        _registerButton.label       ("Register");
        _registerButton.enabled     (true);
        _registerButton.insetX      (1);
        _registerButton.center      (true);
        _registerButton.drawBorder  (true);
    }
    
    Size sizeIntrinsic(Size constraint) override {
        Size s = ModalPanel::sizeIntrinsic(constraint);
        s.y += 2*_ButtonHeight+_ContentSpacingTop;
        return s;
    }
    
    bool layoutNeeded() const override { return true; }
    
    bool layout() override {
        if (!ModalPanel::layout()) return false;
        
        const Rect rect = contentRect();
        
        int offY = message().frame().ymax()+1+_ContentSpacingTop;
        _trialButton.frame({{rect.origin.x, offY}, {rect.size.x, _ButtonHeight}});
        _trialButton.layout(*this);
        offY += _ButtonHeight;
        
        _registerButton.frame({{rect.origin.x, offY}, {rect.size.x, _ButtonHeight}});
        _registerButton.layout(*this);
        offY += _ButtonHeight;
        
        return true;
    }
    
    bool drawNeeded() const override {
        if (ModalPanel::drawNeeded()) return true;
        if (_trialButton.drawNeeded()) return true;
        if (_registerButton.drawNeeded()) return true;
        return false;
    }
    
    bool draw() override {
        if (!ModalPanel::draw()) return false;
        _trialButton.draw(*this);
        _registerButton.draw(*this);
        return true;
    }
    
    bool handleEvent(const Event& ev) override {
//        // Let caller handle window resize
//        if (ev.type == Event::Type::WindowResize) return ev;
//        // Let caller handle Ctrl-C/D
//        if (ev.type == Event::Type::KeyCtrlC) return ev;
//        if (ev.type == Event::Type::KeyCtrlD) return ev;
        
        bool handled = false;
        if (!handled) handled |= _trialButton.handleEvent(*this, ev);
        if (!handled) handled |= _registerButton.handleEvent(*this, ev);
        // If we handled an event, we need to draw
        if (handled) ModalPanel::drawNeeded(true);
        return true;
    }
    
    auto& trialButton() { return _trialButton; }
    auto& registerButton() { return _registerButton; }
    
private:
    static constexpr int _ContentSpacingTop  = 1;
    static constexpr int _ButtonHeight      = 3;
    
    Button _trialButton;
    Button _registerButton;
};

using WelcomePanelPtr = std::shared_ptr<WelcomePanel>;

} // namespace UI
