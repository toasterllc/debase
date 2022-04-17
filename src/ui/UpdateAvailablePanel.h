#pragma once
#include "ModalPanel.h"

namespace UI {

class UpdateAvailablePanel : public Panel {
public:
    UpdateAvailablePanel(Version version) {
//        borderColor(colors().menu);
        
        const std::string msg = "debase v" + std::to_string(version) + " update";
        _message->text(msg);
        _message->prefix(" ");
        _message->suffix(" ");
        _message->align(Align::Center);
        _message->textAttr(colors().menu);
        
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
        return {25, 2};
    }
    
    void layout() override {
        const Rect b = bounds();
        
        constexpr int ButtonWidth = 8;
        _message->frame({{2,0}, {b.w()-2, 1}});
        _downloadButton->frame({{3,1}, {ButtonWidth, 1}});
        _ignoreButton->frame({{b.w()-3-ButtonWidth,1}, {ButtonWidth, 1}});
    }
    
    void draw() override {
        Attr color = attr(colors().menu);
        const Rect b = bounds();
        drawRect();
        
        drawLineHoriz({0,1}, b.w(), ' ');
        drawLineVert({0,1}, 1);
        drawLineVert({b.w()-1,1}, 1);
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
