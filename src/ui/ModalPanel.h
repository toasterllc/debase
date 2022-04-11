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
        
        _title->prefix(" ");
        _title->suffix(" ");
    }
    
    static constexpr Size BorderSize() { return {5,2}; }
    static constexpr Rect InteriorFrame(Rect bounds) { return Inset(bounds, BorderSize()); }
    
    Size sizeIntrinsic(Size constraint) override {
        const Rect interiorFrame = InteriorFrame({{}, constraint});
        const int messageHeight = _message->sizeIntrinsic({interiorFrame.w(), 0}).y;
        return {
            .x = constraint.x,
            .y = 2*BorderSize().y + messageHeight,
        };
    }
    
    void layout() override {
        const Rect f = frame();
        const Point titlePos = {3,0};
        _title->textAttr(_color|A_BOLD);
        
        const int titleWidth = f.size.x-2*titlePos.x;
        _title->frame({titlePos, {titleWidth, 1}});
        _message->frame(messageFrame());
        
//        _title->layout(*this);
//        _message->layout(*this);
    }
    
    void draw() override {
        if (erased()) {
            Attr style = attr(_color);
//            drawRect(Inset(bounds(), {2,1}));
            drawRect();
        }
        
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
        if (_dropEvents) return true;
        return Panel::handleEvent(gstate, ev);
    }
    
    bool handleEvent(const Event& ev) override {
        // Dismiss upon mouse-up
        if (ev.mouseUp(Event::MouseButtons::Left|Event::MouseButtons::Right) ||
            ev.type == Event::Type::KeyEscape) {
            
            if (_dismissAction) {
                _dismissAction(*this);
            }
        }
        return true;
    }
    
//    Rect contentRect() { return ContentRect(size()); }
    
    Rect messageFrame() const {
        Rect interiorFrame = InteriorFrame(bounds());
        interiorFrame.size.y = _message->sizeIntrinsic({interiorFrame.w(), 0}).y;
        return interiorFrame;
    }
    
    Rect contentFrame() const {
        const Rect mf = messageFrame();
        return {
            {BorderSize().x, mf.b()+_MessageSpacingBottom},
            {mf.w(), bounds().b()-(mf.b()+_MessageSpacingBottom)}
        };
    }
    
    const auto& color() const { return _color; }
    template <typename T> void color(const T& x) { _set(_color, x); }
    
    auto& title() { return _title; }
    auto& message() { return _message; }
    
    const auto& dropEvents() const { return _dropEvents; }
    template <typename T> void dropEvents(const T& x) { _setForce(_dropEvents, x); }
    
    const auto& dismissAction() const { return _dismissAction; }
    template <typename T> void dismissAction(const T& x) { _setForce(_dismissAction, x); }
    
private:
    static constexpr int _MessageSpacingBottom = 1;
    
    Color _color;
    LabelPtr _title     = subviewCreate<Label>();
    LabelPtr _message   = subviewCreate<Label>();
    bool _dropEvents    = false;
    std::function<void(ModalPanel&)> _dismissAction;
};

using ModalPanelPtr = std::shared_ptr<ModalPanel>;

} // namespace UI
