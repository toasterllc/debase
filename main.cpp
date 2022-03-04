#include <iostream>
#include <unistd.h>
#include <vector>
#include <cassert>
#include <iostream>
#include <sstream>
#include <iomanip>
#include "lib/ncurses/c++/cursesp.h"
#include "lib/ncurses/include/curses.h"
#include "lib/ncurses/include/panel.h"
#include "lib/libgit2/include/git2.h"
#include "xterm-256color.h"
#include "lib/Toastbox/RuntimeError.h"

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
        result = ::vw_printw(*this, fmt, args);
        va_end(args);
    }

    void drawText(int x, int y, const char* fmt, va_list args)
    {
        int result = ::wmove(*this, y, x);
        result = ::vw_printw(*this, fmt, args);
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

template <typename T, auto& T_Deleter>
struct RefCounted : public std::shared_ptr<T> {
    RefCounted() : std::shared_ptr<T>() {}
    RefCounted(const T& a) : std::shared_ptr<T>(new T(a), _Deleter) {}
private:
    static void _Deleter(T* t) { T_Deleter(*t); }
};

using Repo = RefCounted<git_repository*, git_repository_free>;
using RevWalk = RefCounted<git_revwalk*, git_revwalk_free>;
using Commit = RefCounted<git_commit*, git_commit_free>;

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

class CommitPanel : public Panel {
public:
    CommitPanel(Commit commit) : _commit(commit) {
        setSize(40, 5);
        _drawNeeded = true;
    }
    
    void setSelected(bool x) {
        if (_selected == x) return;
        _selected = x;
        _drawNeeded = true;
    }
    
    void draw() {
        const git_oid* oid          = git_commit_id(*_commit);
        const git_signature* author = git_commit_author(*_commit);
        const char* message         = git_commit_message(*_commit);
        const git_time_t time       = git_commit_time(*_commit);
        
        erase();
        drawText(2, 3, "%s", message);
        drawBox();
        drawText(2, 0, " %s @ %s ", _StrFromGitOid(*oid).c_str(), _StrFromGitTime(time).c_str());
        drawText(2, 1, "%s", author->name, _StrFromGitTime(time).c_str());
        
        _drawNeeded = false;
    }
    
    void drawIfNeeded() {
        if (_drawNeeded) {
            draw();
        }
    }
    
private:
    Commit _commit;
    bool _selected = false;
    bool _drawNeeded = false;
};

static Repo _RepoOpen() {
    git_repository* r = nullptr;
    int ir = git_repository_open(&r, ".");
    if (ir) throw Toastbox::RuntimeError("git_repository_open failed: %s", git_error_last()->message);
    return r;
}

static RevWalk _RevWalkCreate(Repo repo) {
    git_revwalk* w = nullptr;
    int ir = git_revwalk_new(&w, *repo);
    if (ir) throw Toastbox::RuntimeError("git_revwalk_new failed: %s", git_error_last()->message);
    return w;
}

static Commit _CommitLookup(Repo repo, const git_oid& oid) {
    git_commit* c = nullptr;
    int ir = git_commit_lookup(&c, *repo, &oid);
    if (ir) throw Toastbox::RuntimeError("git_commit_lookup failed: %s", git_error_last()->message);
    return c;
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
    // Init ncurses
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
    
    // Init libgit2
    {
        git_libgit2_init();
    }
    
    Window rootWindow(::stdscr);
    Panel selectionRect;
    std::vector<CommitPanel> commitPanels;
    
    selectionRect.setVisible(false);
    
    // Create panels for each commit
    {
        Repo repo = _RepoOpen();
        RevWalk walk = _RevWalkCreate(repo);
        
        int ir = git_revwalk_push_range(*walk, "HEAD~5..HEAD");
        if (ir) throw Toastbox::RuntimeError("git_revwalk_push_range failed: %s", git_error_last()->message);
        
        int count = 0;
        git_oid oid;
        while (!git_revwalk_next(&oid, *walk)) {
            Commit commit = _CommitLookup(repo, oid);
            CommitPanel& commitPanel = commitPanels.emplace_back(commit);
            commitPanel.setPosition(4, 6*count);
            count++;
        }
    }
    
    mousemask(ALL_MOUSE_EVENTS | REPORT_MOUSE_POSITION, NULL);
    mouseinterval(0);
    for (int i=0;; i++) {
        // Redraw panels that need it
        for (CommitPanel& commitPanel : commitPanels) commitPanel.drawIfNeeded();
        Window::Redraw();
        
//        NCursesPanel::redraw();
        int key = rootWindow.getChar();
        if (key == KEY_MOUSE) {
            MEVENT mouse = {};
            int ir = getmouse(&mouse);
            if (ir != OK) continue;
            if (mouse.bstate & BUTTON1_PRESSED) {
                for (CommitPanel& commitPanel : commitPanels) {
                    commitPanel.setSelected(true);
                }
                _TrackSelection(rootWindow, selectionRect, mouse);
            }
        } else if (key == KEY_RESIZE) {
        }
    }
    return 0;
}
