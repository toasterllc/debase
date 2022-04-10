#pragma once
#include <optional>
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
        Attr(WINDOW* window, int attr) : _s({.window=window, .attr=attr}) {
            assert(window);
//            if (rand() % 2) {
//                wattron(*_s.win, A_REVERSE);
//            } else {
//                wattroff(*_s.win, A_REVERSE);
//            }
            // MARK: - Drawing
            wattron(_s.window, _s.attr);
        }
        
        Attr(const Attr& x) = delete;
        Attr(Attr&& x) { std::swap(_s, x._s); }
        Attr& operator =(Attr&& x) { std::swap(_s, x._s); return *this; }
        
        ~Attr() {
            if (_s.window) {
                wattroff(_s.window, _s.attr);
            }
        }
    
    private:
        struct {
            WINDOW* window = nullptr;
            int attr = 0;
        } _s;
    };
    
    struct HitTestExpand {
        int l = 0;
        int r = 0;
        int t = 0;
        int b = 0;
    };
    
    static const ColorPalette& Colors() { return _Colors; }
    static void Colors(const ColorPalette& x) { _Colors = x; }
    
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
        Rect b = bounds();
        b.origin.x -= _hitTestExpand.l;
        b.size.x   += _hitTestExpand.l;
        
        b.size.x   += _hitTestExpand.r;
        
        b.origin.y -= _hitTestExpand.t;
        b.size.y   += _hitTestExpand.t;
        
        b.size.y   += _hitTestExpand.b;
        return HitTest(b, p);
    }
    
    virtual Size sizeIntrinsic(Size constraint) { return size(); }
    
    virtual Point origin() const { return _origin; }
    virtual void origin(const Point& x) {
        if (_origin == x) return;
        _origin = x;
        drawNeeded(true);
    }
    
    virtual Size size() const { return _size; }
    virtual void size(const Size& x) {
        if (_size == x) return;
        _size = x;
        layoutNeeded(true);
        drawNeeded(true);
    }
    
    virtual Rect frame() const { return { .origin=origin(), .size=size() }; }
    virtual void frame(const Rect& x) {
        if (frame() == x) return;
        origin(x.origin);
        size(x.size);
    }
    
    virtual Rect bounds() const { return { .size = size() }; }
    
//    virtual Point _GState.origin const { return _GraphicsState.origin(); }
    
//    Point mousePosition(const Event& ev) const {
//        return ev.mouse.origin-origin();
//    }
    
    // MARK: - Attributes
    virtual Attr attr(int attr) const { return Attr(_gstateWindow(), attr); }
    
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
        mvwhline(_gstateWindow(), y1, x1, 0, r.size.x);
        mvwhline(_gstateWindow(), y2, x1, 0, r.size.x);
        mvwvline(_gstateWindow(), y1, x1, 0, r.size.y);
        mvwvline(_gstateWindow(), y1, x2, 0, r.size.y);
        mvwaddch(_gstateWindow(), y1, x1, ACS_ULCORNER);
        mvwaddch(_gstateWindow(), y2, x1, ACS_LLCORNER);
        mvwaddch(_gstateWindow(), y1, x2, ACS_URCORNER);
        mvwaddch(_gstateWindow(), y2, x2, ACS_LRCORNER);
    }
    
    virtual void drawLineHoriz(const Point& p, int len, chtype ch=0) const {
        const Point off = _GState.originWindow;
        mvwhline(_gstateWindow(), off.y+p.y, off.x+p.x, ch, len);
    }
    
    virtual void drawLineVert(const Point& p, int len, chtype ch=0) const {
        const Point off = _GState.originWindow;
        mvwvline(_gstateWindow(), off.y+p.y, off.x+p.x, ch, len);
    }
    
    virtual void drawText(const Point& p, const char* txt) const {
        const Point off = _GState.originWindow;
        mvwprintw(_gstateWindow(), off.y+p.y, off.x+p.x, "%s", txt);
    }
    
    virtual void drawText(const Point& p, int widthMax, const char* txt) const {
        const Point off = _GState.originWindow;
        widthMax = std::max(0, widthMax);
        
        std::string str = txt;
        auto it = UTF8::NextN(str.begin(), str.end(), widthMax);
        str.resize(std::distance(str.begin(), it));
        mvwprintw(_gstateWindow(), off.y+p.y, off.x+p.x, "%s", str.c_str());
    }
    
    template <typename ...T_Args>
    void drawText(const Point& p, const char* fmt, T_Args&&... args) const {
        const Point off = _GState.originWindow;
        mvwprintw(_gstateWindow(), off.y+p.y, off.x+p.x, fmt, std::forward<T_Args>(args)...);
    }
    
    // MARK: - Accessors
    virtual bool visible() const { return _visible; }
    virtual void visible(bool x) { _set(_visible, x); }
    
    virtual const bool interaction() const { return _interaction; }
    virtual void interaction(bool x) { _set(_interaction, x); }
    
    virtual const std::optional<Color> borderColor() const { return _borderColor; }
    virtual void borderColor(std::optional<Color> x) { _set(_borderColor, x); }
    
    virtual const HitTestExpand& hitTestExpand() const { return _hitTestExpand; }
    virtual void hitTestExpand(const HitTestExpand& x) { _setForce(_hitTestExpand, x); }
    
    virtual bool erased() { return _GState.erased; }
    
    // MARK: - Layout
    virtual bool layoutNeeded() const { return _layoutNeeded; }
    virtual void layoutNeeded(bool x) { _layoutNeeded = x; }
    virtual void layout() {}
    
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
    virtual const Event& eventCurrent() const { return _eventCurrent; }
    
    virtual void track(const Event& ev);
    
    virtual void trackStop() {
        _trackStop = true;
    }
    
    virtual bool tracking() const { return _tracking; }
    
    virtual GraphicsState convert(GraphicsState x) {
        x.originWindow += origin();
        x.originScreen += origin();
        return x;
    }
    
    virtual void layoutTree(GraphicsState gstate) {
        if (!visible()) return;
//        
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
        
        auto it = subviews();
        for (;;) {
            Ptr subview = subviewsNext(it);
            if (!subview) break;
            subview->layoutTree(subview->convert(gstate));
        }
    }
    
    virtual void drawTree(GraphicsState gstate) {
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
        
        auto gpushed = View::GStatePush(gstate);
        
        // Redraw the view if it says it needs it, or if this part of the view tree has been erased
        if (drawNeeded() || gstate.erased) {
            drawBackground();
            draw();
            drawBorder();
            drawNeeded(false);
        }
        
        auto it = subviews();
        for (;;) {
            Ptr subview = subviewsNext(it);
            if (!subview) break;
            subview->drawTree(subview->convert(gstate));
        }
    }
    
    virtual bool handleEventTree(GraphicsState gstate, const Event& ev) {
        if (!visible()) return false;
        if (!interaction()) return false;
        
        auto it = subviews();
        for (;;) {
            Ptr subview = subviewsNext(it);
            if (!subview) break;
            if (subview->handleEventTree(subview->convert(gstate), ev)) return true;
        }
        
        // None of the subviews wanted the event; let the view itself handle it
        return handleEvent(SubviewConvert(gstate, ev));
    }
    
    
    
    
    
    
    
    
    
    
    
    
    // MARK: - Subviews
    
