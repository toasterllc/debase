#pragma once
#include <optional>
#include <climits>
#include <cassert>
#include "UI.h"
#include "Color.h"
#include "lib/toastbox/Defer.h"

namespace UI {

class Window;
class Screen;

class View {
public:
    using Ptr = std::shared_ptr<View>;
    using WeakPtr = std::weak_ptr<View>;
    
    using Views = std::list<WeakPtr>;
    using ViewsIter = Views::iterator;
    
    using Deadline = std::chrono::steady_clock::time_point;
    static constexpr Deadline Forever = std::chrono::steady_clock::time_point::max();
    static constexpr Deadline Once = std::chrono::steady_clock::time_point::min();
    static constexpr Deadline Poll = std::chrono::steady_clock::time_point();
    static_assert(Once != Poll);
    
//    class GraphicsState {
//    public:
//        GraphicsState() {}
//        
//        GraphicsState(GraphicsState& x, const Window& window, const Point& origin, bool erased) :
//        _s{.prev=&x, .window=&window, .origin=origin, .erased=erased} {
//            std::swap(_s, _s.prev->_s);
//        }
//        
//        GraphicsState(const GraphicsState& x) = delete;
//        GraphicsState(GraphicsState&& x) = delete;
//        
//        ~GraphicsState() {
//            if (_s.prev) {
//                std::swap(_s, _s.prev->_s);
//            }
//        }
//        
//        operator bool() const { return _s.prev; }
//        const Window* window() const { return _s.window; }
//        const Point& origin() const { return _s.origin; }
//        bool erased() const { return _s.erased; }
//    
//    private:
//        struct {
//            GraphicsState* prev = nullptr;
//            const Window* window = nullptr;
//            Point origin;
//            bool erased = false;
//        } _s;
//    };
    
    class Attr {
    public:
        Attr() {}
        Attr(WINDOW* window, attr_t attr) : _s({.window=window, .attr=attr}) {
            assert(window);
//            if (rand() % 2) {
//                wattron(_s.window, WA_REVERSE);
//            } else {
//                wattroff(_s.window, WA_REVERSE);
//            }
            // MARK: - Drawing
            wattr_on(_s.window, _s.attr, nullptr);
        }
        
        Attr(const Attr& x) = delete;
        Attr(Attr&& x) { std::swap(_s, x._s); }
        Attr& operator =(Attr&& x) { std::swap(_s, x._s); return *this; }
        
        ~Attr() {
            if (_s.window) {
                wattr_off(_s.window, _s.attr, nullptr);
            }
        }
    
    private:
        struct {
            WINDOW* window = nullptr;
            attr_t attr = 0;
        } _s;
    };
    
    static const GraphicsState& GState() {
        assert(_GState);
        return _GState;
    }
    
    static auto GStatePush(const GraphicsState& x) {
        return _GraphicsStateSwapper(_GState, x);
    }
    
    static Point SubviewConvert(const View& dst, const Point& p) { return p-dst.origin(); }
    static Rect SubviewConvert(const View& dst, const Rect& r) { return { SubviewConvert(dst, r.origin), r.size }; }
    static Event SubviewConvert(const View& dst, const Event& ev) {
        Event r = ev;
        if (r.type == Event::Type::Mouse) {
            r.mouse.origin = SubviewConvert(dst, ev.mouse.origin);
        }
        return r;
    }
    static Event SubviewConvert(const GraphicsState& dst, const Event& ev) {
        Event r = ev;
        if (r.type == Event::Type::Mouse) {
            r.mouse.origin = ev.mouse.origin-dst.originScreen;
        }
        return r;
    }
    
