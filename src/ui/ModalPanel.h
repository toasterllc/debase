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
    
    Edges inset() const {
        if (!_condensed) return {.l=5, .r=5, .t=2, .b=1};
        else             return {.l=4, .r=4, .t=2, .b=2};
    }
    
    
//    Size borderSize() const {
//        return (!_condensed ? Size{5,1} : Size{3,2});
//    }
    
//    Rect interiorFrame(Rect bounds) { return Inset(bounds, BorderSize()); }
    
//    Rect truncatedBounds(Rect bounds) const {
//        bounds.size -= Size{_truncateEdges.r, _truncateEdges.b};
//        Rect r = Inset(bounds, borderSize());
//        r.origin -= Size{_truncateEdges.l, _truncateEdges.t};
//        return r;
//    }
    
    Rect interiorFrame(Rect bounds) const {
        return Inset(bounds, inset());
//        bounds.size -= Size{_truncateEdges.r, _truncateEdges.b};
//        Rect r = Inset(bounds, borderSize());
//        r.origin -= Size{_truncateEdges.l, _truncateEdges.t};
//        return r;
    }
    
    Rect messageFrame(Rect interiorFrame) const {
        Rect r = interiorFrame;
        r.size.y = _message->sizeIntrinsic({interiorFrame.w(), ConstraintNone}).y;
        return r;
    }
    
    Rect contentFrame(Rect messageFrame) const {
        const int h = contentHeight(messageFrame.w());
        const Point p = messageFrame.bl() + Size{0, (messageFrame.h() ? _SectionSpacingY : 0)};
        return {p, {messageFrame.w(), h}};
    }
    
    Rect buttonFrame(Rect contentFrame) const {
        const Size okButtonSize = (okButtonVisible() ? _okButton->sizeIntrinsic(ConstraintNoneSize) : Size{});
        const Size dismissButtonSize = (dismissButtonVisible() ? _dismissButton->sizeIntrinsic(ConstraintNoneSize) : Size{});
        const int h = std::max(okButtonSize.y, dismissButtonSize.y);
        const Point p = contentFrame.bl() + Size{0, (contentFrame.h() ? _SectionSpacingY : 0)};
        return {p, {contentFrame.w(), h}};
    }
    
    virtual int contentHeight(int width) const { return 0; }
