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
#include <map>
#include "Git.h"
#include "Window.h"
#include "Panel.h"
#include "RuntimeError.h"
#include "xterm-256color.h"
#include "RuntimeError.h"
#include "CommitPanel.h"
#include "BorderedPanel.h"
#include "RevColumn.h"
#include "GitOp.h"
#include "MakeShared.h"

struct Selection {
    Git::Rev rev;
    std::set<Git::Commit> commits;
};

static Git::Repo _Repo;

static UI::Window _RootWindow;

static std::vector<UI::RevColumn> _Columns;

static struct {
    UI::CommitPanel titlePanel;
    std::vector<UI::BorderedPanel> shadowPanels;
    bool copy = false;
} _Drag;

static Selection _Selection;
static std::optional<UI::Rect> _SelectionRect;
static std::optional<UI::Rect> _InsertionMarker;

static bool _Selected(UI::RevColumn col, UI::CommitPanel panel) {
    if (col->rev() != _Selection.rev) return false;
    if (_Selection.commits.find(panel->commit()) == _Selection.commits.end()) return false;
    return true;
}

static void _Draw() {
    const UI::Color selectionColor = (_Drag.copy ? UI::Colors::SelectionCopy : UI::Colors::SelectionMove);
    
    // Update selection colors
    {
        if (_Drag.titlePanel) {
            _Drag.titlePanel->setBorderColor(selectionColor);
            
            for (UI::BorderedPanel panel : _Drag.shadowPanels) {
                panel->setBorderColor(selectionColor);
            }
        }
        
        bool dragging = (bool)_Drag.titlePanel;
        bool copying = _Drag.copy;
        for (UI::RevColumn col : _Columns) {
            for (UI::CommitPanel panel : col->panels()) {
                if (_Selected(col, panel)) {
                    if (!dragging)      panel->setVisible(true);
                    else if (copying)   panel->setVisible(true);
                    else                panel->setVisible(false);
                    panel->setBorderColor(selectionColor);
                } else {
                    panel->setVisible(true);
                    panel->setBorderColor(std::nullopt);
                }
            }
        }
        
        // Order all the title panel and shadow panels
        if (dragging) {
            for (auto it=_Drag.shadowPanels.rbegin(); it!=_Drag.shadowPanels.rend(); it++) {
                (*it)->orderFront();
            }
            _Drag.titlePanel->orderFront();
        }
    }
    
    // Draw everything
    {
        _RootWindow->erase();
        
        for (UI::RevColumn col : _Columns) {
            col->draw();
        }
        
        if (_Drag.titlePanel) {
            _Drag.titlePanel->drawIfNeeded();
        }
        
        for (UI::BorderedPanel panel : _Drag.shadowPanels) {
            panel->drawIfNeeded();
        }
        
        if (_SelectionRect) {
            UI::Attr attr(_RootWindow, COLOR_PAIR(selectionColor));
            _RootWindow->drawRect(*_SelectionRect);
        }
        
        if (_InsertionMarker) {
            UI::Attr attr(_RootWindow, COLOR_PAIR(selectionColor));
            _RootWindow->drawLineHoriz(_InsertionMarker->point, _InsertionMarker->size.x);
            
            const char* text = (_Drag.copy ? " Copy " : " Move ");
            const UI::Point point = {
                _InsertionMarker->point.x + (_InsertionMarker->size.x - (int)strlen(text))/2,
                _InsertionMarker->point.y
            };
            _RootWindow->drawText(point, "%s", text);
        }
        
        UI::Redraw();
    }
}

struct _HitTestResult {
    UI::RevColumn column;
    UI::CommitPanel panel;
};

static std::optional<_HitTestResult> _HitTest(const UI::Point& p) {
    for (UI::RevColumn col : _Columns) {
        if (UI::CommitPanel panel = col->hitTest(p)) {
            return _HitTestResult{col, panel};
        }
    }
    return std::nullopt;
}

