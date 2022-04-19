#pragma once
#include <optional>
#include "Panel.h"
#include "Color.h"
#include "Label.h"

namespace UI {

class Alert : public Panel {
public:
    Alert() {
        _message->centerSingleLine(true);
        _message->wrap(true);
        _message->allowEmptyLines(true);
        
        _title->inhibitErase(true); // Title overlaps border, so don't erase
        _title->prefix(" ");
        _title->suffix(" ");
        
        _okButton->label()->text            ("OK");
        _okButton->bordered                 (true);
        
        _dismissButton->label()->text       ("Cancel");
        _dismissButton->label()->textAttr   (WA_NORMAL);
        _dismissButton->bordered            (true);
//        _dismissButton->action              (std::bind(&Alert::dismiss, this));
    }
    
    virtual int contentHeight(int width) const { return 0; }
    
    Rect contentFrame() const {
        return _contentFrameCached;
    }
    
    // MARK: - View Overrides
    Size sizeIntrinsic(Size constraint) override {
        constraint.x = std::min(_width, constraint.x);
        
        const Rect b = {{}, constraint};
        const Rect inf = _interiorFrame(b);
        const Rect mf = _messageFrame(inf);
        const Rect cf = _contentFrame(mf);
        const Rect bf = _buttonFrame(cf);
        const Rect& last = bf;
        
        Size s = Inset(bf, -_inset()).br();
        
        // If the last element's frame has a 0 height, subtract off the unused spacing that it added
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
    
    void layout() override {
        const Rect b   = _boundsLayout(bounds());
        const Rect inf = _interiorFrame(b);
        const Rect mf  = _messageFrame(inf);
        const Rect cf  = _contentFrame(mf);
        const Rect bf  = _buttonFrame(cf);
        
        const Point titleOrigin = b.origin+Size{_TitleInset,0};
        _title->frame({titleOrigin, {b.w()-2*_TitleInset, 1}});
        _title->visible(titleOrigin.y >= 0);
        
        _message->frame(mf);
        
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
        
        _contentFrameCached = cf;
        
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
        _title->textAttr(_color|WA_BOLD);
        // Always redraw _title because our border may have clobbered it
        _title->drawNeeded(true);
        
        // Set the highlight color of all button subviews
        // This is generic (instead of accessing _okButton/_dismissButton directly) so
        // that subclasses (eg ErrorAlert's clickable links) also inherit this behavior
        // for their own buttons
        auto it = subviewsBegin();
        for (;;) {
            ViewPtr subview = subviewsNext(it);
            if (!subview) break;
            if (ButtonPtr button = std::dynamic_pointer_cast<Button>(subview)) {
                button->highlightColor(_color);
            }
        }
        
        // Draw the border
        {
            Attr color = attr(_color);
            
            const int w = bounds().w();
            const int h = bounds().h();
            const Rect b = _boundsLayout(bounds());
            
            const bool left   = b.l() >= 0;
            const bool right  = b.r() <= w;
            const bool top    = b.t() >= 0;
            const bool bottom = b.b() <= h;
            
            // Draw the lines that aren't clipped
            if (left)   drawLineVert ({ 0,        0       }, h);
            if (right)  drawLineVert ({ b.r()-1,  0       }, h);
            if (top)    drawLineHoriz({ 0,        0       }, w);
            if (bottom) drawLineHoriz({ 0,        b.b()-1 }, w);
            
            // Draw the corners that aren't clipped
            if (left && top)     mvwaddch(window(), 0,       0,       ACS_ULCORNER);
            if (right && top)    mvwaddch(window(), 0,       b.r()-1, ACS_URCORNER);
            if (left && bottom)  mvwaddch(window(), b.b()-1, 0,       ACS_LLCORNER);
            if (right && bottom) mvwaddch(window(), b.b()-1, b.r()-1, ACS_LRCORNER);
            
//            {
//                Attr color = attr(_color);
//                drawRect();
//                drawLineVert({0,3}, 1);
//                drawLineVert({bounds().w()-1,3}, 1);
//                drawLineHoriz({1,3}, bounds().w()-2, ' ');
//            }
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
//        Edges edges = _truncateEdges;
//        if (ev.type == Event::Type::KeyLeft) {
//            edges.r++;
//        } else if (ev.type == Event::Type::KeyRight) {
//            edges.l++;
//        } else if (ev.type == Event::Type::KeyUp) {
//            edges.b++;
//        } else if (ev.type == Event::Type::KeyDown) {
//            edges.t++;
//        }
//        truncateEdges(edges);
//        screen().eraseNeeded(true);
//        screen().layoutNeeded(true);
        
//        const Point mid = {bounds().mx(), bounds().my()};
//        const Point delta = ev.mouse.origin - mid.x;
//        const Edges edges = {
//            .l = std::abs(std::max(0, delta.x)),
//            .r = std::abs(std::min(0, delta.x)),
//            .t = std::abs(std::max(0, delta.y)),
//            .b = std::abs(std::min(0, delta.y)),
//        };
//        truncateEdges(edges);
//        eraseNeeded(true);
        
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
            if (enabled()) { // Whether window is enabled
                if (okButtonEnabled() && _okButton->action()) {
                    _okButton->action()(*_okButton);
                }
            }
        }
    }
    
    virtual void dismiss() {
        if (dismissButtonVisible()) {
            if (enabled()) { // Whether window is enabled
                if (dismissButtonEnabled() && _dismissButton->action()) {
                    _dismissButton->action()(*_dismissButton);
                }
            }
        }
    }
    
    virtual bool okButtonVisible() const { return (bool)_okButton->action(); }
    virtual bool okButtonEnabled() const { return true; }
    
    virtual bool dismissButtonVisible() const { return (bool)_dismissButton->action(); }
    virtual bool dismissButtonEnabled() const { return true; }
    
private:
    static constexpr int _TitleInset            = 2;
    static constexpr int _SectionSpacingY       = 1;
    static constexpr int _ButtonPaddingX        = 4;
    static constexpr int _ButtonSpacingX        = 1;
    
    Edges _inset() const {
        if (!_condensed) return {.l=5, .r=5, .t=2, .b=1};
        else             return {.l=4, .r=4, .t=2, .b=2};
    }
    
    // boundsLayout(): returns that bounds that should be used for layout purposes
    Rect _boundsLayout(const Rect& bounds) const {
        return Inset(bounds, -_truncateEdges);
    }
    
    Rect _interiorFrame(Rect bounds) const {
        return Inset(bounds, _inset());
    }
    
    Rect _messageFrame(Rect interiorFrame) const {
        Rect r = interiorFrame;
        r.size.y = _message->sizeIntrinsic({interiorFrame.w(), ConstraintNone}).y;
        return r;
    }
    
    Rect _contentFrame(Rect messageFrame) const {
        const int h = contentHeight(messageFrame.w());
        const Point p = messageFrame.bl() + Size{0, (messageFrame.h() ? _SectionSpacingY : 0)};
        return {p, {messageFrame.w(), h}};
    }
    
    Rect _buttonFrame(Rect contentFrame) const {
        const Size okButtonSize = (okButtonVisible() ? _okButton->sizeIntrinsic(ConstraintNoneSize) : Size{});
        const Size dismissButtonSize = (dismissButtonVisible() ? _dismissButton->sizeIntrinsic(ConstraintNoneSize) : Size{});
        const int h = std::max(okButtonSize.y, dismissButtonSize.y);
        const Point p = contentFrame.bl() + Size{0, (contentFrame.h() ? _SectionSpacingY : 0)};
        return {p, {contentFrame.w(), h}};
    }
    
    int _width = 0;
    Color _color;
    LabelPtr _title             = subviewCreate<Label>();
    LabelPtr _message           = subviewCreate<Label>();
    ButtonPtr _okButton         = subviewCreate<Button>();
    ButtonPtr _dismissButton    = subviewCreate<Button>();
    
    Edges _truncateEdges;
    bool _condensed             = false;
    bool _escapeTriggersOK      = false;
    
    Rect _contentFrameCached;
};

using AlertPtr = std::shared_ptr<Alert>;

} // namespace UI
