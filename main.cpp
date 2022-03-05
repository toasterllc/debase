#include <iostream>
#include <unistd.h>
#include <vector>
#include <cassert>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <memory>
#include <deque>
#include <set>
//#include "lib/ncurses/c++/cursesp.h"
#include "lib/ncurses/include/curses.h"
#include "lib/ncurses/include/panel.h"
#include "lib/libgit2/include/git2.h"
#include "xterm-256color.h"
#include "lib/Toastbox/RuntimeError.h"

struct Vector {
    int x = 0;
    int y = 0;    
};

using Point = Vector;
using Size = Vector;

struct Rect {
    Point point;
    Size size;
};

static Rect _Intersection(const Rect& a, const Rect& b) {
    const int minX = std::max(a.point.x, b.point.x);
    const int maxX = std::min(a.point.x+a.size.x, b.point.x+b.size.x);
    const int w = maxX-minX;
    
    const int minY = std::max(a.point.y, b.point.y);
    const int maxY = std::min(a.point.y+a.size.y, b.point.y+b.size.y);
    const int h = maxY-minY;
    
    if (w<=0 || h<=0) return {};
    return {
        .point = {minX, minY},
        .size = {w, h},
    };
}

static bool _Empty(const Rect& r) {
    return r.size.x==0 || r.size.y==0;
}

class Window {
public:
    static void Redraw() {
        ::update_panels();
        ::doupdate();
    }
    
    // Invalid
    Window() {}
    
    struct NewType {};
    static constexpr NewType New;
    
    Window(NewType) {
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
    Window(Window&& x) { *this = std::move(x); }
    
    // Move assignment operator
    Window& operator=(Window&& x) {
        _state = std::move(x._state);
        x._state = {};
        return *this;
    }
    
    void setSize(const Size& s) {
        ::wresize(*this, std::max(1, s.y), std::max(1, s.x));
    }
    
    void setPosition(const Point& p) {
        ::mvwin(*this, p.y, p.x);
    }
    
    void drawBorder() {
        ::box(*this, 0, 0);
    }
    
    void drawRect(const Rect& rect) {
        const int x1 = rect.point.x;
        const int y1 = rect.point.y;
        const int x2 = rect.point.x+rect.size.x;
        const int y2 = rect.point.y+rect.size.y;
        mvwhline(*this, y1, x1, 0, rect.size.x);
        mvwhline(*this, y2, x1, 0, rect.size.x);
        mvwvline(*this, y1, x1, 0, rect.size.y);
        mvwvline(*this, y1, x2, 0, rect.size.y);
        mvwaddch(*this, y1, x1, ACS_ULCORNER);
        mvwaddch(*this, y2, x1, ACS_LLCORNER);
        mvwaddch(*this, y1, x2, ACS_URCORNER);
        mvwaddch(*this, y2, x2, ACS_LRCORNER);
    }
    
    void drawText(const Point& p, const char* fmt, ...) {
        va_list args;
        va_start(args, fmt);
        drawText(p, fmt, args);
        va_end(args);
    }

    void drawText(const Point& p, const char* fmt, va_list args) {
        ::wmove(*this, p.y, p.x);
        ::vw_printw(*this, fmt, args);
    }
    
    void erase() {
        ::werase(*this);
    }
    
    Rect rect() const {
        return Rect{
            .point = { getbegx(_state.window), getbegy(_state.window) },
            .size  = { getmaxx(_state.window), getmaxy(_state.window) },
        };
    }
    
    bool hitTest(const Point& p) const {
        return !_Empty(_Intersection(rect(), Rect{.point=p, .size={1,1}}));
    }
    
    int getChar() const {
        return ::wgetch(*this);
    }
    
    operator WINDOW*() const { return _state.window; }
    
private:
    struct {
        const Window* parent = nullptr;
        WINDOW* window = nullptr;
    } _state = {};
};

class Panel : public Window {
public:
    Panel() : Window(Window::New) {
        _state.panel = ::new_panel(*this);
        assert(_state.panel);
    }
    
    // Move constructor: use move assignment operator
    Panel(Panel&& x) { *this = std::move(x); }
    
    // Move assignment operator
    Panel& operator=(Panel&& x) {
        printf("MOVE PANEL\n");
        Window::operator=(std::move(x));
        _state = std::move(x._state);
        x._state = {};
        return *this;
    }
    
    ~Panel() {
        ::del_panel(*this);
    }
    
