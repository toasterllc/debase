#pragma once
#define NCURSES_WIDECHAR 1
#include <vector>
#include "lib/ncurses/include/curses.h"
#include "lib/ncurses/include/panel.h"
#include "Bitfield.h"
#include "UI.h"
#include "CursorState.h"
#include "UTF8.h"

namespace UI {

class View;
using ViewPtr = std::shared_ptr<View>;

class Window {
public:
    class Attr {
    public:
        Attr() {}
        Attr(const Window& win, int attr) : _s({.win=&win, .attr=attr}) {
//            if (rand() % 2) {
//                wattron(*_s.win, A_REVERSE);
//            } else {
//                wattroff(*_s.win, A_REVERSE);
//            }
            
            wattron(*_s.win, _s.attr);
        }
        
        Attr(const Attr& x) = delete;
        Attr(Attr&& x) { std::swap(_s, x._s); }
        Attr& operator =(Attr&& x) { std::swap(_s, x._s); return *this; }
        
        ~Attr() {
            if (_s.win) {
                wattroff(*_s.win, _s.attr);
            }
        }
    
    private:
        struct {
            const Window* win = nullptr;
            int attr = 0;
        } _s;
    };
    
    Window() {}
    
    Window(WINDOW* win) : _s{.win = win} {
        if (!_s.win) {
            _s.win = ::newwin(0, 0, 0, 0);
            assert(_s.win);
        }
        
        ::keypad(_s.win, true);
        ::meta(_s.win, true);
    }
    
    Window(const Window& x) = delete;
    Window(Window&& x) {
        std::swap(_s, x._s);
    }
    
    virtual ~Window() {
        if (_s.win) {
            ::delwin(_s.win);
        }
    }
    
    void drawBorder() const {
        ::box(*this, 0, 0);
    }
    
    void drawLineHoriz(const Point& p, int len, chtype ch=0) const {
        mvwhline(*this, p.y, p.x, ch, len);
    }
    
    void drawLineVert(const Point& p, int len, chtype ch=0) const {
        mvwvline(*this, p.y, p.x, ch, len);
    }
    
    void drawRect(const Rect& rect) const {
        const int x1 = rect.origin.x;
        const int y1 = rect.origin.y;
        const int x2 = rect.origin.x+rect.size.x-1;
        const int y2 = rect.origin.y+rect.size.y-1;
        mvwhline(*this, y1, x1, 0, rect.size.x);
        mvwhline(*this, y2, x1, 0, rect.size.x);
        mvwvline(*this, y1, x1, 0, rect.size.y);
        mvwvline(*this, y1, x2, 0, rect.size.y);
        mvwaddch(*this, y1, x1, ACS_ULCORNER);
        mvwaddch(*this, y2, x1, ACS_LLCORNER);
        mvwaddch(*this, y1, x2, ACS_URCORNER);
        mvwaddch(*this, y2, x2, ACS_LRCORNER);
    }
    
    void drawText(const Point& p, const char* txt) const {
        mvwprintw(*this, p.y, p.x, "%s", txt);
    }
    
    void drawText(const Point& p, int maxLen, const char* txt) const {
        std::string str = txt;
        auto it = UTF8::NextN(str.begin(), str.end(), maxLen);
        str.resize(std::distance(str.begin(), it));
        mvwprintw(*this, p.y, p.x, "%s", str.c_str());
    }
    
    template <typename ...T_Args>
    void drawText(const Point& p, const char* fmt, T_Args&&... args) const {
        mvwprintw(*this, p.y, p.x, fmt, std::forward<T_Args>(args)...);
    }
    
    Point origin() const { return { getbegx(_s.win), getbegy(_s.win) }; }
    
    Size size() const { return { getmaxx(_s.win), getmaxy(_s.win) }; }
    void size(const Size& s) {
        // Short-circuit if the size hasn't changed
        if (s == size()) return;
        ::wresize(*this, std::max(1, s.y), std::max(1, s.x));
        layoutNeeded(true);
        drawNeeded(true);
    }
    
    virtual Size sizeIntrinsic(Size constraint) { return size(); }
    
    Rect bounds() const {
        return Rect{ .size = size() };
    }
    
    Rect frame() const {
        return Rect{
            .origin = origin(),
            .size   = size(),
        };
    }
    