//    virtual bool suppressEvents() const { return _suppressEvents; }
//    virtual bool suppressEvents(bool x) { return _set(_suppressEvents, x); }
    
    // MARK: - View Overrides
    Size sizeIntrinsic(Size constraint) override {
        constraint.x = std::min(_width, constraint.x);
        
        const Rect mf = messageFrame(interiorFrame({{}, constraint}));
        const Rect cf = contentFrame(mf);
        const Rect bf = buttonFrame(cf);
        const Rect& last = bf;
        
        Size s = Inset(bf, -inset()).br();
        
        // If the last frame calculation function returns a 0 height, subtract off the spacing that it adds
//        s.y -= _SectionSpacingY;
        if (!last.h()) s.y -= _SectionSpacingY;
        
//        s.x -= _truncateEdges.r;
//        s.y -= _truncateEdges.b;
//        
        s.x -= (_truncateEdges.l + _truncateEdges.r);
        s.y -= (_truncateEdges.t + _truncateEdges.b);
        
        return s;
        
//        const Rect f = InteriorFrame({{}, constraint});
//        int offY = 0;
//        
//        const int heightMessage = _message->sizeIntrinsic({f.w(), ConstraintNone}).y;
//        offY += (offY && heightMessage ? _SectionSpacingY : 0) + heightMessage;
//        
//        const int heightContent = contentHeight(f.w());
//        offY += (offY && heightContent ? _SectionSpacingY : 0) + heightContent;
//        
//        const Size okButtonSize = (okButtonVisible() ? _okButton->sizeIntrinsic(ConstraintNoneSize) : Size{});
//        const Size dismissButtonSize = (dismissButtonVisible() ? _dismissButton->sizeIntrinsic(ConstraintNoneSize) : Size{});
//        const int heightButton = std::max(okButtonSize.y, dismissButtonSize.y);
//        offY += (offY && heightButton ? _SectionSpacingY : 0) + heightButton;
//        
//        offY += 2*BorderSize().y;
//        
//        const Size s = {
//            .x = constraint.x,
//            .y = offY,
//        };
//        return s;
    }
    
    Rect boundsUntruncated() {
        Rect b = bounds();
        b.origin -= Size{_truncateEdges.l, _truncateEdges.t};
        b.size += Size{_truncateEdges.l, _truncateEdges.t};
        b.size += Size{_truncateEdges.r, _truncateEdges.b};
        return b;
    }
    
    void layout() override {
        const Rect b = boundsUntruncated();
        const Point titleOrigin = b.origin+Size{2,0};
        _title->frame({titleOrigin, {b.w()-4, 1}});
        _title->visible(titleOrigin.y >= 0);
        
        const Rect inf = interiorFrame(b);
        const Rect mf = messageFrame(inf);
        _message->frame(mf);
        
//        if (!_condensed) _message->textAttr();
//        else             _message->textAttr();
        
//        const Rect cf = contentFrame();
//        Point off = {cf.r(), cf.b() + _SectionSpacingY};
        
        const Rect bf = buttonFrame(contentFrame(mf));
        Point boff = bf.tr();
        
        if (!_condensed) {
            const Size okButtonSize = _okButton->sizeIntrinsic(ConstraintNoneSize) + Size{_ButtonPaddingX, 0};
            _okButton->frame({boff+Size{-okButtonSize.x,0}, okButtonSize});
            _okButton->visible(okButtonVisible());
            _okButton->enabled(okButtonEnabled());
            if (okButtonVisible()) boff += {-okButtonSize.x-_ButtonSpacingX, 0};
            
            const Size dismissButtonSize = _dismissButton->sizeIntrinsic(ConstraintNoneSize) + Size{_ButtonPaddingX, 0};
            _dismissButton->frame({boff+Size{-dismissButtonSize.x,0}, dismissButtonSize});
            _dismissButton->visible(dismissButtonVisible());
            _dismissButton->enabled(dismissButtonEnabled());
            if (dismissButtonVisible()) boff += {-dismissButtonSize.x-_ButtonSpacingX, 0};
        
        } else {
            _okButton->sizeToFit(ConstraintNoneSize);
            _okButton->origin(bf.tl());
            _okButton->visible(okButtonVisible());
            _okButton->enabled(okButtonEnabled());
            
            _dismissButton->sizeToFit(ConstraintNoneSize);
            _dismissButton->origin(bf.tr()-Size{_dismissButton->frame().w(), 0});
            _dismissButton->visible(dismissButtonVisible());
            _dismissButton->enabled(dismissButtonEnabled());
        }
        
//        _title->layout(*this);
//        _message->layout(*this);
    }
    
//    bool drawNeeded() const override {
//        return true;
//    }
    
    void draw() override {
//        Rect r = Inset(interiorFrame(bounds()), -borderSize());
//        drawRect(r);
//        borderColor(_color);
        _title->textAttr(_color|A_BOLD);
        // Always redraw _title because our border may have clobbered it
        _title->drawNeeded(true);
        
        {
            Attr color = attr(_color);
            drawRect();
            drawLineVert({0,3}, 1);
            drawLineVert({bounds().w()-1,3}, 1);
            drawLineHoriz({1,3}, bounds().w()-2, ' ');
        }
        
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
    
    const auto& truncateEdges() const { return _truncateEdges; }
    template <typename T> bool truncateEdges(const T& x) { return _setForce(_truncateEdges, x); }
    
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
    
    Edges _truncateEdges;
    bool _condensed             = false;
    bool _escapeTriggersOK      = false;
};

using ModalPanelPtr = std::shared_ptr<ModalPanel>;

} // namespace UI
