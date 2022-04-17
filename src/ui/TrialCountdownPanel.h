#pragma once
#include "ModalPanel.h"

namespace UI {

class TrialCountdownPanel : public ModalPanel {
public:
    TrialCountdownPanel(std::chrono::seconds remaining) : _remaining(remaining) {
        _registerButton->label()->text  ("Register");
        _registerButton->bordered       (true);
        _registerButton->enabled        (true);
        
        title()->text("debase trial");
        title()->align(Align::Center);
        
        // If the time remaining is > 1 day, add .5 days, causing the RelativeTimeString()
        // result to effectively round to the nearest day (instead of flooring) when the
        // time remaining is in days.
        constexpr auto Day = std::chrono::seconds(86400);
        if (remaining > Day) remaining += Day/2;
        message()->text(Time::RelativeTimeString({.futureSuffix="left"}, remaining));
    }
    
    // MARK: - View Overrides
    void layout() override {
        ModalPanel::layout();
        _registerButton->frame(contentFrame(messageFrame(interiorFrame(bounds()))));
    }
    
    bool handleEvent(const Event& ev) override {
        const bool hit = hitTest(ev.mouse.origin);
        const bool mouseDown = ev.mouseDown(Event::MouseButtons::Left|Event::MouseButtons::Right);
        const bool mouseUp = ev.mouseUp(Event::MouseButtons::Left|Event::MouseButtons::Right);
        if (hit && (mouseDown || mouseUp)) return true;
        return false;
    }
    
    // MARK: - ModalPanel Overrides
    int contentHeight(int width) const override {
        return _ButtonHeight;
    }
    
    auto& registerButton() { return _registerButton; }
    
private:
    static constexpr int _ButtonHeight = 3;
    
    ButtonPtr _registerButton = subviewCreate<Button>();
    std::chrono::seconds _remaining = {};
};

using TrialCountdownPanelPtr = std::shared_ptr<TrialCountdownPanel>;

} // namespace UI