    bool hitTest(const Point& p) const {
        return HitTest(frame(), p);
    }
    
    // convert(): convert a point from the coorindate system of the parent window to the coordinate system of `this`
    Point convert(const Point& p) const {
        return p-origin();
    }
    
    // convert(): convert an event from the coorindate system of the parent window to the coordinate system of `this`
    Event convert(const Event& p) const {
        Event r = p;
        if (r.type == Event::Type::Mouse) {
            r.mouse.point = convert(r.mouse.point);
        }
        return r;
    }
    
    Attr attr(int attr) const {
        return Attr(*this, attr);
    }
    
    Event nextEvent() const {
        // Wait for another mouse event
        for (;;) {
            int ch = ::wgetch(*this);
            if (ch == ERR) continue;
            
            Event ev = {
                .type = (Event::Type)ch,
                .time = std::chrono::steady_clock::now(),
            };
            switch (ev.type) {
            case Event::Type::Mouse: {
                MEVENT mouse = {};
                int ir = ::getmouse(&mouse);
                if (ir != OK) continue;
                ev.mouse = {
                    .point = {mouse.x, mouse.y},
                    .bstate = mouse.bstate,
                };
                break;
            }
            
            case Event::Type::WindowResize: {
                throw WindowResize();
            }
            
            case Event::Type::KeyCtrlC:
            case Event::Type::KeyCtrlD: {
                throw ExitRequest();
            }
            
            default: {
                break;
            }}
            
            ev = convert(ev);
            return ev;
        }
    }
    
    void refresh() {
        layoutTree();
        drawTree();
        CursorState::Draw();
        ::update_panels();
        ::refresh();
    }
    
    virtual bool layoutNeeded() const { return _s.layoutNeeded || _s.sizePrev!=size(); }
    virtual void layoutNeeded(bool x) { _s.layoutNeeded = x; }
    virtual void layout() {}
    
    virtual bool drawNeeded() const { return _s.drawNeeded; }
    virtual void drawNeeded(bool x) { _s.drawNeeded = x; }
    virtual void draw() {}
    
    virtual bool handleEvent(const Event& ev) { return false; }
    
    virtual void track() {
        do {
            refresh();
            _s.eventCurrent = nextEvent();
            handleEventTree(_s.eventCurrent);
            _s.eventCurrent = {};
        } while (!_s.trackStop);
        
        _s.trackStop = false;
    }
    
    // Call to trigger track() to return
    virtual void trackStop() {
        _s.trackStop = true;
    }
    
    template <typename T, typename ...T_Args>
    std::shared_ptr<T> createSubview(T_Args&&... args) {
        auto view = std::make_shared<T>(std::forward<T_Args>(args)...);
        _s.subviews.push_back(view);
        return view;
    }
    
    virtual std::vector<ViewPtr>& subviews() { return _s.subviews; }
    
    void layoutTree();
    void drawTree();
    bool handleEventTree(const Event& ev);
    
    // eraseNeeded(): sets whether the window should be erased the next time it's drawn
    void eraseNeeded(bool x) {
        _s.eraseNeeded = x;
        if (_s.eraseNeeded) {
            drawNeeded(true);
        }
    }
    
    // erased(): whether the window was erased during this draw cycle
    bool erased() const { return _s.erased; }
    
    Window& operator =(Window&& x) { std::swap(_s, x._s); return *this; }
    
    const Event& eventCurrent() const { return _s.eventCurrent; }
    operator WINDOW*() const { return _s.win; }
    
protected:
    template <typename X, typename Y>
    void _setAlways(X& x, const Y& y) {
        x = y;
        drawNeeded(true);
    }
    
    template <typename X, typename Y>
    void _set(X& x, const Y& y) {
        if (x == y) return;
        x = y;
        drawNeeded(true);
    }
    
private:
    struct {
        WINDOW* win = nullptr;
        Size sizePrev;
        Event eventCurrent;
        bool layoutNeeded = true;
        bool drawNeeded = true;
        // eraseNeeded: tracks whether the window needs to be erased the next time it's drawn
        bool eraseNeeded = true;
        // erased: tracks whether the window was erased in this draw cycle
        bool erased = false;
        bool trackStop = false;
        std::vector<ViewPtr> subviews;
    } _s;
};

using WindowPtr = std::shared_ptr<Window>;

} // namespace UI
