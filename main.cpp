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
#include <filesystem>
#include <spawn.h>
#include "lib/Toastbox/RuntimeError.h"
#include "lib/Toastbox/Defer.h"
#include "Git.h"
#include "Window.h"
#include "Panel.h"
#include "xterm-256color.h"
#include "CommitPanel.h"
#include "BorderedPanel.h"
#include "Menu.h"
#include "RevColumn.h"
#include "GitOp.h"
#include "MakeShared.h"
#include "Bitfield.h"
#include "ErrorPanel.h"
#include "State.h"
#include "StateDir.h"
#include "SnapshotButton.h"
#include "SnapshotMenu.h"

namespace fs = std::filesystem;

struct _Selection {
    Git::Rev rev;
    std::set<Git::Commit> commits;
};

class _ExitRequest : public std::exception {
};

static UI::ColorPalette _Colors;
static std::optional<UI::ColorPalette> _ColorsPrev;

static Git::Repo _Repo;
static State::RepoState _RepoState;
static Git::Rev _Head;
static std::vector<Git::Rev> _Revs;
//static std::map<Git::Ref,RefHistory> _RefHistory;

static UI::Window _RootWindow;
static std::vector<UI::RevColumn> _Columns;

static struct {
    UI::CommitPanel titlePanel;
    std::vector<UI::BorderedPanel> shadowPanels;
    std::optional<UI::Rect> insertionMarker;
    bool copy = false;
} _Drag;

static struct {
    Git::Commit commit;
    std::chrono::steady_clock::time_point mouseUpTime;
} _DoubleClickState;

static constexpr std::chrono::milliseconds _DoubleClickThresh(300);
static constexpr std::chrono::milliseconds _ContextMenuStayOpenThresh(300);

static _Selection _Selection;
static std::optional<UI::Rect> _SelectionRect;

static UI::Menu _ContextMenu;
static UI::SnapshotMenu _SnapshotsMenu;

static UI::ErrorPanel _ErrorPanel;

static constexpr mmask_t _SelectionShiftKeys = BUTTON_CTRL | BUTTON_SHIFT;

enum class _SelectState {
    False,
    True,
    Similar,
};

static _SelectState _SelectStateGet(UI::RevColumn col, UI::CommitPanel panel) {
    bool similar = _Selection.commits.find(panel->commit()) != _Selection.commits.end();
    if (!similar) return _SelectState::False;
    return (col->rev()==_Selection.rev ? _SelectState::True : _SelectState::Similar);
}

static bool _Selected(UI::RevColumn col, UI::CommitPanel panel) {
    return _SelectStateGet(col, panel) == _SelectState::True;
}

//static RefHistory& _RefHistory(Git::Ref ref) {
//    return _RefHistoryMap.at(ref);
//}
//
//static RefHistory& _RefHistoryX(Git::Ref ref) {
//    return _RefCommitHistoryMap[ref][ref.commit()];
//}

static void _Draw() {
    const UI::Color selectionColor = (_Drag.copy ? _Colors.selectionCopy : _Colors.selectionMove);
    
    // Update panels
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
                bool visible = false;
                std::optional<UI::Color> borderColor;
                _SelectState selectState = _SelectStateGet(col, panel);
                if (selectState == _SelectState::True) {
                    visible = !dragging || copying;
                    if (dragging) borderColor = _Colors.selectionSimilar;
                    else          borderColor = _Colors.selectionMove;
                
                } else {
                    visible = true;
                    if (!dragging && selectState==_SelectState::Similar) borderColor = _Colors.selectionSimilar;
                }
                
                panel->setVisible(visible);
                panel->setBorderColor(borderColor);
            }
        }
        
        // Order all the title panel and shadow panels
        if (dragging) {
            for (auto it=_Drag.shadowPanels.rbegin(); it!=_Drag.shadowPanels.rend(); it++) {
                (*it)->orderFront();
            }
            _Drag.titlePanel->orderFront();
        }
        
        if (_ContextMenu) {
            _ContextMenu->orderFront();
        }
        
        if (_SnapshotsMenu) {
            _SnapshotsMenu->orderFront();
        }
        
        if (_ErrorPanel) {
            UI::Size ps = _ErrorPanel->frame().size;
            UI::Size rs = _RootWindow->frame().size;
            UI::Point p = {
                (rs.x-ps.x)/2,
                (rs.y-ps.y)/3,
            };
            _ErrorPanel->setPosition(p);
            _ErrorPanel->orderFront();
        }
    }
    
    // Draw everything
    {
        _RootWindow->erase();
        
        if (_Drag.titlePanel) {
            _Drag.titlePanel->drawIfNeeded();
            
            // Draw insertion marker
            if (_Drag.insertionMarker) {
                UI::Attr attr(_RootWindow, selectionColor);
                _RootWindow->drawLineHoriz(_Drag.insertionMarker->point, _Drag.insertionMarker->size.x);
            }
        }
        
        for (UI::BorderedPanel panel : _Drag.shadowPanels) {
            panel->drawIfNeeded();
        }
        
        if (_SelectionRect) {
            UI::Attr attr(_RootWindow, _Colors.selectionMove);
            _RootWindow->drawRect(*_SelectionRect);
        }
        
        for (UI::RevColumn col : _Columns) {
            col->draw();
        }
        
        if (_ContextMenu) {
            _ContextMenu->draw();
        }
        
        if (_SnapshotsMenu) {
            _SnapshotsMenu->draw();
        }
        
        if (_ErrorPanel) {
            _ErrorPanel->drawIfNeeded();
        }
        
        UI::Redraw();
    }
}

struct _HitTestResult {
    UI::RevColumn column;
    UI::RevColumnHitTestResult hitTest;
    operator bool() const {
        return (bool)column;
    }
};

static _HitTestResult _HitTest(const UI::Point& p) {
    // Don't bail when we get a hit -- we need to update the UI for all
    // columns (by calling hitTest) before returning
    _HitTestResult hit;
    for (UI::RevColumn col : _Columns) {
        UI::RevColumnHitTestResult h = col->updateMouse(p);
        if (h) {
            hit = _HitTestResult{
                .column = col,
                .hitTest = h,
            };
        }
    }
    return hit;
}

struct _InsertionPosition {
    UI::RevColumn col;
    UI::CommitPanelVecIter iter;
};

