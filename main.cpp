#include <iostream>
#include <unistd.h>
#include <vector>
#include <cassert>
#include "lib/ncurses/include/curses.h"
#include "lib/ncurses/include/panel.h"
#include "lib/libgit2/include/git2.h"
#include "xterm-256color.h"

class Window {
public:
    static void Redraw() {
        ::update_panels();
        ::doupdate();
    }
    
    Window() {
        _state.window = ::newwin(0, 0, 0, 0);
        assert(_state.window);
        
        ::keypad(_state.window, true);
        ::meta(_state.window, true);
    }
    
    Window(WINDOW* window) {
        _state.window = window;
        ::keypad(_state.window, true);
        ::meta(_state.window, true);
    }
    
    // Move constructor: use move assignment operator
    Window(Window&& x) {
        _state = std::move(x._state);
        x._state = {};    
    }
    
    void setSize(int w, int h) {
        ::wresize(*this, h, w);
    }
    
    void setPosition(int x, int y) {
        ::mvwin(*this, y, x);
    }
    
    void drawBox() {
        ::box(*this, 0, 0);
    }
    
    void erase() {
        ::werase(*this);
    }
    
    int getChar() const { return ::wgetch(*this); }
    operator WINDOW*() const { return _state.window; }
    
private:
    struct {
        const Window* parent = nullptr;
        WINDOW* window = nullptr;
    } _state = {};
};

class Panel : public Window {
public:
    Panel() {
        _state.panel = ::new_panel(*this);
        assert(_state.panel);
    }
    
    // Move constructor: use move assignment operator
    Panel(Panel&& x) : Window(std::move(x)) {
        _state = std::move(x._state);
        x._state = {};    
    }
    
    ~Panel() {
        ::del_panel(*this);
    }
    
    void setPosition(int x, int y) {
        ::move_panel(*this, y, x);
    }
    
    operator PANEL*() const { return _state.panel; }
    
private:
    struct {
        PANEL* panel = nullptr;
    } _state = {};
};

int main(int argc, const char* argv[]) {
    // Default linux installs may not contain the /usr/share/terminfo database,
    // so provide a default terminfo that usually works.
    nc_set_default_terminfo(xterm_256color, sizeof(xterm_256color));
    
    // Override the terminfo 'kmous' and 'XM' properties
    //   kmous = the prefix used to detect/parse mouse events
    //   XM    = the escape string used to enable mouse events (1006=SGR 1006 mouse
    //           event mode; 1003=report mouse-moved events in addition to clicks)
    setenv("TERM_KMOUS", "\x1b[<", true);
    setenv("TERM_XM", "\x1b[?1006;1003%?%p1%{1}%=%th%el%;", true);
    
    ::initscr();
    ::noecho();
    ::cbreak();
    
    // Hide cursor
    curs_set(0);
    
    Window rootWindow(::stdscr);
    
    std::vector<Panel> panels;
    for (int i=0; i<5; i++) {
        Panel& panel = panels.emplace_back();
        panel.setSize(5, 5);
        panel.setPosition(3+i, 3+i);
        panel.drawBox();
    }
    
    mousemask(ALL_MOUSE_EVENTS | REPORT_MOUSE_POSITION, NULL);
    mouseinterval(0);
    for (int i=0;; i++) {
        Window::Redraw();
        int key = rootWindow.getChar();
        if (key == KEY_MOUSE) {
            MEVENT mouseEvent = {};
            int ir = getmouse(&mouseEvent);
            if (ir != OK) continue;
            try {
                panels[0].setPosition(mouseEvent.x, mouseEvent.y);
                
            } catch (...) {}
        
        } else if (key == KEY_RESIZE) {
        }
    }
    return 0;
}