static std::tuple<UI::RevColumn,UI::CommitPanelVecIter> _FindInsertionPoint(const UI::Point& p) {
    UI::RevColumn insertionCol;
    UI::CommitPanelVec::iterator insertionIter;
    std::optional<int> insertLeastDistance;
    for (UI::RevColumn col : _Columns) {
        UI::CommitPanelVec& panels = col->panels();
        const UI::Rect lastRect = panels.back()->rect();
        const int midX = lastRect.point.x + lastRect.size.x/2;
        const int endY = lastRect.point.y + lastRect.size.y;
        
        for (auto it=panels.begin();; it++) {
            UI::CommitPanel panel = (it!=panels.end() ? *it : nullptr);
            const int x = (panel ? panel->rect().point.x+panel->rect().size.x/2 : midX);
            const int y = (panel ? panel->rect().point.y : endY);
            int dist = (p.x-x)*(p.x-x)+(p.y-y)*(p.y-y);
            
            if (!insertLeastDistance || dist<insertLeastDistance) {
                insertionCol = col;
                insertionIter = it;
                insertLeastDistance = dist;
            }
            if (it==panels.end()) break;
        }
    }
    
    // Adjust the insert point so that it doesn't occur within a selection
    assert(insertionCol);
    UI::CommitPanelVec& insertionColPanels = insertionCol->panels();
    for (;;) {
        if (insertionIter == insertionColPanels.begin()) break;
        UI::CommitPanel prevPanel = *std::prev(insertionIter);
        if (!_Selected(insertionCol, prevPanel)) break;
        insertionIter--;
    }
    
    return std::make_tuple(insertionCol, insertionIter);
}

static std::optional<UI::Event> _WaitForMouseEvent(MEVENT& mouse) {
    const bool mouseUp = mouse.bstate & BUTTON1_RELEASED;
    if (mouseUp) return std::nullopt;
    
    // Wait for another mouse event
    for (;;) {
        UI::Event ev = _RootWindow->nextEvent();
        if (ev == UI::Event::KeyEscape) {
            return UI::Event::KeyEscape;
        }
        if (ev != UI::Event::Mouse) continue;
        int ir = ::getmouse(&mouse);
        if (ir != OK) continue;
        return UI::Event::Mouse;
    }
}

static UI::RevColumn _ColumnForRev(Git::Rev rev) {
    for (UI::RevColumn col : _Columns) {
        if (col->rev() == rev) return col;
    }
    // Programmer error if it doesn't exist
    abort();
}

static UI::CommitPanel _PanelForCommit(UI::RevColumn col, Git::Commit commit) {
    for (UI::CommitPanel panel : col->panels()) {
        if (panel->commit() == commit) return panel;
    }
    // Programmer error if it doesn't exist
    abort();
}

static Git::Commit _FindLatestCommit(Git::Commit head, const std::set<Git::Commit>& commits) {
    while (head) {
        if (commits.find(head) != commits.end()) return head;
        head = head.parent();
    }
    // Programmer error if it doesn't exist
    abort();
}

