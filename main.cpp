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
#include "Git.h"
#include "Window.h"
#include "Panel.h"
#include "RuntimeError.h"
#include "xterm-256color.h"
#include "RuntimeError.h"
#include "CommitPanel.h"

class BranchColumn {
public:
    BranchColumn(int offsetX, int width) : _width(width) {
        
    }
    
private:
    int _offsetX = 0;
    int _width = 0;
    std::vector<CommitPanel> _panels;
};

static Window _RootWindow;
static std::vector<CommitPanel> _CommitPanels;

static void _Redraw() {
    for (CommitPanel& p : _CommitPanels) p.drawIfNeeded();
    Window::Redraw();
}

static CommitPanel* _HitTest(const Point& p) {
    for (CommitPanel& panel : _CommitPanels) {
        if (panel.hitTest(p)) return &panel;
    }
    return nullptr;
}

struct _CommitPanelCompare {
    bool operator() (CommitPanel* a, CommitPanel* b) const {
        return a->idx() < b->idx();
    }
};

using _CommitPanelSet = std::set<CommitPanel*, _CommitPanelCompare>;
static bool _Contains(const _CommitPanelSet& s, CommitPanel* p) {
    return s.find(p) != s.end();
}

//struct _CommitPanelSet : public std::set<CommitPanel*, _CommitPanelCompare> {
//    bool contains(CommitPanel* p) const {
//        return find(p) != end();
//    }
//};

