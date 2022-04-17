#pragma once
#include <optional>
#include "Panel.h"
#include "Color.h"
#include "Label.h"

namespace UI {

class TabPanel : public Panel {
public:
    TabPanel() {
        _message->centerSingleLine(true);
        _message->wrap(true);
        _message->allowEmptyLines(true);
        
        _title->inhibitErase(true); // Title overlaps border, so don't erase
        _title->align(Align::Center);
        _title->prefix(" ");
        _title->suffix(" ");
        
        _leftButton->label()->text          ("OK");
        _leftButton->drawBorder             (true);
        
        _rightButton->label()->text         ("Cancel");
        _rightButton->label()->textAttr     (A_NORMAL);
        _rightButton->drawBorder            (true);
//        _rightButton->action              (std::bind(&TabPanel::dismiss, this));
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
        const int h = contentHeight(mf.w());
        if (!h) return {mf.bl(), {mf.w(), 0}};
        return {mf.bl()+Size{0,_SectionSpacingY}, {mf.w(), h}};
    }
    
    virtual int contentHeight(int width) const { return 0; }
//    virtual bool suppressEvents() const { return _suppressEvents; }
//    virtual bool suppressEvents(bool x) { return _set(_suppressEvents, x); }
    
    // MARK: - View Overrides
    Size sizeIntrinsic(Size constraint) override {
        constraint.x = std::min(_width, constraint.x);
        
        const Rect f = InteriorFrame({{}, constraint});
        int offY = BorderSize().y;
        
        const int heightMessage = _message->sizeIntrinsic({f.w(), ConstraintNone}).y;
        offY += heightMessage;
        
        const int heightContent = contentHeight(f.w());
        if (heightContent) offY += _SectionSpacingY + heightContent;
        
        const int heightButton = (leftButtonVisible() || rightButtonVisible() ? _ButtonHeight : 0);
        if (heightButton) offY += _SectionSpacingY + heightButton;
        
        offY += BorderSize().y;
        
        const Size s = {
            .x = constraint.x,
            .y = offY,
        };
        return s;
    }
    
    void layout() override {
        const Rect f = frame();
        const Point titlePos = {3,0};
        const int titleWidth = f.size.x-2*titlePos.x;
        _title->frame({titlePos, {titleWidth, 1}});
        
        _message->frame(messageFrame());
        
        const Rect cf = contentFrame();
        Point off = {cf.r(), cf.b() + _SectionSpacingY};
        
        const int leftButtonWidth = (int)UTF8::Len(_leftButton->label()->text()) + _ButtonPaddingX;
        _leftButton->frame({off+Size{-leftButtonWidth,0}, {leftButtonWidth, _ButtonHeight}});
        _leftButton->visible(leftButtonVisible());
        _leftButton->enabled(leftButtonEnabled());
        if (leftButtonVisible()) off += {-leftButtonWidth-_ButtonSpacingX, 0};
        
        const int rightButtonWidth = (int)UTF8::Len(_rightButton->label()->text()) + _ButtonPaddingX;
        _rightButton->frame({off+Size{-rightButtonWidth,0}, {rightButtonWidth, _ButtonHeight}});
        _rightButton->visible(rightButtonVisible());
        _rightButton->enabled(rightButtonEnabled());
        if (rightButtonVisible()) off += {-rightButtonWidth-_ButtonSpacingX, 0};
        
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
    
//    bool handleEvent(GraphicsState gstate, const Event& ev) override {
//        // Intercept and drop events before subviews get a chance
//        if (_suppressEvents) return true;
//        return Panel::handleEvent(gstate, ev);
//    }
    
    bool handleEvent(const Event& ev) override {
        if (ev.type == Event::Type::KeyEscape){
            if (_escapeTriggersOK) ok();
            else                   dismiss();
        
        } else if (ev.type == Event::Type::KeyReturn) {
            ok();
        }
        return true;
    }
    
//    Rect contentRect() { return ContentRect(size()); }
    
    // MARK: - Accessors
    const auto& width() const { return _width; }
    template <typename T> bool width(const T& x) { return _set(_width, x); }
    
    const auto& color() const { return _color; }
    template <typename T> bool color(const T& x) { return _set(_color, x); }
    
    auto& title() { return _title; }
    auto& message() { return _message; }
    auto& leftButton() { return _leftButton; }
    auto& rightButton() { return _rightButton; }
    
    const auto& escapeTriggersOK() const { return _escapeTriggersOK; }
    template <typename T> bool escapeTriggersOK(const T& x) { return _set(_escapeTriggersOK, x); }
    
    // MARK: - Methods
    virtual void ok() {
        if (leftButtonVisible()) {
            if (leftButtonEnabled() && _leftButton->action()) {
                _leftButton->action()(*_leftButton);
            }
        }
    }
    
    virtual void dismiss() {
        if (rightButtonVisible()) {
            if (rightButtonEnabled() && _rightButton->action()) {
                _rightButton->action()(*_rightButton);
            }
        }
    }
    
    virtual bool leftButtonVisible() const { return (bool)_leftButton->action(); }
    virtual bool leftButtonEnabled() const { return true; }
    
    virtual bool rightButtonVisible() const { return (bool)_rightButton->action(); }
    virtual bool rightButtonEnabled() const { return true; }
    
private:
    static constexpr int _SectionSpacingY       = 1;
    static constexpr int _ButtonHeight          = 3;
    static constexpr int _ButtonPaddingX        = 8;
    static constexpr int _ButtonSpacingX        = 1;
    
    int _width = 0;
    Color _color;
    LabelPtr _title             = subviewCreate<Label>();
    LabelPtr _message           = subviewCreate<Label>();
    ButtonPtr _leftButton         = subviewCreate<Button>();
    ButtonPtr _rightButton    = subviewCreate<Button>();
    bool _escapeTriggersOK      = false;
};

using TabPanelPtr = std::shared_ptr<TabPanel>;

} // namespace UI