    void setPosition(const Point& p) {
        ::move_panel(*this, p.y, p.x);
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
//    ss << std::put_time(&tm, "%c");
    ss << std::put_time(&tm, "%a %b %d %R");
    return ss.str();
}

class CommitPanel : public Panel {
public:
    CommitPanel(Commit commit, int width) : _commit(commit) {
        _oid = _StrFromGitOid(*git_commit_id(*_commit));
        _time = _StrFromGitTime(git_commit_time(*_commit));
        _author = git_commit_author(*_commit)->name;
        
        const std::string message = git_commit_message(*_commit);
        std::deque<std::string> words;
        
        {
            std::istringstream stream(message);
            std::string word;
            while (stream >> word) words.push_back(word);
        }
        
        const int MaxLineCount = 2;
        const int MaxLineLen = width-4;
        for (;;) {
            std::string& line = _message.emplace_back();
            for (;;) {
                if (words.empty()) break;
                const std::string& word = words.front();
                const std::string add = (line.empty() ? "" : " ") + word;
                if (line.size()+add.size() > MaxLineLen) break; // Line filled, next line
                line += add;
                words.pop_front();
            }
            
            if (words.empty()) break; // No more words -> done
            if (_message.size() >= MaxLineCount) break; // Hit max number of lines -> done
            if (words.front().size() > MaxLineLen) break; // Current word is too large for line -> done
        }
        
        // Add as many letters from the remaining word as will fit on the last line
        if (!words.empty()) {
            const std::string& word = words.front();
            std::string& line = _message.back();
            line += (line.empty() ? "" : " ") + word;
            // Our logic guarantees that if the word would have fit, it would've been included in the last line.
            // So since the word isn't included, the length of the line (with the word included) must be larger
            // than `MaxLineLen`. So verify that assumption.
            assert(line.size() > MaxLineLen);
            line.erase(MaxLineLen);
            
            const char*const ellipses = "...";
            // Find the first non-space character, at least strlen(ellipses) characters before the end
            auto it = line.end()-strlen(ellipses);
            for (; it!=line.begin() && std::isspace(*it); it--);
            
            // Replace the last 4 characters with an ellipses
            line.replace(it, it+strlen(ellipses), ellipses);
        }
        
        setSize({width, 3 + (int)_message.size()});
//        setSize({width, 5 + (int)_message.size()});
        _drawNeeded = true;
    }
    
    bool selected() const { return _selected; }
    
    void setSelected(bool x) {
        if (_selected == x) return;
        _selected = x;
        _drawNeeded = true;
    }
    
    void draw() {
        erase();
        
        int i = 0;
        for (const std::string& line : _message) {
            drawText({2, 2+i}, "%s", line.c_str());
            i++;
        }
        
        if (_selected) wattron(*this, COLOR_PAIR(1));
        drawBorder();
        drawText({2, 0}, " %s ", _oid.c_str());
        if (_selected) wattroff(*this, COLOR_PAIR(1));
        drawText({12, 0}, " %s ", _time.c_str());
        
        wattron(*this, COLOR_PAIR(2));
        drawText({2, 1}, "%s", _author.c_str());
        wattroff(*this, COLOR_PAIR(2));
        
        _drawNeeded = false;
    }
    