// _TrackMouseInsideCommitPanel
// Handles dragging a set of CommitPanels
static std::optional<Git::Op> _TrackMouseInsideCommitPanel(MEVENT mouseDownEvent, UI::RevColumn mouseDownColumn, UI::CommitPanel mouseDownPanel) {
    const UI::Rect mouseDownPanelRect = mouseDownPanel->rect();
    const UI::Size delta = {
        mouseDownPanelRect.point.x-mouseDownEvent.x,
        mouseDownPanelRect.point.y-mouseDownEvent.y,
    };
    const bool wasSelected = _Selected(mouseDownColumn, mouseDownPanel);
    
    // Reset the selection to solely contain the mouse-down CommitPanel if:
    //   - there's no selection; or
    //   - the mouse-down CommitPanel is in a different column than the current selection; or
    //   - an unselected CommitPanel was clicked
    if (_Selection.commits.empty() || (_Selection.rev != mouseDownColumn->rev()) || !wasSelected) {
        _Selection = {
            .rev = mouseDownColumn->rev(),
            .commits = {mouseDownPanel->commit()},
        };
    
    } else {
        assert(!_Selection.commits.empty() && (_Selection.rev == mouseDownColumn->rev()));
        _Selection.commits.insert(mouseDownPanel->commit());
    }
    
    MEVENT mouse = mouseDownEvent;
    UI::RevColumn insertionCol;
    UI::CommitPanelVecIter insertionIter;
    bool abort = false;
    for (;;) {
        assert(!_Selection.commits.empty());
        
        const int w = std::abs(mouseDownEvent.x - mouse.x);
        const int h = std::abs(mouseDownEvent.y - mouse.y);
        const bool dragStart = w>1 || h>1;
        
        if (!_Drag.titlePanel && dragStart) {
            UI::RevColumn selectionColumn = _ColumnForRev(_Selection.rev);
            Git::Commit titleCommit = _FindLatestCommit(_Selection.rev.commit, _Selection.commits);
            UI::CommitPanel titlePanel = _PanelForCommit(selectionColumn, titleCommit);
            _Drag.titlePanel = MakeShared<UI::CommitPanel>(titleCommit, 0, titlePanel->rect().size.x);
            
            // Create shadow panels
            UI::Size shadowSize = titlePanel->rect().size;
            for (size_t i=0; i<_Selection.commits.size()-1; i++) {
                _Drag.shadowPanels.push_back(MakeShared<UI::BorderedPanel>(shadowSize));
            }
            
            // Order all the title panel and shadow panels
            for (auto it=_Drag.shadowPanels.rbegin(); it!=_Drag.shadowPanels.rend(); it++) {
                (*it)->orderFront();
            }
            _Drag.titlePanel->orderFront();
        }
        
        if (_Drag.titlePanel) {
            // Update _Drag.copy depending on whether the option key is held
            {
                const bool copy = (mouse.bstate & BUTTON_ALT);
                _Drag.copy = copy;
            }
            
            // Position title panel / shadow panels
            {
                const UI::Point pos0 = {
                    mouse.x+delta.x,
                    mouse.y+delta.y,
                };
                
                _Drag.titlePanel->setPosition(pos0);
                
                // Position shadowPanels
                int off = 1;
                for (UI::Panel p : _Drag.shadowPanels) {
                    const UI::Point pos = {pos0.x+off, pos0.y+off};
                    p->setPosition(pos);
                    off++;
                }
            }
            
            // Find insertion position
            std::tie(insertionCol, insertionIter) = _FindInsertionPoint({mouse.x, mouse.y});
            
            // Update insertion marker
            {
                constexpr int InsertionExtraWidth = 6;
                UI::CommitPanelVec& insertionColPanels = insertionCol->panels();
                const UI::Rect lastRect = insertionColPanels.back()->rect();
                const int endY = lastRect.point.y + lastRect.size.y;
                const int insertY = (insertionIter!=insertionColPanels.end() ? (*insertionIter)->rect().point.y : endY+1);
                
                _InsertionMarker = {
                    .point = {lastRect.point.x-InsertionExtraWidth/2, insertY-1},
                    .size = {lastRect.size.x+InsertionExtraWidth, 0},
                };
            }
        }
        
        _Draw();
        std::optional<UI::Event> ev = _WaitForMouseEvent(mouse);
        abort = (ev && *ev==UI::Event::KeyEscape);
        if (!ev || abort) break;
    }
    
    std::optional<Git::Op> gitOp;
    
    if (!abort && _Drag.titlePanel) {
        Git::Commit dstCommit = (insertionIter!=insertionCol->panels().end() ? (*insertionIter)->commit() : nullptr);
        gitOp = Git::Op{
            .type = (_Drag.copy ? Git::Op::Type::CopyCommits : Git::Op::Type::MoveCommits),
            .repo = _Repo,
            .src = {
                .rev = _Selection.rev,
                .commits = _Selection.commits,
            },
            .dst = {
                .rev = insertionCol->rev(),
                .position = dstCommit,
            }
        };
    }
    
    // Reset state
    {
        _Drag = {};
        _InsertionMarker = std::nullopt;
    }
    
    return gitOp;
}

