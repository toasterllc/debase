#include <iostream>
#include <unistd.h>
#include <vector>
#include <cassert>
#include <iostream>
#include <sstream>
#include <iomanip>
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

static std::string strFromGitOid(const git_oid& oid) {
    char str[16];
    sprintf(str, "%02x%02x%02x%02x", oid.id[0], oid.id[1], oid.id[2], oid.id[3]);
    return str;
}

static std::string strFromGitTime(git_time_t t) {
    const std::time_t tmp = t;
    std::tm tm;
    gmtime_r(&tmp, &tm);
    
    std::stringstream ss;
    ss << std::put_time(&tm, "%c");
    return ss.str();
}

int main(int argc, const char* argv[]) {
    git_libgit2_init();
    git_repository* repo = nullptr;
    int error = git_repository_open(&repo, ".");
    assert(!error);
    
    git_revwalk* walker;
    error = git_revwalk_new(&walker, repo);
    assert(!error);
    error = git_revwalk_push_range(walker, "HEAD~20..HEAD");
    assert(!error);
    
    git_oid oid;
    while (!git_revwalk_next(&oid, walker)) {
        git_commit* commit = nullptr;
        error = git_commit_lookup(&commit, repo, &oid);
        assert(!error);
        
        const git_signature* author = git_commit_author(commit);
        const char* message         = git_commit_message(commit);
        const git_time_t time       = git_commit_time(commit);
        
        printf("%s\n", strFromGitOid(oid).c_str());
        printf("%s <%s>\n", author->name, author->email);
        
//        git_date_parse();
//        git_date_rfc2822_fmt();
        
        printf("%s\n", strFromGitTime(time).c_str());
        printf("%s\n", message);
        printf("\n");
    }
    
    return 0;
    
    
    
    
//    // Default linux installs may not contain the /usr/share/terminfo database,
//    // so provide a default terminfo that usually works.
//    nc_set_default_terminfo(xterm_256color, sizeof(xterm_256color));
//    
//    // Override the terminfo 'kmous' and 'XM' properties
//    //   kmous = the prefix used to detect/parse mouse events
//    //   XM    = the escape string used to enable mouse events (1006=SGR 1006 mouse
//    //           event mode; 1003=report mouse-moved events in addition to clicks)
//    setenv("TERM_KMOUS", "\x1b[<", true);
//    setenv("TERM_XM", "\x1b[?1006;1003%?%p1%{1}%=%th%el%;", true);
//    
//    ::initscr();
//    ::noecho();
//    ::cbreak();
//    
//    // Hide cursor
//    curs_set(0);
//    
//    Window rootWindow(::stdscr);
//    
//    std::vector<Panel> panels;
//    for (int i=0; i<5; i++) {
//        Panel& panel = panels.emplace_back();
//        panel.setSize(5, 5);
//        panel.setPosition(3+i, 3+i);
//        panel.drawBox();
//    }
//    
//    mousemask(ALL_MOUSE_EVENTS | REPORT_MOUSE_POSITION, NULL);
//    mouseinterval(0);
//    for (int i=0;; i++) {
//        Window::Redraw();
//        int key = rootWindow.getChar();
//        if (key == KEY_MOUSE) {
//            MEVENT mouseEvent = {};
//            int ir = getmouse(&mouseEvent);
//            if (ir != OK) continue;
//            try {
//                panels[0].setPosition(mouseEvent.x, mouseEvent.y);
//                
//            } catch (...) {}
//        
//        } else if (key == KEY_RESIZE) {
//        }
//    }
//    return 0;
}