//    virtual std::list<WeakPtr>& subviews() { return _subviews; }
    
    virtual void subviews(const Views& x) {
        _subviews = x;
        layoutNeeded(true);
    }
    
    virtual ViewsIter subviews() {
        return _subviews.begin();
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
    
    template <typename T, typename ...T_Args>
    std::shared_ptr<T> subviewCreate(T_Args&&... args) {
        auto view = std::make_shared<T>(std::forward<T_Args>(args)...);
        _subviews.push_back(view);
        layoutNeeded(true);
        return view;
    }
    
    void subviewAdd(Ptr view) {
        _subviews.push_back(view);
    }
    
//    for (auto it=_subviews.begin(); it!=_subviews.end();) {
//        Ptr subview = (*it).lock();
//        if (!subview) {
//            // Prune and continue
//            it = _subviews.erase(it);
//            continue;
//        }
//        
//        subview->layoutTree(win, _TState.origin()+subview->origin());
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
//            subview->layoutTree(win, _TState.origin()+subview->origin());
//            it++;
//        }
//        
//    }
    
protected:
    template <typename X, typename Y>
    void _setForce(X& x, const Y& y) {
        x = y;
        View::drawNeeded(true);
    }
    
    template <typename X, typename Y>
    void _set(X& x, const Y& y) {
        if (x == y) return;
        x = y;
        View::drawNeeded(true);
    }
    
protected:
    class _GraphicsStateSwapper {
    public:
        _GraphicsStateSwapper() {}
        _GraphicsStateSwapper(GraphicsState& dst, const GraphicsState& val) : _dst(&dst), _val(val) {
            std::swap(*_dst, _val);
        }
        
        _GraphicsStateSwapper(const _GraphicsStateSwapper& x) = delete;
        _GraphicsStateSwapper(_GraphicsStateSwapper&& x) = delete;
        
        ~_GraphicsStateSwapper() {
            if (_dst) {
                std::swap(*_dst, _val);
            }
        }
    
    private:
        GraphicsState* _dst = nullptr;
        GraphicsState _val;
    };
    
private:
    WINDOW* _gstateWindow() const;
    
    static inline ColorPalette _Colors;
    static inline GraphicsState _GState;
    
    std::list<WeakPtr> _subviews;
    Point _origin;
    Size _size;
    bool _visible = true;
    bool _interaction = true;
    bool _layoutNeeded = true;
    bool _drawNeeded = true;
    bool _tracking = false;
    bool _trackStop = false;
    Event _eventCurrent;
    HitTestExpand _hitTestExpand;
    std::optional<Color> _borderColor;
};

using ViewPtr = std::shared_ptr<View>;

} // namespace UI
