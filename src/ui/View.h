#pragma once
#include <optional>
#include "Color.h"
#include "lib/toastbox/Defer.h"

namespace UI {

class Window;

class View {
public:
    using Ptr = std::shared_ptr<View>;
    using WeakPtr = std::weak_ptr<View>;
    
    class Attr {
    public:
        Attr() {}
        Attr(WINDOW* win, int attr) : _s({.win=win, .attr=attr}) {
            assert(win);
//            if (rand() % 2) {
//                wattron(*_s.win, A_REVERSE);
//            } else {
//                wattroff(*_s.win, A_REVERSE);
//            }
            // MARK: - Drawing
            wattron(_s.win, _s.attr);
        }
        
        Attr(const Attr& x) = delete;
        Attr(Attr&& x) { std::swap(_s, x._s); }
        Attr& operator =(Attr&& x) { std::swap(_s, x._s); return *this; }
        
        ~Attr() {
            if (_s.win) {
                wattroff(_s.win, _s.attr);
            }
        }
    
    private:
        struct {
            WINDOW* win = nullptr;
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
    
    virtual ~View() = default;
    
    virtual bool hitTest(const Point& p) const {
        Rect f = frame();
        f.origin.x -= _hitTestExpand.l;
        f.size.x   += _hitTestExpand.l;
        
        f.size.x   += _hitTestExpand.r;
        
        f.origin.y -= _hitTestExpand.t;
        f.size.y   += _hitTestExpand.t;
        
        f.size.y   += _hitTestExpand.b;
        return HitTest(f, p);
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
    
    virtual Point treeOrigin() const { return _treeState.origin(); }
    
    // MARK: - Attributes
    virtual Attr attr(int attr) const { return Attr(_drawWin(), attr); }
    
    // MARK: - Drawing
    virtual void drawRect() const {
        drawRect({{}, size()});
    }
    
    virtual void drawRect(const Rect& rect) const {
        const Rect r = {_drawOrigin()+rect.origin, rect.size};
        const int x1 = r.origin.x;
        const int y1 = r.origin.y;
        const int x2 = r.origin.x+r.size.x-1;
        const int y2 = r.origin.y+r.size.y-1;
        mvwhline(_drawWin(), y1, x1, 0, r.size.x);
        mvwhline(_drawWin(), y2, x1, 0, r.size.x);
        mvwvline(_drawWin(), y1, x1, 0, r.size.y);
        mvwvline(_drawWin(), y1, x2, 0, r.size.y);
        mvwaddch(_drawWin(), y1, x1, ACS_ULCORNER);
        mvwaddch(_drawWin(), y2, x1, ACS_LLCORNER);
        mvwaddch(_drawWin(), y1, x2, ACS_URCORNER);
        mvwaddch(_drawWin(), y2, x2, ACS_LRCORNER);
    }
    
    virtual void drawLineHoriz(const Point& p, int len, chtype ch=0) const {
        const Point off = _drawOrigin();
        mvwhline(_drawWin(), off.y+p.y, off.x+p.x, ch, len);
    }
    
    virtual void drawLineVert(const Point& p, int len, chtype ch=0) const {
        const Point off = _drawOrigin();
        mvwvline(_drawWin(), off.y+p.y, off.x+p.x, ch, len);
    }
    
    virtual void drawText(const Point& p, const char* txt) const {
        const Point off = _drawOrigin();
        mvwprintw(_drawWin(), off.y+p.y, off.x+p.x, "%s", txt);
    }
    
    virtual void drawText(const Point& p, int widthMax, const char* txt) const {
        const Point off = _drawOrigin();
        widthMax = std::max(0, widthMax);
        
        std::string str = txt;
        auto it = UTF8::NextN(str.begin(), str.end(), widthMax);
        str.resize(std::distance(str.begin(), it));
        mvwprintw(_drawWin(), off.y+p.y, off.x+p.x, "%s", str.c_str());
    }
    
    template <typename ...T_Args>
    void drawText(const Point& p, const char* fmt, T_Args&&... args) const {
        const Point off = _drawOrigin();
        mvwprintw(_drawWin(), off.y+p.y, off.x+p.x, fmt, std::forward<T_Args>(args)...);
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
    virtual bool handleEvent(const Window& win, const Event& ev) { return false; }
    
    virtual void track(const Window& win, const Event& ev) {
        _tracking = true;
        Defer(_tracking = false); // Exception safety
        Defer(_trackStop = false); // Exception safety
        
        do {
            refresh(win);
            
            _eventCurrent = UI::NextEvent();
            Defer(_eventCurrent = {}); // Exception safety
            
            handleEventTree(win, _treeState.origin(), _eventCurrent);
        } while (!_trackStop);
    }
    
    // Signal track() to return
    virtual void trackStop() {
        _trackStop = true;
    }
    
    virtual bool tracking() const { return _tracking; }
    
    // MARK: - Subviews
    template <typename T, typename ...T_Args>
    std::shared_ptr<T> createSubview(T_Args&&... args) {
        auto view = std::make_shared<T>(std::forward<T_Args>(args)...);
        _subviews.push_back(view);
        layoutNeeded(true);
        return view;
    }
    
    virtual std::list<WeakPtr>& subviews() { return _subviews; }
    
    virtual void layoutTree(const Window& win, const Point& orig) {
        if (!visible()) return;
        _TreeState treeState(_treeState, win, orig+origin());
        
        if (layoutNeeded()) {
            layout();
            layoutNeeded(false);
        }
        
        for (auto it=_subviews.begin(); it!=_subviews.end();) {
            Ptr view = (*it).lock();
            if (!view) {
                // Prune and continue
                it = _subviews.erase(it);
                continue;
            }
            
            view->layoutTree(win, _treeState.origin());
            it++;
        }
    }
    
    virtual void drawTree(const Window& win, const Point& orig) {
        if (!visible()) return;
        _TreeState treeState(_treeState, win, orig+origin());
        
        // If the window was erased during this draw cycle, we need to redraw.
        // _winErased() has to be implemented out-of-line because:
        // 
        //   Window.h includes View.h
        //   ∴ View.h can't include Window.h
        //   ∴ View has to forward declare Window
        //   ∴ we can't call Window functions in View.h
        //
        if (drawNeeded() || _winErased(win)) {
            drawBackground();
            draw();
            drawBorder();
            drawNeeded(false);
        }
        
        for (auto it=_subviews.begin(); it!=_subviews.end();) {
            Ptr view = (*it).lock();
            if (!view) {
                // Prune and continue
                it = _subviews.erase(it);
                continue;
            }
            
            view->drawTree(win, _treeState.origin());
            it++;
        }
    }
    
    virtual bool handleEventTree(const Window& win, const Point& orig, const Event& ev) {
        if (!visible()) return false;
        if (!interaction()) return false;
        _TreeState treeState(_treeState, win, orig+origin());
        
        // Let the subviews handle the event first
        for (auto it=_subviews.begin(); it!=_subviews.end();) {
            Ptr view = (*it).lock();
            if (!view) {
                // Prune and continue
                it = _subviews.erase(it);
                continue;
            }
            
            if (view->handleEventTree(win, _treeState.origin(), ev)) return true;
            it++;
        }
        
        // None of the subviews wanted the event; let the view itself handle it
        if (handleEvent(win, ev)) return true;
        return false;
    }
    
    virtual void refresh(const Window& win) {
        layoutTree(win, {});
        drawTree(win, {});
        CursorState::Draw();
        ::update_panels();
        ::refresh();
    }
    
    virtual const Event& eventCurrent() const { return _eventCurrent; }
    
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
    
private:
    class _TreeState {
    public:
        _TreeState() {}
        
        _TreeState(_TreeState& x, const Window& win, const Point& origin) :
        _s{.prev=&x, .win=&win, .origin=origin} {
            std::swap(_s, _s.prev->_s);
        }
        
        _TreeState(const _TreeState& x) = delete;
        _TreeState(_TreeState&& x) = delete;
        
        ~_TreeState() {
            if (_s.prev) {
                std::swap(_s, _s.prev->_s);
            }
        }
        
        operator bool() const { return _s.prev; }
        const Window* win() const { return _s.win; }
        const Point& origin() const { return _s.origin; }
    
    private:
        struct {
            _TreeState* prev = nullptr;
            const Window* win = nullptr;
            Point origin;
        } _s;
    };
    
    bool _winErased(const Window& win) const;
    WINDOW* _drawWin() const;
    Point _drawOrigin() const;
    
    static inline ColorPalette _Colors;
    
    _TreeState _treeState;
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