    static Point SuperviewConvert(const View& src, const Point& p) { return p+src.origin(); }
    static Rect SuperviewConvert(const View& src, const Rect& r) { return { SuperviewConvert(src, r.origin), r.size }; }
    static Event SuperviewConvert(const View& src, const Event& ev) {
        Event r = ev;
        if (r.type == Event::Type::Mouse) {
            r.mouse.origin = SuperviewConvert(src, ev.mouse.origin);
        }
        return r;
    }
    
//    // Convert(): convert a point from `view`'s parent coordinate system to `view`'s coordinate system
//    static Point Convert(const View& view, const Point& p) {
//        return p-view.origin();
//    }
//    
//    // Convert(): convert a rect from `view`'s parent coordinate system to `view`'s coordinate system
//    static Rect Convert(const View& view, const Rect& r) {
//        return { Convert(view, r.origin), r.size };
//    }
//    
//    // Convert(): convert an event from `view`'s parent coordinate system to `view`'s coordinate system
//    static Event Convert(const View& view, const Event& ev) {
//        Event r = ev;
//        if (r.type == Event::Type::Mouse) {
//            r.mouse.origin = Convert(view, ev.mouse.origin);
//        }
//        return r;
//    }
    
    virtual ~View() = default;
    
    virtual bool hitTest(const Point& p) const {
        return HitTest(bounds(), p, _hitTestInset);
    }
    
    static constexpr int ConstraintNone = INT_MAX;
    static constexpr Size ConstraintNoneSize = {ConstraintNone, ConstraintNone};
    
    virtual Size sizeIntrinsic(Size constraint) { return size(); }
    virtual void sizeToFit(Size constraint) { size(sizeIntrinsic(constraint)); }
    
//    // sizeIntrinsic() / sizeToFit(): intentionally non-virtual; override the constraint versions above
//    Size sizeIntrinsic() { return sizeIntrinsic({ConstraintNone, ConstraintNone}); }
//    void sizeToFit() { size(sizeIntrinsic()); }
    
    virtual Point origin() const { return _origin; }
    virtual bool origin(const Point& x) {
        const Point xadj = {std::max(0,x.x), std::max(0,x.y)};
        if (xadj == _origin) return false;
        _origin = xadj;
        drawNeeded(true);
        return true;
    }
    
    virtual Size size() const { return _size; }
    virtual bool size(const Size& x) {
        const Point xadj = {std::max(0,x.x), std::max(0,x.y)};
        if (xadj == _size) return false;
        _size = xadj;
        layoutNeeded(true);
        eraseNeeded(true);
        drawNeeded(true);
        return true;
    }
    
    virtual Rect frame() const { return { .origin=origin(), .size=size() }; }
    virtual bool frame(const Rect& x) {
        bool changed = false;
        changed |= size(x.size);
        changed |= origin(x.origin);
        return changed;
    }
    
    virtual Rect bounds() const { return { .size = size() }; }
    
//    virtual Point _GState.origin const { return _GraphicsState.origin(); }
    
//    Point mousePosition(const Event& ev) const {
//        return ev.mouse.origin-origin();
//    }
    
    // MARK: - Attributes
    virtual Attr attr(attr_t attr) const { return Attr(_window(), attr); }
    
    // MARK: - Drawing
    virtual void drawRect() const {
        drawRect({{}, size()});
    }
    
    virtual void drawRect(const Rect& rect) const {
        const Rect r = {_GState.originWindow+rect.origin, rect.size};
        const int x1 = r.origin.x;
        const int y1 = r.origin.y;
        const int x2 = r.origin.x+r.size.x-1;
        const int y2 = r.origin.y+r.size.y-1;
        mvwhline(_window(), y1, x1, 0, r.size.x);
        mvwhline(_window(), y2, x1, 0, r.size.x);
        mvwvline(_window(), y1, x1, 0, r.size.y);
        mvwvline(_window(), y1, x2, 0, r.size.y);
        mvwaddch(_window(), y1, x1, ACS_ULCORNER);
        mvwaddch(_window(), y2, x1, ACS_LLCORNER);
        mvwaddch(_window(), y1, x2, ACS_URCORNER);
        mvwaddch(_window(), y2, x2, ACS_LRCORNER);
    }
    
    virtual void drawLineHoriz(const Point& p, int len, chtype ch=0) const {
        const Point off = _GState.originWindow;
        mvwhline(_window(), off.y+p.y, off.x+p.x, ch, len);
    }
    
    virtual void drawLineVert(const Point& p, int len, chtype ch=0) const {
        const Point off = _GState.originWindow;
        mvwvline(_window(), off.y+p.y, off.x+p.x, ch, len);
    }
    
