#pragma once
#include <string>

namespace UI {

class ConflictPanel : public Panel {
public:
    ConflictPanel() {
        borderColor(colors().error);
        
        _title->inhibitErase(true); // Title overlaps border, so don't erase
        _title->textAttr(colors().error|WA_BOLD);
        _title->prefix(" ");
        _title->suffix(" ");
    }
    
    Size sizeIntrinsic(Size constraint) override {
        return {std::min(75,constraint.x), std::min(25,constraint.y)};
    }
    
    using View::layout;
    void layout() override {
        View::layout();
        
        const Rect b = bounds();
        const Point titleOrigin = {_TitleInset,0};
        _title->frame({titleOrigin, {b.w()-2*_TitleInset, 1}});
    }
    
    using View::draw;
    void draw() override {
        View::draw();
        
        // Always redraw _title because our border may have clobbered it
        _title->drawNeeded(true);
    }
    
    void filePath(const std::string& p) {
        _title->text("Conflict: " + p);
    }
    
private:
    static constexpr int _TitleInset = 2;
    LabelPtr _title = subviewCreate<Label>();
};

using ConflictPanelPtr = std::shared_ptr<ConflictPanel>;

} // namespace UI