static std::optional<_InsertionPosition> _FindInsertionPosition(const UI::Point& p) {
    UI::RevColumn icol;
    UI::CommitPanelVecIter iiter;
    std::optional<int> leastDistance;
    for (UI::RevColumn col : _Columns) {
        UI::CommitPanelVec& panels = col->panels();
        // Ignore empty columns (eg if the window is too small to fit a column, it may not have any panels)
        if (panels.empty()) continue;
        const UI::Rect lastFrame = panels.back()->frame();
        const int midX = lastFrame.point.x + lastFrame.size.x/2;
        const int endY = lastFrame.point.y + lastFrame.size.y;
        
        for (auto it=panels.begin();; it++) {
            UI::CommitPanel panel = (it!=panels.end() ? *it : nullptr);
            const int x = (panel ? panel->frame().point.x+panel->frame().size.x/2 : midX);
            const int y = (panel ? panel->frame().point.y : endY);
            int dist = (p.x-x)*(p.x-x)+(p.y-y)*(p.y-y);
            
            if (!leastDistance || dist<leastDistance) {
                icol = col;
                iiter = it;
                leastDistance = dist;
            }
            if (it==panels.end()) break;
        }
    }
    
    if (!icol) return std::nullopt;
    // Ignore immutable columns
    if (!icol->rev().isMutable()) return std::nullopt;
    
    // Adjust the insert point so that it doesn't occur within a selection
    UI::CommitPanelVec& icolPanels = icol->panels();
    for (;;) {
        if (iiter == icolPanels.begin()) break;
        UI::CommitPanel prevPanel = *std::prev(iiter);
        if (!_Selected(icol, prevPanel)) break;
        iiter--;
    }
    
    return _InsertionPosition{icol, iiter};
}

struct MouseButtons : Bitfield<uint8_t> {
    static constexpr Bit Left  = 1<<0;
    static constexpr Bit Right = 1<<1;
    using Bitfield::Bitfield;
};

static mmask_t _MouseButtonReleasedMask(MouseButtons buttons) {
    mmask_t r = 0;
    if (buttons & MouseButtons::Left)  r |= BUTTON1_RELEASED;
    if (buttons & MouseButtons::Right) r |= BUTTON3_RELEASED;
    return r;
}

struct _Event {
    UI::Event type = UI::Event::None;
    MEVENT mouse = {};
    bool mouseUp = false;
};