    virtual void drawText(const Point& p, const char* txt) const {
        const Point off = _GState.originWindow;
        mvwprintw(_window(), off.y+p.y, off.x+p.x, "%s", txt);
    }
    
    virtual void drawText(const Point& p, int widthMax, const char* txt) const {
        const Point off = _GState.originWindow;
        widthMax = std::max(0, widthMax);
        
        std::string str = txt;
        auto it = UTF8::NextN(str.begin(), str.end(), widthMax);
        str.resize(std::distance(str.begin(), it));
        mvwprintw(_window(), off.y+p.y, off.x+p.x, "%s", str.c_str());
    }
    
    template <typename ...T_Args>
    void drawText(const Point& p, const char* fmt, T_Args&&... args) const {
        const Point off = _GState.originWindow;
        mvwprintw(_window(), off.y+p.y, off.x+p.x, fmt, std::forward<T_Args>(args)...);
    }
    
    // MARK: - Accessors
    virtual bool visible() const { return _visible; }
    virtual bool visible(bool x) { return _set(_visible, x); }
    
    virtual bool enabled() const { return _enabled; }
    virtual bool enabled(bool x) { return _set(_enabled, x); }
    
    // enabledWindow(): whether the view is enabled, and its containing window is also enabled
    virtual bool enabledWindow() const;
    
    virtual const bool interaction() const { return _interaction; }
    virtual bool interaction(bool x) { return _set(_interaction, x); }
    
    virtual const std::optional<Color> borderColor() const { return _borderColor; }
    virtual bool borderColor(std::optional<Color> x) { return _set(_borderColor, x); }
    
    virtual const Edges& hitTestInset() const { return _hitTestInset; }
    virtual bool hitTestInset(const Edges& x) { return _setForce(_hitTestInset, x); }
    
    virtual const bool inhibitErase() const { return _inhibitErase; }
    virtual bool inhibitErase(bool x) { return _set(_inhibitErase, x); }
    
    virtual Screen& screen() const {
        assert(_GState);
        return *_GState.screen;
    }
    
    virtual Window& window() const {
        assert(_GState);
        return *_GState.window;
    }
    
    virtual bool erased() const {
        assert(_GState);
        return _GState.erased;
    }
    
    virtual const ColorPalette& colors() const;
    virtual bool cursorState(const CursorState& x);
    
    // MARK: - Layout
    virtual bool layoutNeeded() const { return _layoutNeeded; }
    virtual void layoutNeeded(bool x) { _layoutNeeded = x; }
    virtual void layout() {}
    
    // MARK: - Erase
    virtual bool eraseNeeded() const { return _eraseNeeded; }
    virtual void eraseNeeded(bool x) { _eraseNeeded = x; }
    virtual void erase() {
        if (!_inhibitErase) {
//            os_log(OS_LOG_DEFAULT, "VIEW ERASE");
            const Size s = size();
            for (int y=0; y<s.y; y++) drawLineHoriz({0,y}, s.x, ' ');
        }
    }
    
    // MARK: - Draw
    virtual bool drawNeeded() const { return _drawNeeded; }
    virtual void drawNeeded(bool x) { _drawNeeded = x; }
    virtual void draw() {}
    
    virtual void drawBackground() {}
    
    virtual void drawBorder() {
        if (_borderColor) {
            Attr color = attr(*_borderColor);
            drawRect();
        }
    }
    
    // MARK: - Events
    virtual bool handleEvent(const Event& ev) { return false; }
    
    virtual void track(Deadline deadline=Forever);
    
    virtual void trackStop() {
        _trackStop = true;
    }
    
    virtual bool tracking() const { return _tracking; }
    
    virtual GraphicsState convert(GraphicsState x) {
        x.originWindow += origin();
        x.originScreen += origin();
        return x;
    }
    