    void drawIfNeeded() {
        if (_drawNeeded) {
            draw();
        }
    }
    
private:
    Commit _commit;
    std::string _oid;
    std::string _time;
    std::string _author;
    std::vector<std::string> _message;
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

static Window _RootWindow;
static std::vector<CommitPanel> _CommitPanels;

static void _Redraw() {
    for (CommitPanel& p : _CommitPanels) p.drawIfNeeded();
    Window::Redraw();
}

static void _TrackMouse(MEVENT mouseDownEvent) {
    CommitPanel* mouseDownCommitPanel = nullptr;
    Size mouseDownCommitPanelDelta;
    for (CommitPanel& panel : _CommitPanels) {
        if (panel.hitTest({mouseDownEvent.x, mouseDownEvent.y})) {
            mouseDownCommitPanel = &panel;
            
            const Rect panelRect = panel.rect();
            mouseDownCommitPanelDelta = {
                panelRect.point.x-mouseDownEvent.x,
                panelRect.point.y-mouseDownEvent.y,
            };
            break;
        }
    }
    
    std::set<CommitPanel*> selectionOld;
    for (CommitPanel& p : _CommitPanels) {
        if (p.selected()) selectionOld.insert(&p);
    }
    
    const bool xorSelection = (mouseDownEvent.bstate & BUTTON_SHIFT);
//    
//    if (mouseDownEvent.bstate & BUTTON_SHIFT) {
//        // Select `mouseDownCommitPanel` if it's not selected
//        if (mouseDownCommitPanel && !mouseDownCommitPanel->selected()) {
//            mouseDownCommitPanel->setSelected(true);
//        }
//    
//    } else {
//        // Deselect all panels
//        for (CommitPanel& p : _CommitPanels) p.setSelected(false);
//    }
    
    bool dragged = false;
    MEVENT mouse = mouseDownEvent;
    
    for (;;) {
        int x = std::min(mouseDownEvent.x, mouse.x);
        int y = std::min(mouseDownEvent.y, mouse.y);
        int w = std::abs(mouseDownEvent.x - mouse.x);
        int h = std::abs(mouseDownEvent.y - mouse.y);
        
        const Rect selectionRect = {{x,y},{std::max(1,w),std::max(1,h)}};
        dragged = dragged || selectionRect.size.x>1 || selectionRect.size.y>1;
        
        std::set<CommitPanel*> selectionNew;
        for (CommitPanel& p : _CommitPanels) {
            if (!_Empty(_Intersection(selectionRect, p.rect()))) selectionNew.insert(&p);
        }
        
        std::set<CommitPanel*> selection;
        if (xorSelection) {
            for (CommitPanel* p : selectionOld) {
                if (selectionNew.find(p) == selectionNew.end()) selection.insert(p);
            }
            for (CommitPanel* p : selectionNew) {
                if (selectionOld.find(p) == selectionOld.end()) selection.insert(p);
            }
        
        } else {
            selection = selectionNew;
        }
        
        if (mouseDownCommitPanel) {
            if (dragged) {
                const Point pos = {
                    mouse.x+mouseDownCommitPanelDelta.x,
                    mouse.y+mouseDownCommitPanelDelta.y,
                };
                mouseDownCommitPanel->setPosition(pos);
            }
        
        } else {
            // Redraw selection rect
            if (dragged) {
                _RootWindow.erase();
                _RootWindow.drawRect(selectionRect);
            }
        }
        
        // Update selection states of CommitPanels
        for (CommitPanel& p : _CommitPanels) {
            p.setSelected(selection.find(&p) != selection.end());
        }
        
        _Redraw();
        
        if (mouse.bstate & BUTTON1_RELEASED) break;
        
        // Wait for another mouse event
        for (;;) {
            int key = _RootWindow.getChar();
            if (key != KEY_MOUSE) continue;
            int ir = ::getmouse(&mouse);
            if (ir != OK) continue;
            break;
        }
    }
    
    // Clear selection rect when returning
    _RootWindow.erase();
}

int main(int argc, const char* argv[]) {
//    volatile bool a = false;
//    while (!a);
    
    try {
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
            
            ::use_default_colors();
            ::start_color();
            
            #warning TODO: cleanup color logic
            #warning TODO: fix colors aren't restored when exiting
            ::init_pair(1, COLOR_RED, -1);
            
            ::init_color(COLOR_GREEN, 300, 300, 300);
            ::init_pair(2, COLOR_GREEN, -1);
            
            // Hide cursor
            ::curs_set(0);
        }
        
        // Init libgit2
        {
            git_libgit2_init();
        }
        
//        volatile bool a = false;
//        while (!a);
        
        _RootWindow = Window(::stdscr);
        
        // Create panels for each commit
        {
            Repo repo = _RepoOpen();
            RevWalk walk = _RevWalkCreate(repo);
            
            int ir = git_revwalk_push_range(*walk, "HEAD~20..HEAD");
            if (ir) throw Toastbox::RuntimeError("git_revwalk_push_range failed: %s", git_error_last()->message);
            
            int off = 0;
            git_oid oid;
            while (!git_revwalk_next(&oid, *walk)) {
                Commit commit = _CommitLookup(repo, oid);
                CommitPanel& p = _CommitPanels.emplace_back(commit, 32);
                p.setPosition({4, off});
                off += p.rect().size.y + 1;
            }
        }
        
        mousemask(ALL_MOUSE_EVENTS | REPORT_MOUSE_POSITION, NULL);
        mouseinterval(0);
        for (int i=0;; i++) {
            _Redraw();
    //        NCursesPanel::redraw();
            int key = _RootWindow.getChar();
            if (key == KEY_MOUSE) {
                MEVENT mouse = {};
                int ir = ::getmouse(&mouse);
                if (ir != OK) continue;
                if (mouse.bstate & BUTTON1_PRESSED) {
                    _TrackMouse(mouse);
                }
            
            } else if (key == KEY_RESIZE) {
                throw std::runtime_error("hello");
            }
        }
    
    } catch (const std::exception& e) {
        ::endwin();
        fprintf(stderr, "Error: %s\n", e.what());
    }
    
    return 0;
}
