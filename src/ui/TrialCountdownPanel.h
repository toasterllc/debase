#pragma once
#include "ModalPanel.h"

namespace UI {

class TrialCountdownPanel : public ModalPanel {
public:
    TrialCountdownPanel(int64_t expiration) : _expiration(expiration) {
        using namespace std::chrono;
//        using days = duration<int64_t, std::ratio<86400>>;
        constexpr auto Day = std::chrono::seconds(86400);
        
        _registerButton->label()->text  ("Register");
        _registerButton->drawBorder     (true);
        
        title()->text("debase trial");
        title()->align(Align::Center);
        
        // If the time remaining is > 1 day, add .5 days, causing the RelativeTimeString()
        // result to effectively round to the nearest day (instead of flooring) when the
        // time remaining is in days.
        auto rem = duration_cast<seconds>(std::chrono::system_clock::from_time_t(_expiration) - std::chrono::system_clock::now());
        if (rem > Day) rem += Day/2;
        message()->text(Time::RelativeTimeString({.futureSuffix="left"}, rem));
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