    // MARK: - Tree Traversal
    virtual void layout(GraphicsState gstate) {
        if (!visible()) return;
        
//        // When we hit a window, set the window's size/origin according to the gstate, and reset gstate
//        // to reference the new window, and the origin to {0,0}
//        if (Window* win = dynamic_cast<Window*>(&view)) {
//            win->windowSize(win->size());
//            win->windowOrigin(gstate.origin);
//            
//            gstate.window = win;
//            gstate.origin = {};
//        
//        } else {
//            gstate.origin += origin();
//        }
        
        auto gpushed = View::GStatePush(gstate);
        
        if (layoutNeeded()) {
            layout();
            layoutNeeded(false);
        }
        
        auto subviewsId = _subviewsId;
        auto it = subviewsBegin();
        for (;;) {
            // Detect _subviews being modified while we're iterating
            assert(_subviewsId == subviewsId);
            
            Ptr subview = subviewsNext(it);
            if (!subview) break;
            subview->layout(subview->convert(gstate));
        }
    }
    
    
//    virtual void draw(GraphicsState gstate) override {
//        if (!visible()) return;
//        if (eraseNeeded()) {
//            gstate.erased = true;
//            ::werase(*this);
//            eraseNeeded(false);
//        }
//        
//        View::draw(gstate);
//    }
    
    
    virtual void draw(GraphicsState gstate) {
        if (!visible()) return;
        
//        // When we hit a window, reset gstate to reference the new window, and the origin to {0,0}
//        // Also, erase the window if needed, and remember that we did so in the gstate
//        if (Window* win = dynamic_cast<Window*>(&view)) {
//            gstate.window = win;
//            gstate.origin = {};
//            
//            if (win->eraseNeeded()) {
//                gstate.erased = true;
//                ::werase(*win);
//                win->eraseNeeded(false);
//            }
//        
//        } else {
//            gstate.origin += origin();
//        }
        
        const bool erasedPrev = gstate.erased;
        const bool erased = eraseNeeded();
        gstate.erased |= erased;
        auto gpushed = View::GStatePush(gstate);
        
        // If the superview erased itself, then we don't need to erase ourself since we're
        // a subview of the superview, and therefore have already been erased
        if (!erasedPrev && erased) erase();
        eraseNeeded(false);
        
        // Redraw the view if it says it needs it, or if this part of the view tree has been erased
        if (drawNeeded() || gstate.erased) {
            drawBackground();
            draw();
            drawBorder();
            drawNeeded(false);
        }
        
        auto subviewsId = _subviewsId;
        auto it = subviewsBegin();
        for (;;) {
            // Detect _subviews being modified while we're iterating
            assert(_subviewsId == subviewsId);
            
            Ptr subview = subviewsNext(it);
            if (!subview) break;
            subview->draw(subview->convert(gstate));
        }
    }
    
    virtual bool handleEvent(GraphicsState gstate, const Event& ev) {
        if (!visible()) return false;
        if (!interaction()) return false;
        
        auto gpushed = View::GStatePush(gstate);
        
        // We have to copy _subviews since we allow it to be modified while we're iterating over it
        // within our handleEvent() callout
        // TODO: optimize copying _subviews by using a stack array
        Views subviews = _subviews;
        for (auto it=subviews.rbegin(); it!=subviews.rend(); it++) {
            Ptr subview = (*it).lock();
            if (!subview) continue;
            if (subview->handleEvent(subview->convert(gstate), ev)) return true;
        }
        
        // None of the subviews wanted the event; let the view itself handle it
        return handleEvent(SubviewConvert(gstate, ev));
    }
    
    
    
    
    
    
    
    
    
    
    
    
    // MARK: - Subviews
    
//    virtual std::list<WeakPtr>& subviews() { return _subviews; }
    
    virtual void subviews(const std::list<Ptr>& x) {
//        // Copy `x` into `svs` using strong pointers
//        // We can't just clear `_subviews`
//        std::list<Ptr> svs;
//        for (WeakPtr svweak : x) {
//            Ptr sv = svweak.lock();
//            if (sv) svs.push_back(sv);
//        }
        
        _subviews = {};
        for (Ptr sv : x) subviewAdd(sv);
        _subviewsId++;
    }
    
    virtual ViewsIter subviewsBegin() {
        return _subviews.begin();
    }
    
