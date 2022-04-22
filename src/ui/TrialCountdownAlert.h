#pragma once
#include "Alert.h"

namespace UI {

class TrialCountdownAlert : public Alert {
public:
    TrialCountdownAlert(std::chrono::seconds remaining) {
        width           (19);
        condensed       (true);
        color           (colors().menu);
        truncateEdges   (UI::Edges{.r=1, .b=1});
        
        _registerButton->label()->text  ("Register");
//        _registerButton->label()->align(Align::Center);
//        _registerButton->bordered(true);
        _registerButton->enabled        (true);
        
        title()->text("debase trial");
        title()->align(Align::Center);
        
        // If the time remaining is > 1 day, add .5 days, causing the RelativeTimeString()
        // result to effectively round to the nearest day (instead of flooring) when the
        // time remaining is in days.
        constexpr auto Day = std::chrono::seconds(86400);
        if (remaining > Day) remaining += Day/2;
        message()->text(Time::RelativeTimeString({.futureSuffix="left"}, remaining));
        message()->textAttr(colors().dimmed);
    }
    
    // MARK: - View Overrides
    void layout() override {
        Alert::layout();
        _registerButton->frame(contentFrame());
    }
    
    bool handleEvent(const Event& ev) override {
        const bool hit = hitTest(ev.mouse.origin);
        const bool mouseDown = ev.mouseDown(Event::MouseButtons::Left|Event::MouseButtons::Right);
        const bool mouseUp = ev.mouseUp(Event::MouseButtons::Left|Event::MouseButtons::Right);
        if (hit && (mouseDown || mouseUp)) return true;
        return false;
    }
    
    // MARK: - Alert Overrides
    int contentHeight(int width) const override {
        return _ButtonHeight;
    }
    
    auto& registerButton() { return _registerButton; }
    
private:
    static constexpr int _ButtonHeight = 1;
    
    ButtonPtr _registerButton = subviewCreate<Button>();
};

using TrialCountdownAlertPtr = std::shared_ptr<TrialCountdownAlert>;

} // namespace UI
