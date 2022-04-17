#pragma once
#include "ModalPanel.h"

namespace UI {

class UpdateAvailablePanel : public Panel {
public:
    UpdateAvailablePanel(Version version) {
        borderColor(colors().menu);
        
        const std::string msg = "debase version " + std::to_string(version) + " available";
        _message->text(msg);
        _message->align(Align::Center);
        _message->textAttr(colors().dimmed);
        _message->wrap(true);
        
        _downloadButton->label()->text  ("Download");
        _downloadButton->label()->align (Align::Left);
//        _downloadButton->drawBorder     (true);
        _downloadButton->enabled        (true);
        
        _ignoreButton->label()->text    ("Ignore");
        _ignoreButton->label()->align   (Align::Right);
//        _ignoreButton->drawBorder       (true);
        _ignoreButton->enabled          (true);
    }
    
    // MARK: - View Overrides
    Size sizeIntrinsic(Size constraint) override {
        return {22, 6};
    }
    
    void layout() override {
        const Rect b = bounds();
        constexpr int ButtonWidth = 8;
        _message->frame({{1,1}, {b.w()-2, 2}});
        _downloadButton->frame({{2,4}, {ButtonWidth, 1}});
        _ignoreButton->frame({{b.w()-2-ButtonWidth,4}, {ButtonWidth, 1}});
    }
    
//    bool handleEvent(const Event& ev) override {
//        const bool hit = hitTest(ev.mouse.origin);
//        const bool mouseDown = ev.mouseDown(Event::MouseButtons::Left|Event::MouseButtons::Right);
//        const bool mouseUp = ev.mouseUp(Event::MouseButtons::Left|Event::MouseButtons::Right);
//        if (hit && (mouseDown || mouseUp)) return true;
//        return false;
//    }
    
private:
    LabelPtr _message = subviewCreate<Label>();
    ButtonPtr _downloadButton = subviewCreate<Button>();
    ButtonPtr _ignoreButton = subviewCreate<Button>();
};

using UpdateAvailablePanelPtr = std::shared_ptr<UpdateAvailablePanel>;

} // namespace UI