static _Event _WaitForEvent(MouseButtons buttons=MouseButtons::Left) {
    const mmask_t mouseReleasedMask = _MouseButtonReleasedMask(buttons);
    // Wait for another mouse event
    for (;;) {
        UI::Event ev = _RootWindow->nextEvent();
        switch (ev) {
        case UI::Event::WindowResize:
        case UI::Event::KeyEscape: {
            return _Event{ .type = ev };
        }
        
        case UI::Event::Mouse: {
            MEVENT mouse = {};
            int ir = ::getmouse(&mouse);
            if (ir != OK) continue;
            return _Event{
                .type = UI::Event::Mouse,
                .mouse = mouse,
                .mouseUp = (bool)(mouse.bstate & mouseReleasedMask),
            };
        }
        
        case UI::Event::KeyCtrlC:
        case UI::Event::KeyCtrlD: {
            throw _ExitRequest();
        }
        
        default: {
            continue;
        }}
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

static void _Reload() {
    // Create a RevColumn for each specified branch
    constexpr int InsetX = 3;
    constexpr int ColumnWidth = 32;
    constexpr int ColumnSpacing = 6;
    
    if (_Head.ref) {
        _Head = _Repo.revReload(_Head);
    }
    
    for (Git::Rev& rev : _Revs) {
        if (rev.ref) {
            rev = _Repo.revReload(rev);
        }
    }
    
    _Columns.clear();
    int OffsetX = InsetX;
    for (const Git::Rev& rev : _Revs) {
        State::History* h = (rev.ref ? &_RepoState.history(rev.ref) : nullptr);
        UI::RevColumnOptions opts = {
            .win                = _RootWindow,
            .colors             = _Colors,
            .repo               = _Repo,
            .rev                = rev,
            .head               = (rev.displayHead() == _Head.commit),
            .offset             = UI::Size{OffsetX, 0},
            .width              = ColumnWidth,
            .undoEnabled        = (h ? !h->begin() : false),
            .redoEnabled        = (h ? !h->end() : false),
            .snapshotsEnabled   = true,
        };
        _Columns.push_back(MakeShared<UI::RevColumn>(opts));
        OffsetX += ColumnWidth+ColumnSpacing;
    }
}

static void _UndoRedo(UI::RevColumn col, bool undo) {
    Git::Rev rev = col->rev();
    State::History& h = _RepoState.history(col->rev().ref);
    
    State::RefState refStatePrev = h.get();
    if (undo) h.prev();
    else      h.next();
    State::RefState refState = h.get();
    
    Git::Commit commit = State::Convert(_Repo, h.get().head);
    std::set<Git::Commit> selection = State::Convert(_Repo, (!undo ? refState.selection : refStatePrev.selectionPrev));
    
    rev = _Repo.revReplace(rev, commit);
    _Selection = {
        .rev = rev,
        .commits = selection,
    };
    
    _Reload();
}

// _TrackMouseInsideCommitPanel
// Handles clicking/dragging a set of CommitPanels
static std::optional<Git::Op> _TrackMouseInsideCommitPanel(MEVENT mouseDownEvent, UI::RevColumn mouseDownColumn, UI::CommitPanel mouseDownPanel) {
    const UI::Rect mouseDownPanelFrame = mouseDownPanel->frame();
    const UI::Size delta = {
        mouseDownPanelFrame.point.x-mouseDownEvent.x,
        mouseDownPanelFrame.point.y-mouseDownEvent.y,
    };
    const bool wasSelected = _Selected(mouseDownColumn, mouseDownPanel);
    const UI::Rect innerBounds = Inset(_RootWindow->bounds(), {2,2});
    const auto doubleClickStatePrev = _DoubleClickState;
    _DoubleClickState = {};
    
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
    
    _Event ev = {
        .type = UI::Event::Mouse,
        .mouse = mouseDownEvent,
    };
    std::optional<_InsertionPosition> ipos;
    bool abort = false;
    bool mouseDragged = false;
    for (;;) {
        assert(!_Selection.commits.empty());
        UI::RevColumn selectionColumn = _ColumnForRev(_Selection.rev);
        
        const UI::Point p = {ev.mouse.x, ev.mouse.y};
        const int w = std::abs(mouseDownEvent.x - p.x);
        const int h = std::abs(mouseDownEvent.y - p.y);
        // allow: cancel drag when mouse is moved to the edge (as an affordance to the user)
        const bool allow = !Empty(Intersection(innerBounds, {p, {1,1}}));
        mouseDragged |= w>1 || h>1;
        
        // Find insertion position
        ipos = _FindInsertionPosition(p);
        
        if (!_Drag.titlePanel && mouseDragged && allow) {
            Git::Commit titleCommit = _FindLatestCommit(_Selection.rev.commit, _Selection.commits);
            UI::CommitPanel titlePanel = _PanelForCommit(selectionColumn, titleCommit);
            _Drag.titlePanel = MakeShared<UI::CommitPanel>(_Colors, true, titlePanel->frame().size.x, titleCommit);
            
            // Create shadow panels
            UI::Size shadowSize = _Drag.titlePanel->frame().size;
            for (size_t i=0; i<_Selection.commits.size()-1; i++) {
                _Drag.shadowPanels.push_back(MakeShared<UI::BorderedPanel>(shadowSize));
            }
            
            // Order all the title panel and shadow panels
            for (auto it=_Drag.shadowPanels.rbegin(); it!=_Drag.shadowPanels.rend(); it++) {
                (*it)->orderFront();
            }
            _Drag.titlePanel->orderFront();
        
        } else if (!allow) {
            _Drag = {};
        }
        
        if (_Drag.titlePanel) {
            // Update _Drag.copy depending on whether the option key is held
            {
                // forceCopy: require copying if the source column isn't mutable (and therefore commits
                // can't be moved away from it, because that would require deleting the commits from
                // the source column)
                const bool forceCopy = !selectionColumn->rev().isMutable();
                const bool copy = (ev.mouse.bstate & BUTTON_ALT) || forceCopy;
                _Drag.copy = copy;
                _Drag.titlePanel->setHeaderLabel(_Drag.copy ? "Copy" : "Move");
            }
            
            // Position title panel / shadow panels
            {
                const UI::Point pos0 = p + delta;
                
                _Drag.titlePanel->setPosition(pos0);
                
                // Position shadowPanels
                int off = 1;
                for (UI::Panel p : _Drag.shadowPanels) {
                    const UI::Point pos = pos0+off;
                    p->setPosition(pos);
                    off++;
                }
            }
            
            // Update insertion marker
            if (ipos) {
                constexpr int InsertionExtraWidth = 6;
                UI::CommitPanelVec& ipanels = ipos->col->panels();
                const UI::Rect lastFrame = ipanels.back()->frame();
                const int endY = lastFrame.point.y + lastFrame.size.y;
                const int insertY = (ipos->iter!=ipanels.end() ? (*ipos->iter)->frame().point.y : endY+1);
                
                _Drag.insertionMarker = {
                    .point = {lastFrame.point.x-InsertionExtraWidth/2, insertY-1},
                    .size = {lastFrame.size.x+InsertionExtraWidth, 0},
                };
            
            } else {
                _Drag.insertionMarker = std::nullopt;
            }
        }
        
        _Draw();
        ev = _WaitForEvent();
        abort = (ev.type != UI::Event::Mouse);
        // Check if we should abort
        if (abort || ev.mouseUp) {
            break;
        }
    }
    
    std::optional<Git::Op> gitOp;
    if (!abort) {
        if (_Drag.titlePanel && ipos) {
            Git::Commit dstCommit = ((ipos->iter != ipos->col->panels().end()) ? (*ipos->iter)->commit() : nullptr);
            gitOp = Git::Op{
                .type = (_Drag.copy ? Git::Op::Type::Copy : Git::Op::Type::Move),
                .src = {
                    .rev = _Selection.rev,
                    .commits = _Selection.commits,
                },
                .dst = {
                    .rev = ipos->col->rev(),
                    .position = dstCommit,
                }
            };
        
        // If this was a mouse-down + mouse-up without dragging in between,
        // set the selection to the commit that was clicked
        } else if (!mouseDragged) {
            _Selection = {
                .rev = mouseDownColumn->rev(),
                .commits = {mouseDownPanel->commit()},
            };
            
            auto currentTime = std::chrono::steady_clock::now();
            Git::Commit commit = *_Selection.commits.begin();
            const bool doubleClicked =
                doubleClickStatePrev.commit &&
                doubleClickStatePrev.commit==commit &&
                currentTime-doubleClickStatePrev.mouseUpTime < _DoubleClickThresh;
            const bool validTarget = _Selection.rev.isMutable();
            
            if (doubleClicked) {
                if (validTarget) {
                    gitOp = {
                        .type = Git::Op::Type::Edit,
                        .src = {
                            .rev = _Selection.rev,
                            .commits = _Selection.commits,
                        },
                    };
                
                } else {
                    beep();
                }
            }
            
            _DoubleClickState = {
                .commit = *_Selection.commits.begin(),
                .mouseUpTime = currentTime,
            };
        }
    }
    
    // Reset state
    {
        _Drag = {};
    }
    
    return gitOp;
}

// _TrackMouseOutsideCommitPanel
// Handles updating the selection rectangle / selection state
static void _TrackMouseOutsideCommitPanel(MEVENT mouseDownEvent) {
    auto selectionOld = _Selection;
    _Event ev = {
        .type = UI::Event::Mouse,
        .mouse = mouseDownEvent,
    };
    
    for (;;) {
        const int x = std::min(mouseDownEvent.x, ev.mouse.x);
        const int y = std::min(mouseDownEvent.y, ev.mouse.y);
        const int w = std::abs(mouseDownEvent.x - ev.mouse.x);
        const int h = std::abs(mouseDownEvent.y - ev.mouse.y);
        const bool dragStart = w>1 || h>1;
        
        // Mouse-down outside of a commit:
        // Handle selection rect drawing / selecting commits
        const UI::Rect selectionRect = {{x,y}, {std::max(2,w),std::max(2,h)}};
        
        if (_SelectionRect || dragStart) {
            _SelectionRect = selectionRect;
        }
        
        // Update selection
        {
            struct _Selection selectionNew;
            for (UI::RevColumn col : _Columns) {
                for (UI::CommitPanel panel : col->panels()) {
                    if (!Empty(Intersection(selectionRect, panel->frame()))) {
                        selectionNew.rev = col->rev();
                        selectionNew.commits.insert(panel->commit());
                    }
                }
                if (!selectionNew.commits.empty()) break;
            }
            
            const bool shift = (mouseDownEvent.bstate & _SelectionShiftKeys);
            if (shift && (selectionNew.commits.empty() || selectionOld.rev==selectionNew.rev)) {
                struct _Selection selection = {
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
        ev = _WaitForEvent();
        // Check if we should abort
        if (ev.type!=UI::Event::Mouse || ev.mouseUp) {
            break;
        }
    }
    
    // Reset state
    {
        _SelectionRect = std::nullopt;
    }
}

static UI::Button _MakeContextMenuButton(std::string_view label, std::string_view key, bool enabled) {
    constexpr int ContextMenuWidth = 12;
    UI::Button b = MakeShared<UI::Button>();
    UI::ButtonOptions& opts = b->options();
    opts.colors = _Colors;
    opts.label = label;
    opts.key = key;
    opts.enabled = enabled;
    opts.insetX = 0;
    opts.frame.size.x = ContextMenuWidth;
    opts.frame.size.y = 1;
    return b;
}

static std::optional<Git::Op> _TrackRightMouse(MEVENT mouseDownEvent, UI::RevColumn mouseDownColumn, UI::CommitPanel mouseDownPanel) {
    auto mouseDownTime = std::chrono::steady_clock::now();
    _Event ev = {
        .type = UI::Event::Mouse,
        .mouse = mouseDownEvent,
    };
    
    // If the commit that was clicked isn't selected, set the selection to only that commit
    if (!_Selected(mouseDownColumn, mouseDownPanel)) {
        _Selection = {
            .rev = mouseDownColumn->rev(),
            .commits = {mouseDownPanel->commit()},
        };
    }
    
    assert(!_Selection.commits.empty());
    
    bool selectionContainsMerge = false;
    for (Git::Commit commit : _Selection.commits) {
        if (commit.isMerge()) {
            selectionContainsMerge = true;
            break;
        }
    }
    
    bool combineEnabled = _Selection.rev.isMutable() && _Selection.commits.size()>1 && !selectionContainsMerge;
    bool editEnabled    = _Selection.rev.isMutable() && _Selection.commits.size() == 1;
    bool deleteEnabled  = _Selection.rev.isMutable();
    UI::Button combineButton = _MakeContextMenuButton("Combine", "c", combineEnabled);
    UI::Button editButton    = _MakeContextMenuButton("Edit", "ret", editEnabled);
    UI::Button deleteButton  = _MakeContextMenuButton("Delete", "del", deleteEnabled);
    std::vector<UI::Button> buttons = { combineButton, editButton, deleteButton };
    UI::MenuOptions opts = {
        .colors = _Colors,
        .parentWindow = _RootWindow,
        .position = {mouseDownEvent.x, mouseDownEvent.y},
        .buttons = buttons
    };
    _ContextMenu = MakeShared<UI::Menu>(opts);
    
    UI::Button menuButton = nullptr;
    MouseButtons mouseUpButtons = MouseButtons::Right;
    bool abort = false;
    for (;;) {
        if (ev.type == UI::Event::Mouse) {
            menuButton = _ContextMenu->updateMouse({ev.mouse.x, ev.mouse.y});
        } else {
            menuButton = nullptr;
        }
        
        _Draw();
        ev = _WaitForEvent(mouseUpButtons);
        abort = (ev.type != UI::Event::Mouse);
        
        // Check if we should abort
        if (abort) {
            break;
        
        // Handle mouse up
        } else if (ev.mouseUp) {
            if (!(mouseUpButtons & MouseButtons::Left)) {
                // If the right-mouse-up occurs soon enough after right-mouse-down, the menu should
                // stay open and we should start listening for left-mouse-down events.
                // If the right-mouse-up occurs af
                auto duration = std::chrono::steady_clock::now()-mouseDownTime;
                if (duration >= _ContextMenuStayOpenThresh) break;
                
                // Start listening for left mouse up
                mouseUpButtons |= MouseButtons::Left;
                
                // Right mouse up, but menu stays open
                // Now start tracking both left+right mouse down
            } else {
                // Close the menu only if clicking outside of the menu, or clicking on an
                // enabled menu button.
                // In other words, don't close the menu when clicking on a disabled menu
                // button.
                if (!menuButton || menuButton->options().enabled) {
                    break;
                }
            }
        }
    }
    
    // Handle the clicked button
    std::optional<Git::Op> gitOp;
    if (!abort) {
        if (menuButton == combineButton) {
            gitOp = Git::Op{
                .type = Git::Op::Type::Combine,
                .src = {
                    .rev = _Selection.rev,
                    .commits = _Selection.commits,
                },
            };
        
        } else if (menuButton == editButton) {
            gitOp = Git::Op{
                .type = Git::Op::Type::Edit,
                .src = {
                    .rev = _Selection.rev,
                    .commits = _Selection.commits,
                },
            };
        
        } else if (menuButton == deleteButton) {
            gitOp = Git::Op{
                .type = Git::Op::Type::Delete,
                .src = {
                    .rev = _Selection.rev,
                    .commits = _Selection.commits,
                },
            };
        }
    }
    
    // Reset state
    {
        _ContextMenu = nullptr;
    }
    
    return gitOp;
}

constexpr int _SnapshotMenuWidth = 26;

static UI::Button _MakeSnapshotMenuButton(Git::Repo repo, Git::Ref ref, const State::Snapshot& snap, bool sessionStart) {
    bool activeSnapshot = State::Convert(ref.commit()) == snap.head;
    UI::SnapshotButtonOptions snapButtonOpts = {
        .repo           = repo,
        .snapshot       = snap,
        .width          = _SnapshotMenuWidth,
        .sessionStart   = sessionStart,
        .activeSnapshot = activeSnapshot,
    };
    
    UI::SnapshotButton b = MakeShared<UI::SnapshotButton>(snapButtonOpts);
    b->options().enabled = true;
    b->options().colors = _Colors;
    
//    UI::SnapshotButtonOptions& snapOpts = b->snapshotOptions();
//    snapOpts.sessionStart = sessionStart;
//    snapOpts.activeSnapshot = false;
////    .sessionStart = sessionStart
    
    return b;
}

static void _TrackSnapshotsMenu(UI::RevColumn column) {
    Git::Ref ref = column->rev().ref;
    std::vector<UI::Button> buttons = {
        _MakeSnapshotMenuButton(_Repo, ref, _RepoState.initialSnapshot(ref), true),
    };
    
    const std::vector<State::Snapshot>& snapshots = _RepoState.snapshots(ref);
    for (auto it=snapshots.rbegin(); it!=snapshots.rend(); it++) {
//        auto button = MakeShared<UI::SnapshotButton>(_Repo, *it);
        buttons.push_back(_MakeSnapshotMenuButton(_Repo, ref, *it, false));
        
//        button->options().label = "Start of Session";
//        button->options().enabled = true;
//        Git::Commit commit = Convert(_Repo, it->history.get().head);
//        std::string idStr = Git::DisplayStringForId(commit.id());
//        buttons.push_back(MakeShared<UI::SnapshotButton>(UI::ButtonOptions{ .label=idStr, .enabled=true }));
    }
    
    const int width = _SnapshotMenuWidth+UI::_SnapshotMenu::Padding().x;
    const int px = column->opts().offset.x + (column->opts().width-width)/2;
    UI::MenuOptions menuOpts = {
        .colors = _Colors,
        .parentWindow = _RootWindow,
        .position = {px, 1},
        .title = "Session Start",
        .buttons = buttons,
        .allowTruncate = true,
    };
    _SnapshotsMenu = MakeShared<UI::SnapshotMenu>(menuOpts);
    
    UI::SnapshotButton menuButton = nullptr;
    bool abort = false;
    for (;;) {
        _Draw();
        
        _Event ev = _WaitForEvent();
        abort = (ev.type != UI::Event::Mouse);
        
        if (ev.type == UI::Event::Mouse) {
            menuButton = std::dynamic_pointer_cast<UI::_SnapshotButton>(_SnapshotsMenu->updateMouse({ev.mouse.x, ev.mouse.y}));
        } else {
            menuButton = nullptr;
        }
        
        // Check if we should abort
        if (abort) {
            break;
        
        // Handle mouse up
        } else if (ev.mouseUp) {
            // Close the menu only if clicking outside of the menu, or clicking on an
            // enabled menu button.
            // In other words, don't close the menu when clicking on a disabled menu
            // button.
            if (!menuButton || menuButton->options().enabled) {
                break;
            }
        }
    }
    
    if (!abort && menuButton) {
        State::History& h = _RepoState.history(ref);
        State::Commit commitNew = menuButton->snapshotOptions().snapshot.head;
        State::Commit commitCur = h.get().head;
        
        if (commitNew != commitCur) {
            #warning TODO: handle not being able to materialize commit
            h.push(State::RefState{.head = commitNew});
            _Repo.refReplace(ref, Convert(_Repo, commitNew));
            _Reload();
        }
    }
    
    // Reset state
    {
        _SnapshotsMenu = nullptr;
    }
}

static void _TrackMouseInsideButton(MEVENT mouseDownEvent, UI::RevColumn column, const UI::RevColumnHitTestResult& mouseDownHitTest) {
    UI::RevColumnHitTestResult hit;
    _Event ev = {
        .type = UI::Event::Mouse,
        .mouse = mouseDownEvent,
    };
    
    for (;;) {
        if (ev.type == UI::Event::Mouse) {
            hit = column->updateMouse({ev.mouse.x, ev.mouse.y});
        } else {
            hit = {};
        }
        
        _Draw();
        ev = _WaitForEvent();
        if (ev.mouseUp) break;
    }
    
    if (hit.buttonEnabled && hit==mouseDownHitTest) {
        switch (hit.button) {
        case UI::RevColumnButton::Undo:
            _UndoRedo(column, true);
            break;
        case UI::RevColumnButton::Redo:
            _UndoRedo(column, false);
            break;
        case UI::RevColumnButton::Snapshots:
            _TrackSnapshotsMenu(column);
            break;
        default:
            break;
        }
    }
}

struct _SavedColor {
    short r = 0;
    short g = 0;
    short b = 0;
};

static UI::ColorPalette _ColorsSet(const UI::ColorPalette& colors) {
    UI::ColorPalette colorsPrev;
    auto colorsAll = colors.all();
    auto colorsPrevAll = colorsPrev.all();
    for (auto i=colorsAll.begin(), ip=colorsPrevAll.begin(); i!=colorsAll.end(); i++, ip++) {
        UI::Color& c = i->get();
        
        if (c.custom) {
            UI::Color& cprev = ip->get();
            cprev.idx = c.idx;
            color_content(cprev.idx, &cprev.r, &cprev.g, &cprev.b);
            ::init_color(c.idx, c.r, c.g, c.b);
        }
        
        ::init_pair(c.idx, c.idx, -1);
    }
    
    return colorsPrev;
}

static UI::ColorPalette _ColorsCreate() {
    // _Idx0: start outside the standard 0-7 range because we don't want to clobber the standard terminal colors.
    // This is because reading the current terminal colors isn't reliable (via color_content), therefore when we
    // restore colors on exit, we won't necessarily be restoring the original color. So if we're going to clobber
    // colors, clobber the colors that are less likely to be used.
    static constexpr int Idx0 = 16;
    
    std::string termProg = getenv("TERM_PROGRAM");
    
    UI::ColorPalette colors;
    
    if (termProg == "Apple_Terminal") {
        // Colorspace: unknown
        // There's no simple relation between these numbers and the resulting colors because Apple's
        // Terminal.app applies some kind of filtering on top of these numbers. These values were
        // manually chosen based on their appearance.
        colors.normal           = COLOR_BLACK;
        colors.selectionMove    = UI::Color(Idx0+0,    0,    0, 1000);
        colors.selectionCopy    = UI::Color(Idx0+1,    0, 1000,    0);
        colors.selectionSimilar = UI::Color(Idx0+2,  550,  550, 1000);
        colors.disabledText     = UI::Color(Idx0+3,  300,  300,  300);
        colors.menu             = UI::Color(Idx0+4, 1000,  435,    0);
        colors.error            = UI::Color(Idx0+5, 1000,    0,    0);
    
    } else {
        // Colorspace: sRGB
        // These colors were derived by sampling the Apple_Terminal values when they're displayed on-screen
        colors.normal           = COLOR_BLACK;
        colors.selectionMove    = UI::Color(Idx0+0,  463,  271, 1000);
        colors.selectionCopy    = UI::Color(Idx0+1,  165, 1000,  114);
        colors.selectionSimilar = UI::Color(Idx0+2,  671,  667, 1000);
        colors.disabledText     = UI::Color(Idx0+3,  486,  486,  486);
        colors.menu             = UI::Color(Idx0+4,  969,  447,  431);
        colors.error            = UI::Color(Idx0+5, 1000,  298,  153);
    }
    
    return colors;
}

static void _CursesInit() noexcept {
    // Default Linux installs may not contain the /usr/share/terminfo database,
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
//    ::cbreak();
    ::raw();
    
    ::use_default_colors();
    ::start_color();
    
    if (can_change_color()) {
        _Colors = _ColorsCreate();
    }
    
    _ColorsPrev = _ColorsSet(_Colors);
    
    // Hide cursor
    ::curs_set(0);
    
    ::mousemask(ALL_MOUSE_EVENTS | REPORT_MOUSE_POSITION, NULL);
    ::mouseinterval(0);
    
    ::set_escdelay(0);
}

static void _CursesDeinit() noexcept {
//    ::mousemask(0, NULL);
    
    if (_ColorsPrev) {
        _ColorsSet(*_ColorsPrev);
        _ColorsPrev = std::nullopt;
    }
    
    ::endwin();
}

extern "C" {
    extern char** environ;
};

static void _Spawn(const char*const* argv) {
    // preserveTerminalCmds: these commands don't modify the terminal, and therefore
    // we don't want to deinit/reinit curses when calling them.
    // When invoking commands such as vi/pico, we need to deinit/reinit curses
    // when calling out to them, because those commands reconfigure the terminal.
    static const std::set<std::string> preserveTerminalCmds = {
        "mate"
    };
    
    const bool preserveTerminal = preserveTerminalCmds.find(argv[0]) != preserveTerminalCmds.end();
    if (!preserveTerminal) _CursesDeinit();
    
    // Spawn the text editor and wait for it to exit
    {
        pid_t pid = -1;
        int ir = posix_spawnp(&pid, argv[0], nullptr, nullptr, (char *const*)argv, environ);
        if (ir) throw Toastbox::RuntimeError("posix_spawnp failed: %s", strerror(ir));
        
        int status = 0;
        ir = 0;
        do ir = waitpid(pid, &status, 0);
        while (ir==-1 && errno==EINTR);
        if (ir == -1) throw Toastbox::RuntimeError("waitpid failed: %s", strerror(errno));
        if (ir != pid) throw Toastbox::RuntimeError("unknown waitpid result: %d", ir);
    }
    
    if (!preserveTerminal) _CursesInit();
}

static void _ExecGitOp(const Git::Op& gitOp) {
    std::string errorMsg;
    try {
        std::optional<Git::OpResult> opResult = Git::Exec<_Spawn>(_Repo, gitOp);
        if (!opResult) return;
        
        Git::Rev srcRevPrev = gitOp.src.rev;
        Git::Rev dstRevPrev = gitOp.dst.rev;
        Git::Rev srcRev = opResult->src.rev;
        Git::Rev dstRev = opResult->dst.rev;
        assert((bool)srcRev.ref == (bool)srcRevPrev.ref);
        assert((bool)dstRev.ref == (bool)dstRevPrev.ref);
        
        if (srcRev && srcRev.commit!=srcRevPrev.commit) {
            State::History& h = _RepoState.history(srcRev.ref);
            h.push({
                .head = State::Convert(srcRev.commit),
                .selection = State::Convert(opResult->src.selection),
                .selectionPrev = State::Convert(opResult->src.selectionPrev),
            });
        }
        
        if (dstRev && dstRev.commit!=dstRevPrev.commit && dstRev.commit!=srcRev.commit) {
            State::History& h = _RepoState.history(dstRev.ref);
            h.push({
                .head = State::Convert(dstRev.commit),
                .selection = State::Convert(opResult->dst.selection),
                .selectionPrev = State::Convert(opResult->dst.selectionPrev),
            });
        }
        
        // Update the selection
        if (opResult->dst.rev) {
            _Selection = {
                .rev = opResult->dst.rev,
                .commits = opResult->dst.selection,
            };
        
        } else {
            _Selection = {
                .rev = opResult->src.rev,
                .commits = opResult->src.selection,
            };
        }
        
        _Reload();
    
    } catch (const Git::Error& e) {
        switch (e.error) {
        case GIT_EUNMERGED:
        case GIT_EMERGECONFLICT:
            errorMsg = "a merge conflict occurred";
            break;
        
        default:
            errorMsg = e.what();
            break;
        }
    
    } catch (const std::exception& e) {
        errorMsg = e.what();
    }
    
    if (!errorMsg.empty()) {
        constexpr int ErrorPanelWidth = 35;
        const int errorPanelWidth = std::min(ErrorPanelWidth, _RootWindow->bounds().size.x);
        
        errorMsg[0] = toupper(errorMsg[0]);
        _ErrorPanel = MakeShared<UI::ErrorPanel>(_Colors, errorPanelWidth, "Error", errorMsg);
    }
}

static void _EventLoop() {
    _CursesInit();
    Defer(_CursesDeinit());
    
    _RootWindow = MakeShared<UI::Window>(::stdscr);
    _Reload();
    
    for (;;) {
        _Draw();
        UI::Event ev = _RootWindow->nextEvent();
        std::optional<Git::Op> gitOp;
        switch (ev) {
        case UI::Event::Mouse: {
            MEVENT mouse = {};
            int ir = ::getmouse(&mouse);
            if (ir != OK) continue;
            
            // If there's an error displayed, the first click should dismiss the error
            if (_ErrorPanel) {
                if (mouse.bstate & BUTTON1_PRESSED) {
                    _ErrorPanel = nullptr;
                }
                continue;
            }
            
            const _HitTestResult hitTest = _HitTest({mouse.x, mouse.y});
            if (mouse.bstate & BUTTON1_PRESSED) {
                const bool shift = (mouse.bstate & _SelectionShiftKeys);
                if (hitTest && !shift) {
                    if (hitTest.hitTest.panel) {
                        // Mouse down inside of a CommitPanel, without shift key
                        gitOp = _TrackMouseInsideCommitPanel(mouse, hitTest.column, hitTest.hitTest.panel);
                    
                    } else {
                        _TrackMouseInsideButton(mouse, hitTest.column, hitTest.hitTest);
                    }
                
                } else {
                    // Mouse down outside of a CommitPanel, or mouse down anywhere with shift key
                    _TrackMouseOutsideCommitPanel(mouse);
                }
            
            } else if (mouse.bstate & BUTTON3_PRESSED) {
                if (hitTest) {
                    if (hitTest.hitTest.panel) {
                        gitOp = _TrackRightMouse(mouse, hitTest.column, hitTest.hitTest.panel);
                    }
                }
            }
            break;
        }
        
        case UI::Event::KeyEscape: {
            // Dismiss error panel if it's open
            _ErrorPanel = nullptr;
            break;
        }
        
        case UI::Event::KeyDelete:
        case UI::Event::KeyFnDelete: {
            if (_Selection.commits.empty() || !_Selection.rev.isMutable()) {
                beep();
                continue;
            }
            
            gitOp = {
                .type = Git::Op::Type::Delete,
                .src = {
                    .rev = _Selection.rev,
                    .commits = _Selection.commits,
                },
            };
            break;
        }
        
        case UI::Event::KeyC: {
            if (_Selection.commits.size()<=1 || !_Selection.rev.isMutable()) {
                beep();
                continue;
            }
            
            gitOp = {
                .type = Git::Op::Type::Combine,
                .src = {
                    .rev = _Selection.rev,
                    .commits = _Selection.commits,
                },
            };
            break;
        }
        
        case UI::Event::KeyReturn: {
            if (_Selection.commits.size()!=1 || !_Selection.rev.isMutable()) {
                beep();
                continue;
            }
            
            gitOp = {
                .type = Git::Op::Type::Edit,
                .src = {
                    .rev = _Selection.rev,
                    .commits = _Selection.commits,
                },
            };
            break;
        }
        
        case UI::Event::WindowResize: {
            _Reload();
            break;
        }
        
        case UI::Event::KeyCtrlC:
        case UI::Event::KeyCtrlD: {
            throw _ExitRequest();
        }
        
        default: {
            break;
        }}
        
        if (gitOp) _ExecGitOp(*gitOp);
    }
}

int main(int argc, const char* argv[]) {
    #warning TODO: fix: if the mouse is moving upon exit, we get mouse characters printed to the terminal
    
    #warning TODO: support light mode
    
    #warning TODO: do lots of testing
    
//  Future:
    
    #warning TODO: move commits away from dragged commits to show where the commits will land
    
    #warning TODO: add column scrolling
    
    #warning TODO: if we can't speed up git operations, show a progress indicator
    
    #warning TODO: figure out why moving/copying commits is slow sometimes
    
//  DONE:
//    #warning TODO: when copying commmits, don't hide the source commits
//    
//    #warning TODO: allow escape key to abort a drag
//    
//    #warning TODO: we need to unique-ify the supplied revs, since we assume that each column has a unique rev
//    #warning       to do so, we'll need to implement operator< on Rev so we can put them in a set
//    
//    #warning TODO: show similar commits to the selected commit using a lighter color
//    
//    #warning TODO: don't allow remote branches to be modified (eg 'origin/master')
//    
//    #warning TODO: create concept of revs being mutable. if they're not mutable, don't allow moves from them (only copies), moves to them, deletes, etc
//    
//    #warning TODO: draw "Move/Copy" text immediately above the dragged commits, instead of at the insertion point
//
//    #warning TODO: implement CombineCommits
//
//    #warning TODO: add affordance to cancel current drag by moving mouse to edge of terminal
//
//    #warning TODO: improve panel dragging along edges. currently the shadow panels get a little messed up
//
//    #warning TODO: fix commit message rendering when there are newlines
//    
//    #warning TODO: configure tty before/after calling out to a new editor
//    
//    #warning TODO: allow editing author/date of commit
//
//    #warning TODO: implement EditCommit
//
//    #warning TODO: special-case opening `mate` when editing commit, to not call CursesDeinit/CursesInit
//
//    #warning TODO: show indication in the UI that a column is immutable
//
//    #warning TODO: implement key combos for combine/edit
//
//    #warning TODO: implement double-click to edit
//
//    #warning TODO: double-click broken: click commit, wait, then double-click
//
//    #warning TODO: implement error messages
//
//    #warning TODO: figure out whether/where/when to call git_libgit2_shutdown()
//
//    #warning TODO: fix: colors aren't restored when exiting
//
//    #warning TODO: if can_change_color() returns false, use default color palette (COLOR_RED, etc)
//
//    #warning TODO: create special color palette for apple terminal
//    
//    #warning TODO: handle rewriting tags
//
//    #warning TODO: don't allow double-click/return key on commits in read-only columns
//
//    #warning TODO: re-evaluate size of drag-cancel affordance since it's not as reliable in iTerm
//
//    #warning TODO: handle window resizing
//
//    #warning TODO: show some kind of indication that a commit is a merge commit
//
//    #warning TODO: properly handle moving/copying merge commits
//
//    #warning TODO: make sure moving/deleting commits near the root commit still works
//    
//    #warning TODO: always show the same contextual menu, but gray-out the disabled options
//
//    #warning TODO: don't allow combine when a merge commit is selected
//
//    #warning TODO: set_escdelay: not sure if we're going to encounter issues?
//
//    #warning TODO: handle merge conflicts
//
//    #warning TODO: bring back _CommitTime so we don't need to worry about the 'sign' field of git_time
//    
//    #warning TODO: make sure we can round-trip with the same date/time. especially test commits with negative UTC offsets!
//
//    #warning TODO: no-op guarantee:
//    #warning TODO:   same commit message -> no repo changes
//    #warning TODO:   dragging commits to their current location -> no repo changes
//
//    #warning TODO: improve error messages: merge conflicts, deleting last branch commit
//
//    #warning TODO: when supplying refs on the command line in the form ref^ or ref~N, can we use a ref-backed rev (instead of using a commit-backed rev), and just offset the RevColumn, so that the rev is mutable?
//
//    #warning TODO: improve error messages when we can't lookup supplied refs
//
//    #warning TODO: fix wrong column being selected after moving commit in master^
//
//    #warning TODO: make sure debase still works when running on a detached HEAD
//    
//    #warning TODO: make "(HEAD)" suffix persist across branch modifications
//
//    #warning TODO: undo: fix UndoHistory deserialization
//    
//    #warning TODO: when deserializing, if a ref doesn't exist, don't throw an exception, just prune the entry
//
//    #warning TODO: support undo/redo
//
//    #warning TODO: fix: copying a commit from a column shouldn't affect column's undo state (but it does)
//    
//    #warning TODO: fix: (1) copy a commit to col A, (2) swap elements 1 & 2 of col A. note how the copied commit doesn't get selected when performing undo/redo
//    
//    #warning TODO: undo: remember selection as a part of the undo state
//
//    #warning TODO: implement _ConfigDir() for real
//
//    #warning TODO: perf: if we aren't modifying the current checked-out branch, don't detach head
//
//    #warning TODO: refuse to run if there are uncomitted changes and we're detaching head
//
//    #warning TODO: show detailed error message if we fail to restore head due to conflicts
//
//    #warning TODO: don't lose the UndoHistory when: debase is run and a ref points to a different commit than we last observed
//    #warning TODO: don't lose the UndoHistory when: a ref no longer exists that we previously observed
//    
//    #warning TODO: write a version number at the root of the StateDir, and check it somewhere
//
//    #warning TODO: use advisory locking in State class to prevent multiple entities from modifying state on disk simultaneously. RepoState class will need to use this facility when writing its state. Whenever we acquire the lock, we should read the version, because the version may have changed since the last read! if so -> bail!
//
//    #warning TODO: test a commit stored in the undo history not existing
//
//    #warning TODO: nevermind: implement log of events, so that if something goes wrong, we can manually get back
//    
//    #warning TODO: backup all supplied revs before doing anything
//
//    #warning TODO: turn snapshots into merely keeping track of what commit was the HEAD, not the full undo history.
//    #warning TODO: when restoring a snapshot, it should just restore the HEAD of the snapshot, and push an undo operation (as any other operation) that the user can undo/redo as normal
//    #warning TODO: further, snapshots should be created only at startup ("Session Start"), and on exit, the snapshot to only be enqueued if it differs from the current HEAD of the repo
//
//    #warning TODO: when creating a new snapshot, if an equivalent one already exists, remove the old one.
//
//    #warning TODO: get command-. working while context menu/snapshot menu is open
//
//    #warning TODO: fix: crash when deleting prefs dir while debase is running
//
//    #warning TODO: show "Session Start" snapshot
//
//    #warning TODO: fully flesh out when to create new snapshots
//
//    #warning TODO: nevermind: only show the current-session marker on the "Session Start" element if there's no other snapshot that has it
//
//    #warning TODO: improve appearance of active snapshot marker in snapshot menu
//
//    #warning TODO: nevermind: figure out how to hide a snapshot if it's the same as the "Session Start" snapshot
//
//    #warning TODO: keep the oldest snapshots, instead of the newest
//
//    #warning TODO: make sure works with submodules
//
//    #warning TODO: nevermind: don't show the first snapshot if it's the same as the "Session Start" snapshot
//
//    #warning TODO: fix: handle case when snapshot menu won't fit entirely on screen
//    
//    #warning TODO: rename all hittest functions -> updateMouse

//    #warning TODO: nevermind: show warning on startup: Take care when rewriting history. As with any software, debase may be bugs. As a safety precaution, debase will automatically backup all branches before modifying them, as <BranchName>-DebaseBackup
    
//    {
//        Git::Commit a;
//        if (a) {
//            a.idStr();
//        }
//    }
//    
//    {
//        volatile bool a = false;
//        while (!a);
//    }
    
    try {
        setlocale(LC_ALL, "");
        
        std::vector<std::string> revNames;
        for (int i=1; i<argc; i++) revNames.push_back(argv[i]);
        
        _Repo = Git::Repo::Open(".");
        _Head = _Repo.head();
        
        if (revNames.empty()) {
            _Revs.emplace_back(_Head);
        
        } else {
            // Unique the supplied revs, because our code assumes a 1:1 mapping between Revs and RevColumns
            std::set<Git::Rev> unique;
            for (const std::string& revName : revNames) {
                Git::Rev rev;
                try {
                    rev = _Repo.revLookup(revName);
                } catch (...) {
                    throw Toastbox::RuntimeError("invalid rev: %s", revName.c_str());
                }
                
                if (unique.find(rev) == unique.end()) {
                    _Revs.push_back(rev);
                    unique.insert(rev);
                }
            }
        }
        
        // Create _RepoState
        std::set<Git::Ref> refs;
        for (Git::Rev rev : _Revs) {
            if (rev.ref) refs.insert(rev.ref);
        }
        _RepoState = State::RepoState(StateDir(), _Repo, refs);
        
        // Determine if we need to detach head.
        // This is required when a ref (ie a branch or tag) is checked out, and the ref is specified in _Revs.
        // In other words, we need to detach head when whatever is checked-out may be modified.
        bool detachHead = _Head.ref && std::find(_Revs.begin(), _Revs.end(), _Head)!=_Revs.end();
        
        if (detachHead && _Repo.dirty()) {
            throw Toastbox::RuntimeError("please commit or stash your outstanding changes before running debase on %s", _Head.displayName().c_str());
        }
        
        // Detach HEAD if it's attached to a ref, otherwise we'll get an error if
        // we try to replace that ref.
        if (detachHead) _Repo.headDetach();
        Defer(
            if (detachHead) {
                // Restore previous head on exit
                std::cout << "Restoring HEAD to " << _Head.ref.name() << std::endl;
                std::string err;
                try {
                    _Repo.checkout(_Head.ref);
                } catch (const Git::ConflictError& e) {
                    err = "Error: checkout failed because these untracked files would be overwritten:\n";
                    for (const fs::path& path : e.paths) {
                        err += "  " + std::string(path) + "\n";
                    }
                    
                    err += "\n";
                    err += "Please move or delete them and run:\n";
                    err += "  git checkout " + _Head.ref.name() + "\n";
                
                } catch (const std::exception& e) {
                    err = std::string("Error: ") + e.what();
                }
                
                std::cout << (!err.empty() ? err : "Done") << std::endl;
            }
        );
        
        try {
            _EventLoop();
        } catch (const _ExitRequest&) {
            // Nothing to do
        } catch (...) {
            throw;
        }
        
        _RepoState.write();
    
    } catch (const std::exception& e) {
        fprintf(stderr, "Error: %s\n", e.what());
        return 1;
    }
    
    return 0;
}
