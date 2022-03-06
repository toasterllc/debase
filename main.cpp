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
#include "BranchColumn.h"

static Window _RootWindow;
static std::vector<BranchColumn> _BranchColumns;

static void _Draw() {
    for (BranchColumn& col : _BranchColumns) col.draw();
    Window::Redraw();
}

static std::tuple<BranchColumn*,CommitPanel*> _HitTest(const Point& p) {
    for (BranchColumn& col : _BranchColumns) {
        if (CommitPanel* panel = col.hitTest(p)) {
            return std::make_tuple(&col, panel);
        }
    }
    return {};
}

static CommitPanelSet _Selection() {
    CommitPanelSet s;
    for (BranchColumn& col : _BranchColumns) {
        CommitPanelSet x = col.selection();
        s.insert(x.begin(), x.end());
    }
    return s;
}

static void _TrackMouse(MEVENT mouseDownEvent) {
    struct {
        BranchColumn* col = nullptr;
        CommitPanel* panel = nullptr;
        Size delta;
        bool wasSelected = false;
    } mouseDownCommit;
    
    std::tie(mouseDownCommit.col, mouseDownCommit.panel) = _HitTest({mouseDownEvent.x, mouseDownEvent.y});
    
    if (mouseDownCommit.panel) {
        const Rect panelRect = mouseDownCommit.panel->rect();
        mouseDownCommit.delta = {
            panelRect.point.x-mouseDownEvent.x,
            panelRect.point.y-mouseDownEvent.y,
        };
        mouseDownCommit.wasSelected = mouseDownCommit.panel->selected();
    }
    
    
    const bool shift = (mouseDownEvent.bstate & BUTTON_SHIFT);
    CommitPanelSet selectionOld = _Selection();
    CommitPanelSet selection;
    
    if (mouseDownCommit.panel) {
        if (shift || mouseDownCommit.wasSelected) {
            selection = selectionOld;
        }
        selection.insert(mouseDownCommit.panel);
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
        
        if (mouseDownCommit.panel) {
            if (drag.underway) {
                assert(!selection.empty());
                
                const Point pos0 = {
                    mouse.x+mouseDownCommit.delta.x,
                    mouse.y+mouseDownCommit.delta.y,
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
                    CommitPanelVector& panels = mouseDownCommit.col->panels();
                    CommitPanelVector::iterator insertion = panels.begin();
                    std::optional<int> leastDistance;
                    const Rect lastRect = panels.back().rect();
                    const int endY = lastRect.point.y + lastRect.size.y;
                    for (auto it=panels.begin();; it++) {
                        const int y = (it!=panels.end() ? it->rect().point.y : endY);
                        int dist = std::abs(mouse.y-y);
                        if (!leastDistance || dist<leastDistance) {
                            insertion = it;
                            leastDistance = dist;
                        }
                        if (it==panels.end()) break;
                    }
                    
                    // Adjust the insertion point so that it doesn't occur within a selection
                    while (insertion!=panels.begin() && Contains(selection, &*std::prev(insertion))) {
                        insertion--;
                    }
                    
                    // Draw insertion point
                    {
                        const int insertionY = (insertion!=panels.end() ? insertion->rect().point.y : endY+1);
                        Window::Attr attr = _RootWindow.setAttr(COLOR_PAIR(1));
                        _RootWindow.erase();
                        _RootWindow.drawLineHoriz({lastRect.point.x-3,insertionY-1}, lastRect.point.x+lastRect.size.x+3);
                    }
                }
            
            } else {
                if (mouseUp) {
                    if (shift) {
                        if (mouseDownCommit.wasSelected) {
                            selection.erase(mouseDownCommit.panel);
                        }
                    } else {
                        selection = {mouseDownCommit.panel};
                    }
                }
            }
        
        } else {
            const Rect selectionRect = {{x,y},{std::max(1,w),std::max(1,h)}};
            
            // Update selection
            {
                CommitPanelSet selectionNew;
                for (BranchColumn& col : _BranchColumns) {
                    for (CommitPanel& panel : col.panels()) {
                        if (!Empty(Intersection(selectionRect, panel.rect()))) selectionNew.insert(&panel);
                    }
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
        for (BranchColumn& col : _BranchColumns) {
            for (CommitPanel& panel : col.panels()) {
                panel.setSelected(Contains(selection, &panel));
            }
        }
        
        _Draw();
        
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
            constexpr int InsetX = 3;
            constexpr int ColumnWidth = 32;
            constexpr int ColumnSpacing = 6;
            Repo repo = RepoOpen();
            
            int OffsetX = InsetX;
            for (auto name : {"HEAD", "PerfComparison"}) {
                _BranchColumns.emplace_back(_RootWindow, repo, name, OffsetX, ColumnWidth);
                OffsetX += ColumnWidth+ColumnSpacing;
            }
        }
        
        mousemask(ALL_MOUSE_EVENTS | REPORT_MOUSE_POSITION, NULL);
        mouseinterval(0);
        for (int i=0;; i++) {
            _Draw();
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