static void _TrackMouse(MEVENT mouseDownEvent) {
    CommitPanel* mouseDownCommitPanel = _HitTest({mouseDownEvent.x, mouseDownEvent.y});
    Size mouseDownCommitPanelDelta;
    bool mouseDownCommitPanelWasSelected = (mouseDownCommitPanel ? mouseDownCommitPanel->selected() : false);
    if (mouseDownCommitPanel) {
        const Rect panelRect = mouseDownCommitPanel->rect();
        mouseDownCommitPanelDelta = {
            panelRect.point.x-mouseDownEvent.x,
            panelRect.point.y-mouseDownEvent.y,
        };
    }
    
    _CommitPanelSet selectionOld;
//    if (mouseDownCommitPanel) selectionOld.insert(mouseDownCommitPanel);
    for (CommitPanel& p : _CommitPanels) {
        if (p.selected()) selectionOld.insert(&p);
    }
    
    const bool shift = (mouseDownEvent.bstate & BUTTON_SHIFT);
    
    _CommitPanelSet selection;
    if (mouseDownCommitPanel) {
        if (shift || mouseDownCommitPanelWasSelected) {
            selection = selectionOld;
        }
        selection.insert(mouseDownCommitPanel);
    }
    
    struct {
        std::optional<CommitPanel> titlePanel;
        std::vector<Panel> shadowPanels;
        bool underway = false;
    } drag;
    
    MEVENT mouse = mouseDownEvent;
    for (;;) {
        const bool mouseUp = mouse.bstate & BUTTON1_RELEASED;
        
        const int x = std::min(mouseDownEvent.x, mouse.x);
        const int y = std::min(mouseDownEvent.y, mouse.y);
        const int w = std::abs(mouseDownEvent.x - mouse.x);
        const int h = std::abs(mouseDownEvent.y - mouse.y);
        
        drag.underway = drag.underway || w>1 || h>1;
        
        if (mouseDownCommitPanel) {
            if (drag.underway) {
                assert(!selection.empty());
                
                const Point pos0 = {
                    mouse.x+mouseDownCommitPanelDelta.x,
                    mouse.y+mouseDownCommitPanelDelta.y,
                };
                
                // Prepare drag state
                if (!drag.titlePanel) {
                    const CommitPanel& titlePanel = *(*selection.begin());
                    Commit titleCommit = titlePanel.commit();
                    drag.titlePanel.emplace(titlePanel.commit(), 0, titlePanel.rect().size.x);
                    drag.titlePanel->setSelected(true);
                    drag.titlePanel->draw();
                    
                    for (size_t i=0; i<selection.size()-1; i++) {
                        Panel& shadow = drag.shadowPanels.emplace_back();
                        shadow.setSize((*selection.begin())->rect().size);
                        Window::Attr attr = shadow.setAttr(COLOR_PAIR(1));
                        shadow.drawBorder();
                    }
                    
                    // Hide the original CommitPanels while we're dragging
                    for (auto it=selection.begin(); it!=selection.end(); it++) {
                        (*it)->setVisible(false);
                    }
                }
                
                // Position real panel
                drag.titlePanel->setPosition(pos0);
                
                // Position shadowPanels
                int off = 1;
                for (Panel& p : drag.shadowPanels) {
                    const Point pos = {pos0.x+off, pos0.y+off};
                    p.setPosition(pos);
                    off++;
                }
                
                // Order all the dragged panels
                for (auto it=drag.shadowPanels.rbegin(); it!=drag.shadowPanels.rend(); it++) {
                    it->orderFront();
                }
                drag.titlePanel->orderFront();
                
                {
                    // Find insertion position
                    decltype(_CommitPanels)::iterator insertion = _CommitPanels.begin();
                    std::optional<int> leastDistance;
                    for (auto it=_CommitPanels.begin(); it!=_CommitPanels.end(); it++) {
                        const int midY = it->rect().point.y;
                        int dist = std::abs(mouse.y-midY);
                        if (!leastDistance || dist<leastDistance) {
                            insertion = it;
                            leastDistance = dist;
                        }
                    }
                    
                    // Adjust the insertion point so that it doesn't occur within a selection
                    while (insertion!=_CommitPanels.begin() && _Contains(selection, &*std::prev(insertion))) {
                        insertion--;
                    }
                    
                    // Draw insertion point
                    {
                        const Rect panelRect = insertion->rect();
                        Window::Attr attr = _RootWindow.setAttr(COLOR_PAIR(1));
                        _RootWindow.erase();
                        _RootWindow.drawLineHoriz({panelRect.point.x-3,panelRect.point.y-1}, panelRect.point.x+panelRect.size.x+6);
                    }
                }
            
            } else {
                if (mouseUp) {
                    if (shift) {
                        if (mouseDownCommitPanelWasSelected) {
                            selection.erase(mouseDownCommitPanel);
                        }
                    } else {
                        selection = {mouseDownCommitPanel};
                    }
                }
            }
        
        } else {
            const Rect selectionRect = {{x,y},{std::max(1,w),std::max(1,h)}};
            
            // Update selection
            {
                _CommitPanelSet selectionNew;
                for (CommitPanel& p : _CommitPanels) {
                    if (!Empty(Intersection(selectionRect, p.rect()))) selectionNew.insert(&p);
                }
                
                if (shift) {
                    selection = {};
                    
                    // selection = selectionOld XOR selectionNew
                    std::set_symmetric_difference(
                        selectionOld.begin(), selectionOld.end(),
                        selectionNew.begin(), selectionNew.end(),
                        std::inserter(selection, selection.begin())
                    );
                
                } else {
                    selection = selectionNew;
                }
            }
            
            // Redraw selection rect
            if (drag.underway) {
                _RootWindow.erase();
                _RootWindow.drawRect(selectionRect);
            }
        }
        
        // Update selection states of CommitPanels
        for (CommitPanel& p : _CommitPanels) {
            p.setSelected(selection.find(&p) != selection.end());
        }
        
        _Redraw();
        
        if (mouseUp) break;
        
        // Wait for another mouse event
        for (;;) {
            int key = _RootWindow.getChar();
            if (key != KEY_MOUSE) continue;
            int ir = ::getmouse(&mouse);
            if (ir != OK) continue;
            break;
        }
    }
    
    for (CommitPanel* p : selection) {
        p->setVisible(true);
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
            Repo repo = RepoOpen();
            RevWalk walk = RevWalkCreate(repo);
            
            int ir = git_revwalk_push_range(*walk, "HEAD~5..HEAD");
            if (ir) throw RuntimeError("git_revwalk_push_range failed: %s", git_error_last()->message);
            
            constexpr int PanelWidth = 32;
            size_t idx = 0;
            int off = 0;
            git_oid oid;
            while (!git_revwalk_next(&oid, *walk)) {
                Commit commit = CommitCreate(repo, oid);
                CommitPanel& p = _CommitPanels.emplace_back(commit, idx, PanelWidth);
                p.setPosition({4, off});
                off += p.rect().size.y + 1;
                idx++;
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