    virtual ViewsIter subviewsEnd() {
        return _subviews.end();
    }
    
    virtual Ptr subviewsNext(ViewsIter& it) {
        Ptr subview = nullptr;
        while (it!=_subviews.end() && !subview) {
            subview = (*it).lock();
            if (!subview) {
                // Prune and continue
                it = _subviews.erase(it);
            } else {
                it++;
            }
        }
        return subview;
    }
    
    virtual Ptr subviewsPrev(ViewsIter& it) {
        Ptr subview = nullptr;
        while (it!=_subviews.begin() && !subview) {
            it--;
            subview = (*it).lock();
            if (!subview) {
                // Prune and continue
                it = _subviews.erase(it);
            }
        }
        return subview;
    }
    
    template <typename T, typename ...T_Args>
    std::shared_ptr<T> subviewCreate(T_Args&&... args) {
        auto view = std::make_shared<T>(std::forward<T_Args>(args)...);
        subviewAdd(view);
        return view;
    }
    
    virtual void subviewAdd(Ptr view) {
        assert(view);
        _subviews.push_back(view);
        _subviewsId++;
        view->addedToSuperview(*this);
        layoutNeeded(true);
    }
    
    virtual void addedToSuperview(View& superview) {}
    
//    for (auto it=_subviews.begin(); it!=_subviews.end();) {
//        Ptr subview = (*it).lock();
//        if (!subview) {
//            // Prune and continue
//            it = _subviews.erase(it);
//            continue;
//        }
//        
//        subview->layout(win, _TState.origin()+subview->origin());
//        it++;
//    }
    
//    virtual std::list<WeakPtr>& subviewsIter() {
//        
//        for (auto it=_subviews.begin(); it!=_subviews.end();) {
//            Ptr subview = (*it).lock();
//            if (!subview) {
//                // Prune and continue
//                it = _subviews.erase(it);
//                continue;
//            }
//            
//            subview->layout(win, _TState.origin()+subview->origin());
//            it++;
//        }
//        
//    }
    
protected:
    template <typename X, typename Y>
    bool _setForce(X& x, const Y& y) {
        x = y;
        layoutNeeded(true);
        drawNeeded(true);
        return true;
    }
    
    template <typename X, typename Y>
    bool _set(X& x, const Y& y) {
        if (x == y) return false;
        x = y;
        layoutNeeded(true);
        drawNeeded(true);
        return true;
    }
    
protected:
    class _GraphicsStateSwapper {
    public:
        _GraphicsStateSwapper() {}
        _GraphicsStateSwapper(GraphicsState& dst, const GraphicsState& val) : _s{.dst=&dst, .val=val} {
            std::swap(*_s.dst, _s.val);
        }
        
        _GraphicsStateSwapper(const _GraphicsStateSwapper& x) = delete;
        _GraphicsStateSwapper(_GraphicsStateSwapper&& x) = delete;
        
        _GraphicsStateSwapper& operator =(_GraphicsStateSwapper&& x) = delete;
        
//        _GraphicsStateSwapper& operator =(_GraphicsStateSwapper&& x) {
//            // Only allow move-assignment if we didn't have a value previously
//            assert(!_s.dst);
//            std::swap(_s, x._s);
//            return *this;
//        }
        
        ~_GraphicsStateSwapper() {
            if (_s.dst) {
                std::swap(*_s.dst, _s.val);
            }
        }
    
    private:
        struct {
            GraphicsState* dst = nullptr;
            GraphicsState val;
        } _s;
    };
    
private:
    WINDOW* _window() const;
    
    static inline GraphicsState _GState;
    
    Views _subviews;
    uint32_t _subviewsId = 0;
    Point _origin;
    Size _size;
    bool _visible = true;
    bool _enabled = true;
    bool _interaction = true;
    bool _layoutNeeded = true;
    bool _eraseNeeded = false;
    bool _drawNeeded = true;
    bool _tracking = false;
    bool _trackStop = false;
    bool _inhibitErase = false;
    Edges _hitTestInset;
    std::optional<Color> _borderColor;
};

using ViewPtr = std::shared_ptr<View>;

} // namespace UI
