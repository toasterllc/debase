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
        
        _okButton->label()->text            ("OK");
        _okButton->bordered                 (true);
        
        _dismissButton->label()->text       ("Cancel");
        _dismissButton->label()->textAttr   (A_NORMAL);
        _dismissButton->bordered            (true);
//        _dismissButton->action              (std::bind(&ModalPanel::dismiss, this));
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
        const Point orig = mf.bl() + Size{0, (mf.h() ? _SectionSpacingY : 0)};
        return {orig, {mf.w(), h}};
    }
    
    Rect buttonFrame() const {
        const Rect cf = contentFrame();
        const int h = contentHeight(cf.w());
        if (!h) return {cf.bl(), {cf.w(), 0}};
        const Point orig = cf.bl() + Size{0, (cf.h() ? _SectionSpacingY : 0)};
        return {orig, {cf.w(), h}};
    }
    
    virtual int contentHeight(int width) const { return 0; }
//    virtual bool suppressEvents() const { return _suppressEvents; }
//    virtual bool suppressEvents(bool x) { return _set(_suppressEvents, x); }
    
    // MARK: - View Overrides
    Size sizeIntrinsic(Size constraint) override {
        constraint.x = std::min(_width, constraint.x);
        
        const Rect f = InteriorFrame({{}, constraint});
        int offY = 0;
        
        const int heightMessage = _message->sizeIntrinsic({f.w(), ConstraintNone}).y;
        offY += (offY && heightMessage ? _SectionSpacingY : 0) + heightMessage;
        
        const int heightContent = contentHeight(f.w());
        offY += (offY && heightContent ? _SectionSpacingY : 0) + heightContent;
        
        const Size okButtonSize = (okButtonVisible() ? _okButton->sizeIntrinsic(ConstraintNoneSize) : Size{});
        const Size dismissButtonSize = (dismissButtonVisible() ? _dismissButton->sizeIntrinsic(ConstraintNoneSize) : Size{});
        const int heightButton = std::max(okButtonSize.y, dismissButtonSize.y);
        offY += (offY && heightButton ? _SectionSpacingY : 0) + heightButton;
        
        offY += 2*BorderSize().y;
        
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
        
        const Rect mf = messageFrame();
        _message->frame(mf);
        
//        const Rect cf = contentFrame();
//        Point off = {cf.r(), cf.b() + _SectionSpacingY};
        
        const Rect bf = buttonFrame();
        Point boff = bf.tr();
        
        const Size okButtonSize = _okButton->sizeIntrinsic(ConstraintNoneSize) + Size{(!_condensed ? _ButtonPaddingX : 0), 0};
        _okButton->frame({boff+Size{-okButtonSize.x,0}, okButtonSize});
        _okButton->visible(okButtonVisible());
        _okButton->enabled(okButtonEnabled());
        if (okButtonVisible()) boff += {-okButtonSize.x-_ButtonSpacingX, 0};
        
        const Size dismissButtonSize = _dismissButton->sizeIntrinsic(ConstraintNoneSize) + Size{(!_condensed ? _ButtonPaddingX : 0), 0};
        _dismissButton->frame({boff+Size{-dismissButtonSize.x,0}, dismissButtonSize});
        _dismissButton->visible(dismissButtonVisible());
        _dismissButton->enabled(dismissButtonEnabled());
        if (dismissButtonVisible()) boff += {-dismissButtonSize.x-_ButtonSpacingX, 0};
        
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
    auto& okButton() { return _okButton; }
    auto& dismissButton() { return _dismissButton; }
    
    const auto& condensed() const { return _condensed; }
    template <typename T> bool condensed(const T& x) {
        const bool changed = _set(_condensed, x);
        // Updating buttons here so that our sizeIntrinsic() can get an accurate size immediately,
        // without needing a layout pass
        _okButton->bordered(!_condensed);
        _dismissButton->bordered(!_condensed);
        return changed;
    }
    
    const auto& escapeTriggersOK() const { return _escapeTriggersOK; }
    template <typename T> bool escapeTriggersOK(const T& x) { return _set(_escapeTriggersOK, x); }
    
    // MARK: - Methods
    virtual void ok() {
        if (okButtonVisible()) {
            if (okButtonEnabled() && _okButton->action()) {
                _okButton->action()(*_okButton);
            }
        }
    }
    
    virtual void dismiss() {
        if (dismissButtonVisible()) {
            if (dismissButtonEnabled() && _dismissButton->action()) {
                _dismissButton->action()(*_dismissButton);
            }
        }
    }
    
    virtual bool okButtonVisible() const { return (bool)_okButton->action(); }
    virtual bool okButtonEnabled() const { return true; }
    
    virtual bool dismissButtonVisible() const { return (bool)_dismissButton->action(); }
    virtual bool dismissButtonEnabled() const { return true; }
    
private:
    static constexpr int _SectionSpacingY       = 1;
    static constexpr int _ButtonPaddingX        = 4;
    static constexpr int _ButtonSpacingX        = 1;
    
    int _width = 0;
    Color _color;
    LabelPtr _title             = subviewCreate<Label>();
    LabelPtr _message           = subviewCreate<Label>();
    ButtonPtr _okButton         = subviewCreate<Button>();
    ButtonPtr _dismissButton    = subviewCreate<Button>();
    
    bool _condensed             = false;
    bool _escapeTriggersOK      = false;
};

using ModalPanelPtr = std::shared_ptr<ModalPanel>;

} // namespace UI
