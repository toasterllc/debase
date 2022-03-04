#include <iostream>
#include <unistd.h>
#include <vector>
#include <cassert>
#include <iostream>
#include <sstream>
#include <iomanip>
#include "lib/ncurses/c++/cursesp.h"
//#include "lib/ncurses/include/curses.h"
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
    
    void drawText(int x, int y, const char* fmt, ...)
    {
        va_list args;
        va_start(args, fmt);
        int result = ::wmove(*this, y, x);
//        assert(!result);
        result = ::vw_printw(*this, fmt, args);
//        assert(!result);
        va_end(args);
//        assert(!result);
    }

    void drawText(int x, int y, const char* fmt, va_list args)
    {
        int result = ::wmove(*this, y, x);
//        assert(!result);
        result = ::vw_printw(*this, fmt, args);
//        assert(!result);
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
    
    bool visible() const {
        return !::panel_hidden(*this);
    }
    
    void setVisible(bool v) {
        if (v) ::show_panel(*this);
        else ::hide_panel(*this);
    }
    
    void orderFront() {
        ::top_panel(*this);
    }
    
    void orderBack() {
        ::bottom_panel(*this);
    }
    
    operator PANEL*() const { return _state.panel; }
    
private:
    struct {
        PANEL* panel = nullptr;
    } _state = {};
};

static std::string _StrFromGitOid(const git_oid& oid) {
    char str[16];
    sprintf(str, "%02x%02x%02x%02x", oid.id[0], oid.id[1], oid.id[2], oid.id[3]);
    str[7] = 0;
    return str;
}

static std::string _StrFromGitTime(git_time_t t) {
    const std::time_t tmp = t;
    std::tm tm;
    localtime_r(&tmp, &tm);
    
    std::stringstream ss;
    ss << std::put_time(&tm, "%c");
    return ss.str();
}

static void _TrackSelection(Window& rootWindow, Panel& selectionRect, MEVENT mouseDownEvent) {
    for (;;) {
        Window::Redraw();
        int key = rootWindow.getChar();
        if (key != KEY_MOUSE) continue;
        
        MEVENT mouse = {};
        int ir = getmouse(&mouse);
        if (ir != OK) continue;
        
        int x = std::min(mouseDownEvent.x, mouse.x);
        int y = std::min(mouseDownEvent.y, mouse.y);
        int w = std::abs(mouseDownEvent.x - mouse.x);
        int h = std::abs(mouseDownEvent.y - mouse.y);
        
        selectionRect.setPosition(x, y);
        selectionRect.setSize(w, h);
        selectionRect.erase();
        selectionRect.drawBox();
        
        // Make selectionRect visible _after_ drawing, otherwise we draw
        // selectionRect's old state for a single frame
        if (!selectionRect.visible()) {
            selectionRect.setVisible(true);
            selectionRect.orderBack();
        }
        
        if (mouse.bstate & BUTTON1_RELEASED) break;
    }
    
    selectionRect.setVisible(false);
}

int main(int argc, const char* argv[]) {
    // Initialize ncurses
    {
        // Default linux installs may not contain the /usr/share/terminfo database,
        // so provide a fallback terminfo that usually works.
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
    }
    
    Window rootWindow(::stdscr);
    Panel selectionRect;
//    selectionRect.setVisible(false);
    
    std::vector<Panel> panels;
    
    
    
    // Create panels for each commit
    {
        git_libgit2_init();
        git_repository* repo = nullptr;
        int error = git_repository_open(&repo, ".");
        assert(!error);
        
        git_revwalk* walker = nullptr;
        error = git_revwalk_new(&walker, repo);
        assert(!error);
        error = git_revwalk_push_range(walker, "HEAD~4..HEAD");
        assert(!error);
        
        int count = 0;
        git_oid oid;
        while (!git_revwalk_next(&oid, walker)) {
            git_commit* commit = nullptr;
            error = git_commit_lookup(&commit, repo, &oid);
            assert(!error);
            
            const git_signature* author = git_commit_author(commit);
            const char* message         = git_commit_message(commit);
            const git_time_t time       = git_commit_time(commit);
            
//            printf("%s\n", _StrFromGitOid(oid).c_str());
//            printf("%s <%s>\n", author->name, author->email);
//            
//    //        git_date_parse();
//    //        git_date_rfc2822_fmt();
//            
//            printf("%s\n", _StrFromGitTime(time).c_str());
//            printf("%s\n", message);
//            printf("\n");
            
            Panel& panel = panels.emplace_back();
            panel.setSize(40, 5);
            panel.setPosition(4, 6*count);
            
            panel.drawText(2, 3, "%s", message);
            panel.drawBox();
            panel.drawText(2, 0, " %s @ %s ", _StrFromGitOid(oid).c_str(), _StrFromGitTime(time).c_str());
//            panel.drawText(2, 0, " %s ", _StrFromGitOid(oid).c_str());
            panel.drawText(2, 1, "%s", author->name, _StrFromGitTime(time).c_str());
            
            count++;
            
//            panel.drawBox();
//            panel.drawText(2, 1, "<%s>", author->email);
//            panel.drawText(2, 1, "%s <%s>", author->name, author->email);
        }
    }
    
    
    
    
    
    
    
    
//    for (int i=0; i<5; i++) {
//        Panel& panel = panels.emplace_back();
//        panel.setSize(5, 5);
//        panel.setPosition(3+i, 3+i);
//        panel.drawBox();
//    }
    
    mousemask(ALL_MOUSE_EVENTS | REPORT_MOUSE_POSITION, NULL);
    mouseinterval(0);
    for (int i=0;; i++) {
        Window::Redraw();
        int key = rootWindow.getChar();
        if (key == KEY_MOUSE) {
            MEVENT mouse = {};
            int ir = getmouse(&mouse);
            if (ir != OK) continue;
            if (mouse.bstate & BUTTON1_PRESSED) {
                _TrackSelection(rootWindow, selectionRect, mouse);
            }
        } else if (key == KEY_RESIZE) {
        }
    }
    return 0;
}
