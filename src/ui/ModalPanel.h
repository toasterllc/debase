#pragma once
#include <optional>
#include "Panel.h"
#include "Color.h"
#include "Label.h"

namespace UI {

class ModalPanel : public Panel {
public:
    ModalPanel() {
        _message->centerSingleLine(true);
        _message->wrap(true);
        _message->allowEmptyLines(true);
        
        _title->inhibitErase(true); // Title overlaps border, so don't erase
        _title->prefix(" ");
        _title->suffix(" ");
    }
    
    static constexpr Size BorderSize() { return {5,2}; }
    static constexpr Rect InteriorFrame(Rect bounds) { return Inset(bounds, BorderSize()); }
    
    Rect messageFrame() const {
        Rect interiorFrame = InteriorFrame(bounds());
        interiorFrame.size.y = _message->sizeIntrinsic({interiorFrame.w(), ConstraintNone}).y;
        return interiorFrame;
    }
    
    Rect contentFrame() const {
        const Rect mf = messageFrame();
        return {mf.bl()+Size{0,1}, {mf.w(), contentHeight(mf.w())}};
    }
    
    virtual int contentHeight(int width) const { return 0; }
    virtual bool suppressEvents() const { return _suppressEvents; }
    virtual bool suppressEvents(bool x) { return _set(_suppressEvents, x); }
    
    // MARK: - View Overrides
    Size sizeIntrinsic(Size constraint) override {
        const Rect f = InteriorFrame({{}, constraint});
        const int heightMessage = _message->sizeIntrinsic({f.w(), ConstraintNone}).y;
        const int heightContent = contentHeight(constraint.x);
        const int heightBottomSpacing = (heightContent ? 1 : 0);
        return {
            .x = constraint.x,
            .y = 2*BorderSize().y + heightMessage + heightContent + heightBottomSpacing,
        };
    }
    
    void layout() override {
        const Rect f = frame();
        const Point titlePos = {3,0};
        const int titleWidth = f.size.x-2*titlePos.x;
        _title->frame({titlePos, {titleWidth, 1}});
        _message->frame(messageFrame());
        
//        _title->layout(*this);
//        _message->layout(*this);
    }
    
//    bool drawNeeded() const override {
//        return true;
//    }
    
    void draw() override {
        borderColor(_color);
        _title->textAttr(_color|A_BOLD);
        // Always redraw _title because our border may have clobbered it
        _title->drawNeeded(true);
        
//        if (erased()) {
//            Attr style = attr(_color);
////            drawRect(Inset(bounds(), {2,1}));
//            drawRect();
//        }
//        
//        _title->draw(*this);
//        if (!_title->text().empty()) {
//            // Add spaces around title
//            drawText(_title->frame().tl()-Size{1,0}, " ");
//            drawText(_title->frame().tr()+Size{1,0}, " ");
//        }
        
//        _message->draw(*this);
    }
    
    bool handleEvent(GraphicsState gstate, const Event& ev) override {
        // Intercept and drop events before subviews get a chance
        if (_suppressEvents) return true;
        return Panel::handleEvent(gstate, ev);
    }
    
    bool handleEvent(const Event& ev) override {
        // Dismiss upon mouse-up
        if (ev.mouseUp(Event::MouseButtons::Left|Event::MouseButtons::Right) ||
            ev.type == Event::Type::KeyReturn ||
            ev.type == Event::Type::KeyEscape ) {
            
            if (_dismissAction) {
                _dismissAction(*this);
            }
        }
        return true;
    }
    
//    Rect contentRect() { return ContentRect(size()); }
    
    // MARK: - Accessors
    const auto& color() const { return _color; }
    template <typename T> bool color(const T& x) { return _set(_color, x); }
    
    auto& title() { return _title; }
    auto& message() { return _message; }
    
    const auto& dismissAction() const { return _dismissAction; }
    template <typename T> bool dismissAction(const T& x) { return _setForce(_dismissAction, x); }
    
private:
    static constexpr int _MessageSpacingBottom = 1;
    
    Color _color;
    LabelPtr _title     = subviewCreate<Label>();
    LabelPtr _message   = subviewCreate<Label>();
    bool _suppressEvents    = false;
    std::function<void(ModalPanel&)> _dismissAction;
};

using ModalPanelPtr = std::shared_ptr<ModalPanel>;

} // namespace UI