// _TrackMouseOutsideCommitPanel
// Handles updating the selection rectangle / selection state
static void _TrackMouseOutsideCommitPanel(MEVENT mouseDownEvent) {
    auto selectionOld = _Selection;
    MEVENT mouse = mouseDownEvent;
    bool abort = false;
    for (;;) {
        const int x = std::min(mouseDownEvent.x, mouse.x);
        const int y = std::min(mouseDownEvent.y, mouse.y);
        const int w = std::abs(mouseDownEvent.x - mouse.x);
        const int h = std::abs(mouseDownEvent.y - mouse.y);
        const bool dragStart = w>1 || h>1;
        
        // Mouse-down outside of a commit:
        // Handle selection rect drawing / selecting commits
        const UI::Rect selectionRect = {{x,y}, {std::max(1,w),std::max(1,h)}};
        
        if (_SelectionRect || dragStart) {
            _SelectionRect = selectionRect;
        }
        
        // Update selection
        {
            Selection selectionNew;
            for (UI::RevColumn col : _Columns) {
                for (UI::CommitPanel panel : col->panels()) {
                    if (!Empty(Intersection(selectionRect, panel->rect()))) {
                        selectionNew.rev = col->rev();
                        selectionNew.commits.insert(panel->commit());
                    }
                }
                if (!selectionNew.commits.empty()) break;
            }
            
            const bool shift = (mouseDownEvent.bstate & BUTTON_SHIFT);
            if (shift && (selectionNew.commits.empty() || selectionOld.rev==selectionNew.rev)) {
                Selection selection = {
                    .rev = selectionOld.rev,
                };
                
                // selection = _Selection XOR selectionNew
                std::set_symmetric_difference(
                    selectionOld.commits.begin(), selectionOld.commits.end(),
                    selectionNew.commits.begin(), selectionNew.commits.end(),
                    std::inserter(selection.commits, selection.commits.begin())
                );
                
                _Selection = selection;
            
            } else {
                _Selection = selectionNew;
            }
        }
        
        _Draw();
        std::optional<UI::Event> ev = _WaitForMouseEvent(mouse);
        abort = (ev && *ev==UI::Event::KeyEscape);
        if (!ev || abort) break;
    }
    
    // Reset state
    {
        _SelectionRect = std::nullopt;
    }
}

static void _Reload(const std::vector<std::string>& revNames) {
    // Create a RevColumn for each specified branch
    constexpr int InsetX = 3;
    constexpr int ColumnWidth = 32;
    constexpr int ColumnSpacing = 6;
    int OffsetX = InsetX;
    
    std::vector<Git::Rev> revs;
    if (revNames.empty()) {
        revs.push_back(_Repo.head());
    
    } else {
        for (const std::string& revName : revNames) {
            revs.push_back(Git::Rev(_Repo, revName));
        }
    }
    
    _Columns.clear();
    for (const Git::Rev& rev : revs) {
        _Columns.push_back(MakeShared<UI::RevColumn>(_RootWindow, _Repo, rev, OffsetX, ColumnWidth));
        OffsetX += ColumnWidth+ColumnSpacing;
    }
}

