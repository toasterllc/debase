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
#include "lib/toastbox/RuntimeError.h"
#include "lib/toastbox/Defer.h"
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
#include "MessagePanel.h"
#include "RegisterPanel.h"
#include "State.h"
#include "StateDir.h"
#include "SnapshotButton.h"
#include "SnapshotMenu.h"
#include "Terminal.h"
#include "Debase.h"
#include "CursorState.h"

namespace fs = std::filesystem;

struct _Selection {
    Git::Rev rev;
    std::set<Git::Commit> commits;
};

static State::Theme _Theme = State::Theme::None;

static UI::ColorPalette _Colors;
static UI::ColorPalette _ColorsPrev;
static UI::CursorState _CursorState;

static Git::Repo _Repo;
static State::RepoState _RepoState;
static Git::Rev _Head;
static std::vector<Git::Rev> _Revs;
//static std::map<Git::Ref,RefHistory> _RefHistory;

static UI::WindowPtr _RootWindow;
static std::vector<UI::RevColumnPtr> _Columns;

static struct {
    UI::CommitPanelPtr titlePanel;
    std::vector<UI::BorderedPanelPtr> shadowPanels;
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

static UI::MenuPtr _ContextMenu;
static UI::SnapshotMenuPtr _SnapshotsMenu;

static UI::MessagePanelPtr _MessagePanel;
static UI::RegisterPanelPtr _RegisterPanel;

static constexpr mmask_t _SelectionShiftKeys = BUTTON_CTRL | BUTTON_SHIFT;

enum class _SelectState {
    False,
    True,
    Similar,
};

static _SelectState _SelectStateGet(UI::RevColumnPtr col, UI::CommitPanelPtr panel) {
    bool similar = _Selection.commits.find(panel->commit()) != _Selection.commits.end();
    if (!similar) return _SelectState::False;
    return (col->rev==_Selection.rev ? _SelectState::True : _SelectState::Similar);
}

static bool _Selected(UI::RevColumnPtr col, UI::CommitPanelPtr panel) {
    return _SelectStateGet(col, panel) == _SelectState::True;
}

static void _Draw() {
    const UI::Color selectionColor = (_Drag.copy ? _Colors.selectionCopy : _Colors.selection);
    
    // Update panels
    {
        if (_Drag.titlePanel) {
            _Drag.titlePanel->setBorderColor(selectionColor);
            
            for (UI::BorderedPanelPtr panel : _Drag.shadowPanels) {
                panel->setBorderColor(selectionColor);
            }
        }
        
        for (UI::RevColumnPtr col : _Columns) {
            col->layout();
        }
        
        bool dragging = (bool)_Drag.titlePanel;
        bool copying = _Drag.copy;
        for (UI::RevColumnPtr col : _Columns) {
            for (UI::CommitPanelPtr panel : col->panels) {
                bool visible = false;
                std::optional<UI::Color> borderColor;
                _SelectState selectState = _SelectStateGet(col, panel);
                if (selectState == _SelectState::True) {
                    visible = !dragging || copying;
                    if (dragging) borderColor = _Colors.selectionSimilar;
                    else          borderColor = _Colors.selection;
                
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
            _ContextMenu->containerSize = _RootWindow->bounds().size;
            _ContextMenu->layout();
            _ContextMenu->orderFront();
        }
        
        if (_SnapshotsMenu) {
            _SnapshotsMenu->containerSize = _RootWindow->bounds().size;
            _SnapshotsMenu->layout();
            _SnapshotsMenu->orderFront();
        }
        
        if (_MessagePanel) {
            _MessagePanel->layout();
            
            UI::Size ps = _MessagePanel->frame().size;
            UI::Size rs = _RootWindow->frame().size;
            UI::Point p = {
                (rs.x-ps.x)/2,
                (rs.y-ps.y)/3,
            };
            _MessagePanel->setPosition(p);
            _MessagePanel->orderFront();
        }
        
        if (_RegisterPanel) {
            constexpr int RegisterPanelWidth = 50;
            _RegisterPanel->width = std::min(RegisterPanelWidth, _RootWindow->bounds().size.x);
            _RegisterPanel->layout();
            
            UI::Size ps = _RegisterPanel->frame().size;
            UI::Size rs = _RootWindow->frame().size;
            UI::Point p = {
                (rs.x-ps.x)/2,
                (rs.y-ps.y)/3,
            };
            _RegisterPanel->setPosition(p);
            _RegisterPanel->orderFront();
        }
    }
    
    // Draw everything
    {
        _RootWindow->erase();
        
        if (_Drag.titlePanel) {
            _Drag.titlePanel->draw();
            
            // Draw insertion marker
            if (_Drag.insertionMarker) {
                UI::Window::Attr color = _RootWindow->attr(selectionColor);
                _RootWindow->drawLineHoriz(_Drag.insertionMarker->point, _Drag.insertionMarker->size.x);
            }
        }
        
        for (UI::BorderedPanelPtr panel : _Drag.shadowPanels) {
            panel->draw();
        }
        
        if (_SelectionRect) {
            UI::Window::Attr color = _RootWindow->attr(_Colors.selection);
            _RootWindow->drawRect(*_SelectionRect);
        }
        
        for (UI::RevColumnPtr col : _Columns) {
            col->draw(*_RootWindow);
        }
        
        if (_ContextMenu) {
            _ContextMenu->draw();
        }
        
        if (_SnapshotsMenu) {
            _SnapshotsMenu->draw();
        }
        
        if (_MessagePanel) {
            _MessagePanel->draw();
        }
        
        if (_RegisterPanel) {
            _RegisterPanel->draw();
        }
        
//        curs_set(1);
//        move(5, 5);
////        wmove(*_RootWindow, 5, 5);
        
        UI::Redraw();
    }
}

struct _HitTestResult {
    UI::RevColumnPtr column;
    UI::RevColumn::HitTestResult hitTest;
    operator bool() const {
        return (bool)column;
    }
};

static _HitTestResult _HitTest(const UI::Point& p) {
    // Don't bail when we get a hit -- we need to update the UI for all
    // columns (by calling hitTest) before returning
    _HitTestResult hit;
    for (UI::RevColumnPtr col : _Columns) {
        UI::RevColumn::HitTestResult h = col->updateMouse(p);
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
    UI::RevColumnPtr col;
    UI::CommitPanelVecIter iter;
};

static std::optional<_InsertionPosition> _FindInsertionPosition(const UI::Point& p) {
    UI::RevColumnPtr icol;
    UI::CommitPanelVecIter iiter;
    std::optional<int> leastDistance;
    for (UI::RevColumnPtr col : _Columns) {
        UI::CommitPanelVec& panels = col->panels;
        // Ignore empty columns (eg if the window is too small to fit a column, it may not have any panels)
        if (panels.empty()) continue;
        const UI::Rect lastFrame = panels.back()->frame();
        const int midX = lastFrame.point.x + lastFrame.size.x/2;
        const int endY = lastFrame.point.y + lastFrame.size.y;
        
        for (auto it=panels.begin();; it++) {
            UI::CommitPanelPtr panel = (it!=panels.end() ? *it : nullptr);
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
    if (!icol->rev.isMutable()) return std::nullopt;
    
    // Adjust the insert point so that it doesn't occur within a selection
    UI::CommitPanelVec& icolPanels = icol->panels;
    for (;;) {
        if (iiter == icolPanels.begin()) break;
        UI::CommitPanelPtr prevPanel = *std::prev(iiter);
        if (!_Selected(icol, prevPanel)) break;
        iiter--;
    }
    
    return _InsertionPosition{icol, iiter};
}

static UI::RevColumnPtr _ColumnForRev(Git::Rev rev) {
    for (UI::RevColumnPtr col : _Columns) {
        if (col->rev == rev) return col;
    }
    // Programmer error if it doesn't exist
    abort();
}

static UI::CommitPanelPtr _PanelForCommit(UI::RevColumnPtr col, Git::Commit commit) {
    for (UI::CommitPanelPtr panel : col->panels) {
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
        UI::RevColumnPtr col = MakeShared<UI::RevColumnPtr>(_Colors);
        col->containerBounds    = _RootWindow->bounds();
        col->repo               = _Repo;
        col->rev                = rev;
        col->head               = (rev.displayHead() == _Head.commit);
        col->offset             = UI::Size{OffsetX, 0};
        col->width              = ColumnWidth;
        col->undoEnabled        = (h ? !h->begin() : false);
        col->redoEnabled        = (h ? !h->end() : false);
        col->snapshotsEnabled   = true;
        _Columns.push_back(col);
        
        OffsetX += ColumnWidth+ColumnSpacing;
    }
}

static void _UndoRedo(UI::RevColumnPtr col, bool undo) {
    Git::Rev rev = col->rev;
    State::History& h = _RepoState.history(col->rev.ref);
    State::RefState refStatePrev = h.get();
    State::RefState refState = (undo ? h.prevPeek() : h.nextPeek());
    
    try {
        Git::Commit commit;
        try {
            commit = State::Convert(_Repo, refState.head);
        } catch (...) {
            throw Toastbox::RuntimeError("failed to find commit %s", refState.head.c_str());
        }
        
        std::set<Git::Commit> selection = State::Convert(_Repo, (!undo ? refState.selection : refStatePrev.selectionPrev));
        rev = _Repo.revReplace(rev, commit);
        _Selection = {
            .rev = rev,
            .commits = selection,
        };
        
        if (undo) h.prev();
        else      h.next();
    
    } catch (const std::exception& e) {
        if (undo) throw Toastbox::RuntimeError("undo failed: %s", e.what());
        else      throw Toastbox::RuntimeError("redo failed: %s", e.what());
    }
    
    _Reload();
}

// _TrackMouseInsideCommitPanel
// Handles clicking/dragging a set of CommitPanels
static std::optional<Git::Op> _TrackMouseInsideCommitPanel(const UI::Event& mouseDownEvent, UI::RevColumnPtr mouseDownColumn, UI::CommitPanelPtr mouseDownPanel) {
    const UI::Rect mouseDownPanelFrame = mouseDownPanel->frame();
    const UI::Size mouseDownOffset = mouseDownPanelFrame.point - mouseDownEvent.mouse.point;
    const bool wasSelected = _Selected(mouseDownColumn, mouseDownPanel);
    const UI::Rect rootWinBounds = _RootWindow->bounds();
    const auto doubleClickStatePrev = _DoubleClickState;
    _DoubleClickState = {};
    
    // Reset the selection to solely contain the mouse-down CommitPanel if:
    //   - there's no selection; or
    //   - the mouse-down CommitPanel is in a different column than the current selection; or
    //   - an unselected CommitPanel was clicked
    if (_Selection.commits.empty() || (_Selection.rev != mouseDownColumn->rev) || !wasSelected) {
        _Selection = {
            .rev = mouseDownColumn->rev,
            .commits = {mouseDownPanel->commit()},
        };
    
    } else {
        assert(!_Selection.commits.empty() && (_Selection.rev == mouseDownColumn->rev));
        _Selection.commits.insert(mouseDownPanel->commit());
    }
    
    UI::Event ev = mouseDownEvent;
    std::optional<_InsertionPosition> ipos;
    bool abort = false;
    bool mouseDragged = false;
    for (;;) {
        assert(!_Selection.commits.empty());
        UI::RevColumnPtr selectionColumn = _ColumnForRev(_Selection.rev);
        
        const UI::Point p = ev.mouse.point;
        const UI::Size delta = mouseDownEvent.mouse.point-p;
        const int w = std::abs(delta.x);
        const int h = std::abs(delta.y);
        // allow: cancel drag when mouse is moved to the edge (as an affordance to the user)
        const bool allow = UI::HitTest(rootWinBounds, p, {-3,-3});
        mouseDragged |= w>1 || h>1;
        
        // Find insertion position
        ipos = _FindInsertionPosition(p);
        
        if (!_Drag.titlePanel && mouseDragged && allow) {
            Git::Commit titleCommit = _FindLatestCommit(_Selection.rev.commit, _Selection.commits);
            UI::CommitPanelPtr titlePanel = _PanelForCommit(selectionColumn, titleCommit);
            _Drag.titlePanel = MakeShared<UI::CommitPanelPtr>(_Colors, true, titlePanel->frame().size.x, titleCommit);
            
            // Create shadow panels
            UI::Size shadowSize = _Drag.titlePanel->frame().size;
            for (size_t i=0; i<_Selection.commits.size()-1; i++) {
                _Drag.shadowPanels.push_back(MakeShared<UI::BorderedPanelPtr>(shadowSize));
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
                const bool forceCopy = !selectionColumn->rev.isMutable();
                const bool copy = (ev.mouse.bstate & BUTTON_ALT) || forceCopy;
                _Drag.copy = copy;
                _Drag.titlePanel->setHeaderLabel(_Drag.copy ? "Copy" : "Move");
            }
            
            // Position title panel / shadow panels
            {
                const UI::Point pos0 = p + mouseDownOffset;
                
                _Drag.titlePanel->setPosition(pos0);
                
                // Position shadowPanels
                int off = 1;
                for (UI::PanelPtr panel : _Drag.shadowPanels) {
                    const UI::Point pos = pos0+off;
                    panel->setPosition(pos);
                    off++;
                }
            }
            
            // Update insertion marker
            if (ipos) {
                constexpr int InsertionExtraWidth = 6;
                UI::CommitPanelVec& ipanels = ipos->col->panels;
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
        ev = _RootWindow->nextEvent();
        abort = (ev.type != UI::Event::Type::Mouse);
        // Check if we should abort
        if (abort || ev.mouseUp()) {
            break;
        }
    }
    
    std::optional<Git::Op> gitOp;
    if (!abort) {
        if (_Drag.titlePanel && ipos) {
            Git::Commit dstCommit = ((ipos->iter != ipos->col->panels.end()) ? (*ipos->iter)->commit() : nullptr);
            gitOp = Git::Op{
                .type = (_Drag.copy ? Git::Op::Type::Copy : Git::Op::Type::Move),
                .src = {
                    .rev = _Selection.rev,
                    .commits = _Selection.commits,
                },
                .dst = {
                    .rev = ipos->col->rev,
                    .position = dstCommit,
                }
            };
        
        // If this was a mouse-down + mouse-up without dragging in between,
        // set the selection to the commit that was clicked
        } else if (!mouseDragged) {
            _Selection = {
                .rev = mouseDownColumn->rev,
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
static void _TrackMouseOutsideCommitPanel(const UI::Event& mouseDownEvent) {
    auto selectionOld = _Selection;
    UI::Event ev = mouseDownEvent;
    
    for (;;) {
        const int x = std::min(mouseDownEvent.mouse.point.x, ev.mouse.point.x);
        const int y = std::min(mouseDownEvent.mouse.point.y, ev.mouse.point.y);
        const int w = std::abs(mouseDownEvent.mouse.point.x - ev.mouse.point.x);
        const int h = std::abs(mouseDownEvent.mouse.point.y - ev.mouse.point.y);
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
            for (UI::RevColumnPtr col : _Columns) {
                for (UI::CommitPanelPtr panel : col->panels) {
                    if (!Empty(Intersection(selectionRect, panel->frame()))) {
                        selectionNew.rev = col->rev;
                        selectionNew.commits.insert(panel->commit());
                    }
                }
                if (!selectionNew.commits.empty()) break;
            }
            
            const bool shift = (mouseDownEvent.mouse.bstate & _SelectionShiftKeys);
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
        ev = _RootWindow->nextEvent();
        // Check if we should abort
        if (ev.type!=UI::Event::Type::Mouse || ev.mouseUp()) {
            break;
        }
    }
    
    // Reset state
    {
        _SelectionRect = std::nullopt;
    }
}

static UI::ButtonPtr _MakeContextMenuButton(std::string_view label, std::string_view key, bool enabled) {
    constexpr int ContextMenuWidth = 12;
    UI::ButtonPtr b = std::make_shared<UI::Button>(_Colors);
    b->label          = std::string(label);
    b->key            = std::string(key);
    b->enabled        = enabled;
    b->insetX         = 0;
    b->frame.size.x   = ContextMenuWidth;
    b->frame.size.y   = 1;
    return b;
}

static std::optional<Git::Op> _TrackRightMouse(const UI::Event& mouseDownEvent, UI::RevColumnPtr mouseDownColumn, UI::CommitPanelPtr mouseDownPanel) {
    auto mouseDownTime = std::chrono::steady_clock::now();
    UI::Event ev = mouseDownEvent;
    
    // If the commit that was clicked isn't selected, set the selection to only that commit
    if (!_Selected(mouseDownColumn, mouseDownPanel)) {
        _Selection = {
            .rev = mouseDownColumn->rev,
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
    UI::ButtonPtr combineButton = _MakeContextMenuButton("Combine", "c", combineEnabled);
    UI::ButtonPtr editButton    = _MakeContextMenuButton("Edit", "ret", editEnabled);
    UI::ButtonPtr deleteButton  = _MakeContextMenuButton("Delete", "del", deleteEnabled);
    std::vector<UI::ButtonPtr> buttons = { combineButton, editButton, deleteButton };
    _ContextMenu = MakeShared<UI::MenuPtr>(_Colors);
    _ContextMenu->buttons = buttons;
    _ContextMenu->setPosition(mouseDownEvent.mouse.point);
    
    UI::ButtonPtr menuButton = nullptr;
    UI::Event::MouseButtons mouseUpButtons = UI::Event::MouseButtons::Right;
    bool abort = false;
    for (;;) {
        if (ev.type == UI::Event::Type::Mouse) {
            menuButton = _ContextMenu->updateMouse(ev.mouse.point);
        } else {
            menuButton = nullptr;
        }
        
        _Draw();
        ev = _RootWindow->nextEvent();
        abort = (ev.type != UI::Event::Type::Mouse);
        
        // Check if we should abort
        if (abort) {
            break;
        
        // Handle mouse up
        } else if (ev.mouseUp(mouseUpButtons)) {
            if (!(mouseUpButtons & UI::Event::MouseButtons::Left)) {
                // If the right-mouse-up occurs soon enough after right-mouse-down, the menu should
                // stay open and we should start listening for left-mouse-down events.
                // If the right-mouse-up occurs af
                auto duration = std::chrono::steady_clock::now()-mouseDownTime;
                if (duration >= _ContextMenuStayOpenThresh) break;
                
                // Start listening for left mouse up
                mouseUpButtons |= UI::Event::MouseButtons::Left;
                
                // Right mouse up, but menu stays open
                // Now start tracking both left+right mouse down
            } else {
                // Close the menu only if clicking outside of the menu, or clicking on an
                // enabled menu button.
                // In other words, don't close the menu when clicking on a disabled menu
                // button.
                if (!menuButton || menuButton->enabled) {
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

static UI::ButtonPtr _MakeSnapshotMenuButton(Git::Repo repo, Git::Ref ref, const State::Snapshot& snap, bool sessionStart) {
    bool activeSnapshot = State::Convert(ref.commit()) == snap.head;
    
//    UI::Button::Options buttonOpts = {
//        .enabled = true,
//    };
//    
//    UI::SnapshotButton::Options snapButtonOpts = {
//        .repo           = repo,
//        .snapshot       = snap,
//        .width          = _SnapshotMenuWidth,
//        .activeSnapshot = activeSnapshot,
//    };
    
    UI::SnapshotButtonPtr b = std::make_shared<UI::SnapshotButton>(_Colors, repo, snap, _SnapshotMenuWidth);
    b->enabled = true;
    b->activeSnapshot = activeSnapshot;
    return b;
}

static void _TrackSnapshotsMenu(UI::RevColumnPtr column) {
    Git::Ref ref = column->rev.ref;
    std::vector<UI::ButtonPtr> buttons = {
        _MakeSnapshotMenuButton(_Repo, ref, _RepoState.initialSnapshot(ref), true),
    };
    
    const std::vector<State::Snapshot>& snapshots = _RepoState.snapshots(ref);
    for (auto it=snapshots.rbegin(); it!=snapshots.rend(); it++) {
        // Creating the button will throw if we can't get the commit for the snapshot
        // If that happens, just don't shown the button representing the snapshot
        try {
            buttons.push_back(_MakeSnapshotMenuButton(_Repo, ref, *it, false));
        } catch (...) {}
    }
    
    const int width = _SnapshotMenuWidth+UI::SnapshotMenu::Padding().x;
    const int px = column->offset.x + (column->width-width)/2;
    _SnapshotsMenu = MakeShared<UI::SnapshotMenuPtr>(_Colors);
    _SnapshotsMenu->title = "Session Start";
    _SnapshotsMenu->buttons = buttons;
    _SnapshotsMenu->allowTruncate = true;
    _SnapshotsMenu->setPosition({px, 2});
    
    UI::SnapshotButtonPtr menuButton;
    bool abort = false;
    for (;;) {
        _Draw();
        
        UI::Event ev = _RootWindow->nextEvent();
        abort = (ev.type != UI::Event::Type::Mouse);
        
        if (ev.type == UI::Event::Type::Mouse) {
            menuButton = std::dynamic_pointer_cast<UI::SnapshotButton>(_SnapshotsMenu->updateMouse(ev.mouse.point));
        } else {
            menuButton = nullptr;
        }
        
        // Check if we should abort
        if (abort) {
            break;
        
        // Handle mouse up
        } else if (ev.mouseUp()) {
            // Close the menu only if clicking outside of the menu, or clicking on an
            // enabled menu button.
            // In other words, don't close the menu when clicking on a disabled menu
            // button.
            if (!menuButton || menuButton->enabled) {
                break;
            }
        }
    }
    
    if (!abort && menuButton) {
        State::History& h = _RepoState.history(ref);
        State::Commit commitNew = menuButton->snapshot.head;
        State::Commit commitCur = h.get().head;
        
        if (commitNew != commitCur) {
            Git::Commit commit = Convert(_Repo, commitNew);
            h.push(State::RefState{.head = commitNew});
            _Repo.refReplace(ref, commit);
            // Clear the selection when restoring a snapshot
            _Selection = {};
            _Reload();
        }
    }
    
    // Reset state
    {
        _SnapshotsMenu = nullptr;
    }
}

static void _TrackMouseInsideButton(const UI::Event& mouseDownEvent, UI::RevColumnPtr column, const UI::RevColumn::HitTestResult& mouseDownHitTest) {
    UI::RevColumn::HitTestResult hit;
    UI::Event ev = mouseDownEvent;
    
    for (;;) {
        if (ev.type == UI::Event::Type::Mouse) {
            hit = column->updateMouse(ev.mouse.point);
        } else {
            hit = {};
        }
        
        _Draw();
        ev = _RootWindow->nextEvent();
        if (ev.mouseUp()) break;
    }
    
    if (hit.buttonEnabled && hit==mouseDownHitTest) {
        switch (hit.button) {
        case UI::RevColumn::Button::Undo:
            _UndoRedo(column, true);
            break;
        case UI::RevColumn::Button::Redo:
            _UndoRedo(column, false);
            break;
        case UI::RevColumn::Button::Snapshots:
            _TrackSnapshotsMenu(column);
            break;
        default:
            break;
        }
    }
}

static UI::Color _ColorSet(const UI::Color& c) {
    UI::Color prev(c.idx);
    color_content(prev.idx, &prev.r, &prev.g, &prev.b);
    ::init_color(c.idx, c.r, c.g, c.b);
    return prev;
}

static UI::ColorPalette _ColorsSet(const UI::ColorPalette& p) {
    UI::ColorPalette pcopy = p;
    
    // Set the values for the custom colors, and remember the old values
    for (UI::Color& c : pcopy.custom()) {
        c = _ColorSet(c);
    }
    
    for (const UI::Color& c : p.colors()) {
        ::init_pair(c.idx, c.idx, -1);
    }
    
    return pcopy;
}

static UI::ColorPalette _ColorsCreate(State::Theme theme) {
    const std::string termProg = getenv("TERM_PROGRAM");
    const bool themeDark = (theme==State::Theme::None || theme == State::Theme::Dark);
    
    UI::ColorPalette colors;
    UI::Color black;
    UI::Color gray;
    UI::Color purple;
    UI::Color purpleLight;
    UI::Color greenMint;
    UI::Color red;
    
    if (termProg == "Apple_Terminal") {
        // Colorspace: unknown
        // There's no simple relation between these numbers and the resulting colors because Apple's
        // Terminal.app applies some kind of filtering on top of these numbers. These values were
        // manually chosen based on their appearance.
        if (themeDark) {
            colors.normal           = COLOR_BLACK;
            colors.dimmed           = colors.add( 77,  77,  77);
            colors.selection        = colors.add(  0,   2, 255);
            colors.selectionSimilar = colors.add(140, 140, 255);
            colors.selectionCopy    = colors.add(  0, 229, 130);
            colors.menu             = colors.selectionCopy;
            colors.error            = colors.add(194,   0,  71);
        
        } else {
            colors.normal           = COLOR_BLACK;
            colors.dimmed           = colors.add(128, 128, 128);
            colors.selection        = colors.add(  0,   2, 255);
            colors.selectionSimilar = colors.add(140, 140, 255);
            colors.selectionCopy    = colors.add( 52, 167,   0);
            colors.menu             = colors.add(194,   0,  71);
            colors.error            = colors.menu;
        }
    
    } else {
        // Colorspace: sRGB
        // These colors were derived by sampling the Apple_Terminal values when they're displayed on-screen
        
        if (themeDark) {
            colors.normal           = COLOR_BLACK;
            colors.dimmed           = colors.add(.486*255, .486*255, .486*255);
            colors.selection        = colors.add(.463*255, .275*255, 1.00*255);
            colors.selectionSimilar = colors.add(.663*255, .663*255, 1.00*255);
            colors.selectionCopy    = colors.add(.204*255, .965*255, .569*255);
            colors.menu             = colors.selectionCopy;
            colors.error            = colors.add(.969*255, .298*255, .435*255);
        
        } else {
            colors.normal           = COLOR_BLACK;
            colors.dimmed           = colors.add(.592*255, .592*255, .592*255);
            colors.selection        = colors.add(.369*255, .208*255, 1.00*255);
            colors.selectionSimilar = colors.add(.627*255, .627*255, 1.00*255);
            colors.selectionCopy    = colors.add(.306*255, .737*255, .153*255);
            colors.menu             = colors.add(.969*255, .298*255, .435*255);
            colors.error            = colors.menu;
        }
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
    ::raw();
    
    ::use_default_colors();
    ::start_color();
    
    if (can_change_color()) {
        _Colors = _ColorsCreate(_Theme);
    }
    
    _ColorsPrev = _ColorsSet(_Colors);
    
    _CursorState = UI::CursorState(false, {});
    
//    // Hide cursor
//    ::curs_set(0);
    
    ::mousemask(ALL_MOUSE_EVENTS | REPORT_MOUSE_POSITION, NULL);
    ::mouseinterval(0);
    
    ::set_escdelay(0);
}

static void _CursesDeinit() noexcept {
//    ::mousemask(0, NULL);
    
    _CursorState.restore();
    
    _ColorsSet(_ColorsPrev);
    ::endwin();
    
//    sleep(1);
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
}

static void _EventLoop() {
    _CursesInit();
    Defer(_CursesDeinit());
    
    _RootWindow = MakeShared<UI::WindowPtr>(::stdscr);
    
    
    
    
    
//    {
//        _RegisterPanel = MakeShared<UI::RegisterPanelPtr>(_Colors);
//        _RegisterPanel->color           = _Colors.menu;
//        _RegisterPanel->messageInsetY   = 1;
//        _RegisterPanel->center          = false;
//        _RegisterPanel->title           = "Register";
//        _RegisterPanel->message         = "Please register debase";
//    }
    
    
    
    
    
    _Reload();
    
    for (;;) {
        std::string errorMsg;
        _Draw();
        UI::Event ev = _RootWindow->nextEvent();
        
        try {
            // If we have a modal panel, let it handle the event
            if (_MessagePanel) {
                ev = _MessagePanel->handleEvent(_MessagePanel->convert(ev));
            
            } else if (_RegisterPanel) {
                ev = _RegisterPanel->handleEvent(_RegisterPanel->convert(ev));
            }
            
            std::optional<Git::Op> gitOp;
            switch (ev.type) {
            case UI::Event::Type::Mouse: {
                // If _MessagePanel is displayed, the first click should dismiss the error
                // Note that _MessagePanel eats all mouse events except mouse-up, so we
                // don't have to check for the type
                if (_MessagePanel) {
                    _MessagePanel = nullptr;
                    break;
                }
                
                const _HitTestResult hitTest = _HitTest(ev.mouse.point);
                if (ev.mouseDown(UI::Event::MouseButtons::Left)) {
                    const bool shift = (ev.mouse.bstate & _SelectionShiftKeys);
                    if (hitTest && !shift) {
                        if (hitTest.hitTest.panel) {
                            // Mouse down inside of a CommitPanel, without shift key
                            gitOp = _TrackMouseInsideCommitPanel(ev, hitTest.column, hitTest.hitTest.panel);
                        
                        } else {
                            _TrackMouseInsideButton(ev, hitTest.column, hitTest.hitTest);
                        }
                    
                    } else {
                        // Mouse down outside of a CommitPanel, or mouse down anywhere with shift key
                        _TrackMouseOutsideCommitPanel(ev);
                    }
                
                } else if (ev.mouseDown(UI::Event::MouseButtons::Right)) {
                    if (hitTest) {
                        if (hitTest.hitTest.panel) {
                            gitOp = _TrackRightMouse(ev, hitTest.column, hitTest.hitTest.panel);
                        }
                    }
                }
                break;
            }
            
            case UI::Event::Type::KeyEscape: {
                // Dismiss _MessagePanel if it's open
                _MessagePanel = nullptr;
                break;
            }
            
            case UI::Event::Type::KeyDelete:
            case UI::Event::Type::KeyFnDelete: {
                if (_Selection.commits.empty() || !_Selection.rev.isMutable()) {
                    beep();
                    break;
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
            
            case UI::Event::Type::KeyC: {
                if (_Selection.commits.size()<=1 || !_Selection.rev.isMutable()) {
                    beep();
                    break;
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
            
            case UI::Event::Type::KeyReturn: {
                if (_Selection.commits.size()!=1 || !_Selection.rev.isMutable()) {
                    beep();
                    break;
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
            
            case UI::Event::Type::WindowResize: {
                _Reload();
                break;
            }
            
            default: {
                break;
            }}
            
            if (gitOp) _ExecGitOp(*gitOp);
        
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
        
        } catch (const UI::ExitRequest&) {
            throw;
        
        } catch (const std::exception& e) {
            errorMsg = e.what();
        }
        
        if (!errorMsg.empty()) {
            constexpr int MessagePanelWidth = 35;
            const int errorPanelWidth = std::min(MessagePanelWidth, _RootWindow->bounds().size.x);
            
            errorMsg[0] = toupper(errorMsg[0]);
            
            _MessagePanel = MakeShared<UI::MessagePanelPtr>(_Colors);
            _MessagePanel->color    = _Colors.error;
            _MessagePanel->width    = errorPanelWidth;
            _MessagePanel->center   = true;
            _MessagePanel->title    = "Error";
            _MessagePanel->message  = errorMsg;
        }
    }
}

static State::Theme _ThemeRead() {
    State::State state(StateDir());
    State::Theme theme = state.theme();
    if (theme != State::Theme::None) return theme;
    
    bool write = false;
    // If a theme isn't set, ask the terminal for its background color,
    // and we'll choose the theme based on that
    Terminal::Background bg = Terminal::Background::Dark;
    try {
        bg = Terminal::BackgroundGet();
    } catch (...) {
        // We failed to get the terminal background color, so write the theme
        // for the default background color to disk, so we don't try to get
        // the background color again in the future. (This avoids a timeout
        // delay in Terminal::BackgroundGet() that occurs if the terminal
        // doesn't support querying the background color.)
        write = true;
    }
    
    switch (bg) {
    case Terminal::Background::Dark:    theme = State::Theme::Dark; break;
    case Terminal::Background::Light:   theme = State::Theme::Light; break;
    }
    
    if (write) {
        state.theme(theme);
        state.write();
    }
    return theme;
}

static void _ThemeWrite(State::Theme theme) {
    State::State state(StateDir());
    state.theme(theme);
    state.write();
}

struct _Args {
    struct {
        bool en = false;
    } help;
    
    struct {
        bool en = false;
        std::string theme;
    } setTheme;
    
    struct {
        bool en = false;
        std::vector<std::string> revs;
    } normal;
};

static _Args _ParseArgs(int argc, const char* argv[]) {
    using namespace Toastbox;
    
    std::vector<std::string> strs;
    for (int i=0; i<argc; i++) strs.push_back(argv[i]);
    
    _Args args;
    if (strs.size() < 1) {
        return _Args{ .normal = {.en = true}, };
    }
    
    if (strs[0]=="-h" || strs[0]=="--help") {
        return _Args{ .help = {.en = true}, };
    }
    
    if (strs[0] == "--theme") {
        if (strs.size() < 2) throw std::runtime_error("no theme specified");
        if (strs.size() > 2) throw std::runtime_error("too many arguments supplied");
        return _Args{
            .setTheme = {
                .en = true,
                .theme = strs[1],
            },
        };
    }
    
    return _Args{
        .normal = {
            .en = true,
            .revs = strs,
        },
    };
}

static void _PrintUsage() {
    using namespace std;
    cout << "debase version " DebaseVersionString "\n";
    cout << "\n";
    cout << "Usage:\n";
    cout << "  -h, --help\n";
    cout << "      Print this help message\n";
    cout << "\n";
    cout << "  --theme <auto|dark|light>\n";
    cout << "      Set theme\n";
    cout << "\n";
    cout << "  [<rev>...]\n";
    cout << "      Open the specified git revisions in debase\n";
    cout << "\n";
}

int main(int argc, const char* argv[]) {
    #warning TODO: switch Button to follow TextField handleEvent() model
    
    #warning TODO: switch Menu to follow TextField handleEvent() model
    
    #warning TODO: try to optimize drawing. maybe draw using a random color so we can tell when things refresh?
    
    #warning TODO: implement 7-day trial
    
    #warning TODO: implement registration
    
    #warning TODO: do lots of testing
    
//  Future:
    
    #warning TODO: move commits away from dragged commits to show where the commits will land
    
    #warning TODO: add column scrolling
    
    #warning TODO: if we can't speed up git operations, show a progress indicator
    
    #warning TODO: figure out why moving/copying commits is slow sometimes
    
//  DONE:
//    #warning TODO: switch classes to not be shared_ptr where possible. particularly Window.
//
//    #warning TODO: TextField: don't lose content when resizing window
//
//    #warning TODO: TextField: filter printable characters better
//
//    #warning TODO: TextField: make clicking to reposition cursor work
//
//    #warning TODO: TextField: make text truncation work, including arrow keys to change left position//
//
//    #warning TODO: TextField: make enter key work
//
//    #warning TODO: TextField: make tabbing between fields work
//
//    #warning TODO: add help flag
//
//    #warning TODO: fix: if the mouse is moving upon exit, we get mouse characters printed to the terminal
//
//    #warning TODO: handle not being able to materialize commits via Convert()
//
//    #warning TODO: make dark-mode error color more reddish
//
//    #warning TODO: support light mode
//
//    #warning TODO: improve error when working directory isn't a git repo
//
//    #warning TODO: when switching snapshots, clear the selection
//
//    #warning TODO: move branch name to top
//
//    #warning TODO: fix: snapshots line wrap is 1-character off
//
//    #warning TODO: nevermind: show warning on startup: Take care when rewriting history. As with any software, debase may be bugs. As a safety precaution, debase will automatically backup all branches before modifying them, as <BranchName>-DebaseBackup
//
//    #warning TODO: rename all hittest functions -> updateMouse
//    
//    #warning TODO: fix: handle case when snapshot menu won't fit entirely on screen
//
//    #warning TODO: nevermind: don't show the first snapshot if it's the same as the "Session Start" snapshot
//
//    #warning TODO: make sure works with submodules
//
//    #warning TODO: keep the oldest snapshots, instead of the newest
//
//    #warning TODO: nevermind: figure out how to hide a snapshot if it's the same as the "Session Start" snapshot
//
//    #warning TODO: improve appearance of active snapshot marker in snapshot menu
//
//    #warning TODO: nevermind: only show the current-session marker on the "Session Start" element if there's no other snapshot that has it
//
//    #warning TODO: fully flesh out when to create new snapshots
//
//    #warning TODO: show "Session Start" snapshot
//
//    #warning TODO: fix: crash when deleting prefs dir while debase is running
//
//    #warning TODO: get command-. working while context menu/snapshot menu is open
//
//    #warning TODO: when creating a new snapshot, if an equivalent one already exists, remove the old one.
//
//    #warning TODO: further, snapshots should be created only at startup ("Session Start"), and on exit, the snapshot to only be enqueued if it differs from the current HEAD of the repo
//    #warning TODO: when restoring a snapshot, it should just restore the HEAD of the snapshot, and push an undo operation (as any other operation) that the user can undo/redo as normal
//    #warning TODO: turn snapshots into merely keeping track of what commit was the HEAD, not the full undo history.
//
//    #warning TODO: backup all supplied revs before doing anything
//    
//    #warning TODO: nevermind: implement log of events, so that if something goes wrong, we can manually get back
//
//    #warning TODO: test a commit stored in the undo history not existing
//
//    #warning TODO: use advisory locking in State class to prevent multiple entities from modifying state on disk simultaneously. RepoState class will need to use this facility when writing its state. Whenever we acquire the lock, we should read the version, because the version may have changed since the last read! if so -> bail!
//
//    #warning TODO: write a version number at the root of the StateDir, and check it somewhere
//    
//    #warning TODO: don't lose the UndoHistory when: a ref no longer exists that we previously observed
//    #warning TODO: don't lose the UndoHistory when: debase is run and a ref points to a different commit than we last observed
//
//    #warning TODO: show detailed error message if we fail to restore head due to conflicts
//
//    #warning TODO: refuse to run if there are uncomitted changes and we're detaching head
//
//    #warning TODO: perf: if we aren't modifying the current checked-out branch, don't detach head
//
//    #warning TODO: implement _ConfigDir() for real
//
//    #warning TODO: undo: remember selection as a part of the undo state
//    
//    #warning TODO: fix: (1) copy a commit to col A, (2) swap elements 1 & 2 of col A. note how the copied commit doesn't get selected when performing undo/redo
//    
//    #warning TODO: fix: copying a commit from a column shouldn't affect column's undo state (but it does)
//
//    #warning TODO: support undo/redo
//
//    #warning TODO: when deserializing, if a ref doesn't exist, don't throw an exception, just prune the entry
//    
//    #warning TODO: undo: fix UndoHistory deserialization
//
//    #warning TODO: make "(HEAD)" suffix persist across branch modifications
//    
//    #warning TODO: make sure debase still works when running on a detached HEAD
//
//    #warning TODO: fix wrong column being selected after moving commit in master^
//
//    #warning TODO: improve error messages when we can't lookup supplied refs
//
//    #warning TODO: when supplying refs on the command line in the form ref^ or ref~N, can we use a ref-backed rev (instead of using a commit-backed rev), and just offset the RevColumn, so that the rev is mutable?
//
//    #warning TODO: improve error messages: merge conflicts, deleting last branch commit
//
//    #warning TODO:   dragging commits to their current location -> no repo changes
//    #warning TODO:   same commit message -> no repo changes
//    #warning TODO: no-op guarantee:
//
//    #warning TODO: make sure we can round-trip with the same date/time. especially test commits with negative UTC offsets!
//    
//    #warning TODO: bring back _CommitTime so we don't need to worry about the 'sign' field of git_time
//
//    #warning TODO: handle merge conflicts
//
//    #warning TODO: set_escdelay: not sure if we're going to encounter issues?
//
//    #warning TODO: don't allow combine when a merge commit is selected
//
//    #warning TODO: always show the same contextual menu, but gray-out the disabled options
//    
//    #warning TODO: make sure moving/deleting commits near the root commit still works
//
//    #warning TODO: properly handle moving/copying merge commits
//
//    #warning TODO: show some kind of indication that a commit is a merge commit
//
//    #warning TODO: handle window resizing
//
//    #warning TODO: re-evaluate size of drag-cancel affordance since it's not as reliable in iTerm
//
//    #warning TODO: don't allow double-click/return key on commits in read-only columns
//
//    #warning TODO: handle rewriting tags
//    
//    #warning TODO: create special color palette for apple terminal
//
//    #warning TODO: if can_change_color() returns false, use default color palette (COLOR_RED, etc)
//
//    #warning TODO: fix: colors aren't restored when exiting
//
//    #warning TODO: figure out whether/where/when to call git_libgit2_shutdown()
//
//    #warning TODO: implement error messages
//
//    #warning TODO: double-click broken: click commit, wait, then double-click
//
//    #warning TODO: implement double-click to edit
//
//    #warning TODO: implement key combos for combine/edit
//
//    #warning TODO: show indication in the UI that a column is immutable
//
//    #warning TODO: special-case opening `mate` when editing commit, to not call CursesDeinit/CursesInit
//
//    #warning TODO: implement EditCommit
//
//    #warning TODO: allow editing author/date of commit
//    
//    #warning TODO: configure tty before/after calling out to a new editor
//    
//    #warning TODO: fix commit message rendering when there are newlines
//
//    #warning TODO: improve panel dragging along edges. currently the shadow panels get a little messed up
//
//    #warning TODO: add affordance to cancel current drag by moving mouse to edge of terminal
//
//    #warning TODO: implement CombineCommits
//
//    #warning TODO: draw "Move/Copy" text immediately above the dragged commits, instead of at the insertion point
//    
//    #warning TODO: create concept of revs being mutable. if they're not mutable, don't allow moves from them (only copies), moves to them, deletes, etc
//    
//    #warning TODO: don't allow remote branches to be modified (eg 'origin/master')
//    
//    #warning TODO: show similar commits to the selected commit using a lighter color
//    
//    #warning       to do so, we'll need to implement operator< on Rev so we can put them in a set
//    #warning TODO: we need to unique-ify the supplied revs, since we assume that each column has a unique rev
//    
//    #warning TODO: allow escape key to abort a drag
//    
//    #warning TODO: when copying commmits, don't hide the source commits
    
//    {
//        volatile bool a = false;
//        while (!a);
//    }
    
    try {
        _Args args = _ParseArgs(argc-1, argv+1);
        
        if (args.help.en) {
            _PrintUsage();
            return 0;
        
        } else if (args.setTheme.en) {
            State::Theme theme = State::Theme::None;
            if (args.setTheme.theme == "auto") {
                theme = State::Theme::None;
            } else if (args.setTheme.theme == "dark") {
                theme = State::Theme::Dark;
            } else if (args.setTheme.theme == "light") {
                theme = State::Theme::Light;
            } else {
                throw Toastbox::RuntimeError("invalid theme: %s", args.setTheme.theme.c_str());
            }
            _ThemeWrite(theme);
            return 0;
        
        } else if (!args.normal.en) {
            throw Toastbox::RuntimeError("invalid arguments");
        }
        
        // Disable echo before activating ncurses
        // This is necessary to prevent an edge case where the mouse-moved escape
        // sequences can get printed to the console when debase exits.
        // So far we haven't been able to reproduce the issue after adding this
        // code. But if we do see it again in the future, try giving tcsetattr()
        // the TCSAFLUSH or TCSADRAIN flag in Terminal::Settings (instead of
        // TCSANOW) to see if that solves it.
        Terminal::Settings term(STDIN_FILENO);
        term.c_lflag &= ~(ICANON|ECHO);
        term.set();
        
        setlocale(LC_ALL, "");
        
        _Theme = _ThemeRead();
        
        try {
            _Repo = Git::Repo::Open(".");
        } catch (...) {
            throw Toastbox::RuntimeError("current directory isn't a git repository");
        }
        
        _Head = _Repo.head();
        
        if (args.normal.revs.empty()) {
            _Revs.emplace_back(_Head);
        
        } else {
            // Unique the supplied revs, because our code assumes a 1:1 mapping between Revs and RevColumns
            std::set<Git::Rev> unique;
            for (const std::string& revName : args.normal.revs) {
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
        } catch (const UI::ExitRequest&) {
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
