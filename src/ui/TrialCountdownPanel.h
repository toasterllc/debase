#pragma once
#include "ModalPanel.h"

namespace UI {

class TrialCountdownPanel : public ModalPanel {
public:
    TrialCountdownPanel(int64_t expiration) : _expiration(expiration) {
        _registerButton->label()->text      ("Register");
        _registerButton->drawBorder         (true);
        
        title()->text("debase trial");
        title()->align(Align::Center);
        message()->text(Time::RelativeTimeDisplayString({.futureSuffix="left"}, _expiration));
    }
    
    // MARK: - View Overrides
    void layout() override {
        ModalPanel::layout();
        _registerButton->frame(contentFrame());
    }
    
    bool handleEvent(const Event& ev) override {
        const bool hit = hitTest(ev.mouse.origin);
        const bool mouseDown = ev.mouseDown(Event::MouseButtons::Left|Event::MouseButtons::Right);
        const bool mouseUp = ev.mouseUp(Event::MouseButtons::Left|Event::MouseButtons::Right);
        if (hit && (mouseDown || mouseUp)) return true;
        return false;
    }
    
    // MARK: - ModalPanel Overrides
    int contentHeight() const override {
        return _ButtonHeight;
    }
    
    auto& registerButton() { return _registerButton; }
    
private:
    static constexpr int _ButtonHeight = 3;
    
    ButtonPtr _registerButton = subviewCreate<Button>();
    int64_t _expiration = 0;
};

using TrialCountdownPanelPtr = std::shared_ptr<TrialCountdownPanel>;

} // namespace UI