int main(int argc, const char* argv[]) {
    #warning TODO: reject remote branches (eg 'origin/master')
    
    #warning TODO: we need to unique-ify the supplied revs, since we assume that each column has a unique rev
    #warning TODO: to do so, we'll need to implement operator< on Rev so we can put them in a set
    
    #warning TODO: draw "Move/Copy" text immediately above the dragged commits, instead of at the insertion point
    
    #warning TODO: move commits away from dragged commits to show where the commits will land
    
    #warning TODO: show similar commits to the selected commit using a lighter color
    
    #warning TODO: backup all supplied revs before doing anything
    
    #warning TODO: an undo/redo button would be nice
    
    // DONE:
//    #warning TODO: when copying commmits, don't hide the source commits
//    #warning TODO: allow escape key to abort a drag
    
    
    
    
//    git_libgit2_init();
//    
//    Git::Repo repo = Git::Repo::Open("/Users/dave/Desktop/HouseStuff");
//    Git::Ref head = repo.head();
//    Git::Branch branch = Git::Branch::FromRef(*head);
//    
//    if (branch) {
//        printf("branch != nullptr\n");
//    }
//    
//    git_reference* b = *branch;
//    printf("b == %p\n", b);
//    
//    for (;;);
    
    
//    
//    {
//        git_reference* ref = nullptr;
//        int ir = git_reference_dwim(&ref, *repo, "MyTag~1");
//        assert(!ir);
//    }
    
//    Git::Ref ref = Git::Ref::Lookup(repo, "PerfComparison2");
//    Git::Branch branch = Git::Branch::Lookup(repo, "PerfComparison2");
//    Git::Branch branch2 = Git::Branch::Lookup(repo, "PerfComparison2");
//    printf("branch==branch2: %d\n", branch==branch2);
//    printf("ref==branch: %d\n", ref==branch);
//    printf("ref.isSymbolic(): %d\n", ref.isSymbolic());
//    printf("ref.isDirect(): %d\n", ref.isDirect());
//    
//    git_reference_t t = git_reference_type(ref);
//    printf("git_reference_type: %d\n", t);
//    
//    printf("ref names: %s %s\n", ref.name(), ref.shorthandName());
//    
//    const git_oid* id1 = git_reference_target(*branch);
//    assert(id1);
//    printf("id1: %s\n", Git::Str(*id1).c_str());
//    
//    git_object* obj = nullptr;
//    int ir = git_reference_peel(&obj, *branch, GIT_OBJECT_COMMIT);
//    assert(!ir);
//    
//    const git_oid* id2 = git_object_id(obj);
//    printf("id2: %s\n", Git::Str(*id2).c_str());
    
//    volatile bool a = false;
//    while (!a);
    
    // Handle args
    std::vector<std::string> revNames;
    {
        for (int i=1; i<argc; i++) {
            revNames.push_back(argv[i]);
        }
    }
    
    // Init ncurses
    {
        // Default linux installs may not contain the /usr/share/terminfo database,
        // so provide a fallback terminfo that usually works.
        nc_set_default_terminfo(xterm_256color, sizeof(xterm_256color));
        
        // Override the terminfo 'kmous' and 'XM' properties to permit mouse-moved events,
        // in addition to the default mouse-down/up events.
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
        #warning TODO: fix: colors aren't restored when exiting
        // Redefine colors to our custom palette
        {
            int c = 1;
            
            ::init_color(c, 0, 0, 1000);
            ::init_pair(UI::Colors::SelectionMove, c, -1);
            c++;
            
            ::init_color(c, 0, 1000, 0);
            ::init_pair(UI::Colors::SelectionCopy, c, -1);
            c++;
            
            ::init_color(c, 300, 300, 300);
            ::init_pair(UI::Colors::SubtitleText, c, -1);
            c++;
        }
        
        // Hide cursor
        ::curs_set(0);
        
        _RootWindow = MakeShared<UI::Window>(::stdscr);
    }
    
    // Init git
    {
        git_libgit2_init();
        _Repo = Git::Repo::Open(".");
    }
    
    try {
//        volatile bool a = false;
//        while (!a);
        
        _Reload(revNames);
        
        ::mousemask(ALL_MOUSE_EVENTS | REPORT_MOUSE_POSITION, NULL);
        ::mouseinterval(0);
        
        #warning TODO: not sure if we're going to encounter issues with this set_escdelay
        ::set_escdelay(0);
        for (;;) {
            _Draw();
            UI::Event ev = _RootWindow->nextEvent();
            std::optional<Git::Op> gitOp;
            switch (ev) {
            case UI::Event::Mouse: {
                MEVENT mouse = {};
                int ir = ::getmouse(&mouse);
                if (ir != OK) continue;
                if (mouse.bstate & BUTTON1_PRESSED) {
                    const bool shift = (mouse.bstate & BUTTON_SHIFT);
                    const auto hitTest = _HitTest({mouse.x, mouse.y});
                    if (hitTest && !shift) {
                        // Mouse down inside of a CommitPanel, without shift key
                        gitOp = _TrackMouseInsideCommitPanel(mouse, hitTest->column, hitTest->panel);
                    } else {
                        // Mouse down outside of a CommitPanel, or mouse down anywhere with shift key
                        _TrackMouseOutsideCommitPanel(mouse);
                    }
                }
                break;
            }
            
            case UI::Event::WindowResize: {
                throw std::runtime_error("window resize");
                break;
            }
            
            case UI::Event::KeyDelete:
            case UI::Event::KeyDeleteFn: {
                gitOp = {
                    .type = Git::Op::Type::DeleteCommits,
                    .repo = _Repo,
                    .src = {
                        .rev = _Selection.rev,
                        .commits = _Selection.commits,
                    },
                };
                break;
            }
            
            default: {
//                printf("%x\n", (int)ev);
                break;
            }}
            
            if (gitOp) {
                Git::OpResult opResult = Git::Exec(*gitOp);
                
                // Reload the UI
                _Reload(revNames);
                
                // Update the selection
                _Selection = {
                    .rev = opResult.dst.rev,
                    .commits = opResult.dst.commits,
                };
            }
        }
    
    } catch (const std::exception& e) {
        ::endwin();
        fprintf(stderr, "Error: %s\n", e.what());
    }
    
    return 0;
}
