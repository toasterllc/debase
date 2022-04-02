#pragma once
#define NCURSES_WIDECHAR 1
#include "lib/ncurses/include/curses.h"
#include "lib/ncurses/include/panel.h"
#include "Bitfield.h"
#include "UI.h"
#include "CursorState.h"

namespace UI {

class Window {
public:
    class Attr {
    public:
        Attr() {}
        Attr(const Window& win, int attr) : _s({.win=&win, .attr=attr}) {
            wattron(*_s.win, _s.attr);
        }
        
        Attr(const Attr& x) = delete;
        Attr(Attr&& x) { std::swap(_s, x._s); }
        Attr& operator=(Attr&& x) { std::swap(_s, x._s); return *this; }
        
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
    
    ~Window() {
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
        const int x1 = rect.point.x;
        const int y1 = rect.point.y;
        const int x2 = rect.point.x+rect.size.x-1;
        const int y2 = rect.point.y+rect.size.y-1;
        mvwhline(*this, y1, x1, 0, rect.size.x);
        mvwhline(*this, y2, x1, 0, rect.size.x);
        mvwvline(*this, y1, x1, 0, rect.size.y);
        mvwvline(*this, y1, x2, 0, rect.size.y);
        mvwaddch(*this, y1, x1, ACS_ULCORNER);
        mvwaddch(*this, y2, x1, ACS_LLCORNER);
        mvwaddch(*this, y1, x2, ACS_URCORNER);
        mvwaddch(*this, y2, x2, ACS_LRCORNER);
    }
    
    template <typename ...T_Args>
    void drawText(const Point& p, const char* fmt, T_Args&&... args) const {
        mvwprintw(*this, p.y, p.x, fmt, std::forward<T_Args>(args)...);
    }
    
    void erase() const {
        ::werase(*this);
    }
    
    Point position() const { return { getbegx(_s.win), getbegy(_s.win) }; }
    
    Size size() const { return { getmaxx(_s.win), getmaxy(_s.win) }; }
    void size(const Size& s) {
        // Short-circuit if the size hasn't changed
        // We need to compare against the last size we set (_s.sizePrev) *and* our current size(),
        // because ncurses can change our size underneath us, due to terminal resizing clipping
        // the window.
        if (s==_s.sizePrev && s==size()) return;
        ::wresize(*this, std::max(1, s.y), std::max(1, s.x));
        _s.sizePrev = s;
        layoutNeeded = true;
        drawNeeded = true;
    }
    
    virtual Size sizeIntrinsic() { return size(); }
    
    Rect bounds() const {
        return Rect{ .size = size() };
    }
    
    Rect frame() const {
        return Rect{
            .point = position(),
            .size  = size(),
        };
    }
    
    bool hitTest(const Point& p) const {
        return HitTest(frame(), p);
    }
    
    // convert(): convert a point from the coorindate system of the parent window to the coordinate system of `this`
    Point convert(const Point& p) const {
        Point r = p;
        r -= frame().point;
        return r;
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
    
    virtual bool handleEvent(const Event& ev) { return false; }
    
    void refresh() {
        layout();
        draw();
        CursorState::Draw();
        ::update_panels();
        ::refresh();
    }
    
    virtual void track() {
        for (;;) {
            refresh();
            
            _s.eventCurrent = nextEvent();
            bool handled = handleEvent(_s.eventCurrent);
            _s.eventCurrent = {};
            
            // Continue until an event isn't handled
            if (!handled) break;
        }
    }
    
    bool layoutNeeded = true;
    virtual void layout() {
        assert(layoutNeeded); // For debugging unnecessary layout
        layoutNeeded = false;
    }
    
    bool drawNeeded = true;
    virtual void draw() {
        assert(drawNeeded); // For debugging unnecessary drawing
        drawNeeded = false;
    }
    
    Window& operator =(Window&& x) { std::swap(_s, x._s); return *this; }
    
    const Event& eventCurrent() const { return _s.eventCurrent; }
    operator WINDOW*() const { return _s.win; }
    
private:
    struct {
        WINDOW* win = nullptr;
        Size sizePrev;
        Event eventCurrent;
    } _s;
};

using WindowPtr = std::shared_ptr<Window>;

} // namespace UI
