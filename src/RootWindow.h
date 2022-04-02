#pragma once
#include "Window.h"
#include "Color.h"
#include "Git.h"
#include "State.h"
#include "RevColumn.h"
#include "SnapshotButton.h"
#include "BorderedPanel.h"
#include "Menu.h"
#include "SnapshotMenu.h"
#include "ModalPanel.h"
#include "RegisterPanel.h"

extern "C" {
    extern char** environ;
};

namespace UI {

template <auto T_SpawnFn>
class RootWindow : public UI::Window {
public:
    RootWindow(WINDOW* win, const ColorPalette& colors, State::RepoState& repoState, Git::Repo repo, Git::Rev head, const std::vector<Git::Rev>& revs) :
    Window(win), _Colors(colors), _RepoState(repoState), _Repo(repo), _Head(head), _Revs(revs) {
        
    }
    
    void layout() override {
        layoutNeeded = true;
        if (!layoutNeeded) return;
        Window::layout();
        
        const UI::Color selectionColor = (_Drag.copy ? _Colors.selectionCopy : _Colors.selection);
        
        if (_Drag.titlePanel) {
            _Drag.titlePanel->borderColor(selectionColor);
            
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
                
                panel->visible(visible);
                panel->borderColor(borderColor);
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
        
        if (_MessagePanel) {
            constexpr int MessagePanelWidth = 40;
            _MessagePanel->width = std::min(MessagePanelWidth, bounds().size.x);
            _MessagePanel->size(_MessagePanel->sizeIntrinsic());
            
            UI::Size ps = _MessagePanel->frame().size;
            UI::Size rs = frame().size;
            UI::Point p = {
                (rs.x-ps.x)/2,
                (rs.y-ps.y)/3,
            };
            _MessagePanel->position(p);
            _MessagePanel->orderFront();
            _MessagePanel->layout();
        }
        
        if (_RegisterPanel) {
            constexpr int RegisterPanelWidth = 50;
            _RegisterPanel->width = std::min(RegisterPanelWidth, bounds().size.x);
            _RegisterPanel->size(_RegisterPanel->sizeIntrinsic());
            
            UI::Size ps = _RegisterPanel->frame().size;
            UI::Size rs = frame().size;
            UI::Point p = {
                (rs.x-ps.x)/2,
                (rs.y-ps.y)/3,
            };
            _RegisterPanel->position(p);
            _RegisterPanel->orderFront();
            _RegisterPanel->layout();
        }
    }
    
    void draw() override {
        drawNeeded = true;
        if (!drawNeeded) return;
        Window::draw();
        
        const UI::Color selectionColor = (_Drag.copy ? _Colors.selectionCopy : _Colors.selection);
        
        erase();
        
        if (_Drag.titlePanel) {
            _Drag.titlePanel->draw();
            
            // Draw insertion marker
            if (_Drag.insertionMarker) {
                UI::Window::Attr color = attr(selectionColor);
                drawLineHoriz(_Drag.insertionMarker->point, _Drag.insertionMarker->size.x);
            }
        }
        
        for (UI::BorderedPanelPtr panel : _Drag.shadowPanels) {
            panel->draw();
        }
        
        if (_SelectionRect) {
            UI::Window::Attr color = attr(_Colors.selection);
            drawRect(*_SelectionRect);
        }
        
        for (UI::RevColumnPtr col : _Columns) {
            col->draw(*this);
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
    }
    
    bool handleEvent(const UI::Event& ev) override {
        std::string errorMsg;
        try {
            if (_MessagePanel) {
                bool handled = _MessagePanel->handleEvent(ev);
                if (!handled) {
                    _MessagePanel = nullptr;
                    return false;
                }
                return true;
            }
            
            if (_RegisterPanel) {
                bool handled = _RegisterPanel->handleEvent(ev);
                if (!handled) {
                    _RegisterPanel = nullptr;
                    return false;
                }
                return true;
            }
            
            // Let every column handle the event
            for (UI::RevColumnPtr col : _Columns) {
                bool handled = col->handleEvent(*this, ev);
                if (handled) return true;
            }
            
            switch (ev.type) {
            case UI::Event::Type::Mouse: {
                const _HitTestResult hitTest = _HitTest(ev.mouse.point);
                if (ev.mouseDown(UI::Event::MouseButtons::Left)) {
                    const bool shift = (ev.mouse.bstate & _SelectionShiftKeys);
                    if (hitTest && !shift) {
                        if (hitTest.panel) {
                            // Mouse down inside of a CommitPanel, without shift key
                            std::optional<Git::Op> gitOp = _TrackMouseInsideCommitPanel(ev, hitTest.column, hitTest.panel);
                            if (gitOp) _ExecGitOp(*gitOp);
                        }
                    
                    } else {
                        // Mouse down outside of a CommitPanel, or mouse down anywhere with shift key
                        _TrackMouseOutsideCommitPanel(ev);
                    }
                
                } else if (ev.mouseDown(UI::Event::MouseButtons::Right)) {
                    if (hitTest) {
                        if (hitTest.panel) {
                            std::optional<Git::Op> gitOp = _TrackRightMouse(ev, hitTest.column, hitTest.panel);
                            if (gitOp) _ExecGitOp(*gitOp);
                        }
                    }
                }
                break;
            }
            
            case UI::Event::Type::KeyDelete:
            case UI::Event::Type::KeyFnDelete: {
                if (!_SelectionCanDelete()) {
                    beep();
                    break;
                }
                
                Git::Op gitOp = {
                    .type = Git::Op::Type::Delete,
                    .src = {
                        .rev = _Selection.rev,
                        .commits = _Selection.commits,
                    },
                };
                _ExecGitOp(gitOp);
                break;
            }
            
            case UI::Event::Type::KeyC: {
                if (!_SelectionCanCombine()) {
                    beep();
                    break;
                }
                
                Git::Op gitOp = {
                    .type = Git::Op::Type::Combine,
                    .src = {
                        .rev = _Selection.rev,
                        .commits = _Selection.commits,
                    },
                };
                _ExecGitOp(gitOp);
                break;
            }
            
            case UI::Event::Type::KeyReturn: {
                if (!_SelectionCanEdit()) {
                    beep();
                    break;
                }
                
                Git::Op gitOp = {
                    .type = Git::Op::Type::Edit,
                    .src = {
                        .rev = _Selection.rev,
                        .commits = _Selection.commits,
                    },
                };
                _ExecGitOp(gitOp);
                break;
            }
            
            default: {
                break;
            }}
        
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
        
        } catch (const UI::WindowResize&) {
            throw; // Bubble up
        
        } catch (const UI::ExitRequest&) {
            throw; // Bubble up
        
        } catch (const std::exception& e) {
            errorMsg = e.what();
        }
        
        if (!errorMsg.empty()) {
            errorMsg[0] = toupper(errorMsg[0]);
            
            _MessagePanel = std::make_shared<UI::ModalPanel>(_Colors);
            _MessagePanel->color    = _Colors.error;
            _MessagePanel->align    = UI::ModalPanel::TextAlign::CenterSingleLine;
            _MessagePanel->title    = "Error";
            _MessagePanel->message  = errorMsg;
        }
        
        return true;
    }
    
    void run() {
        for (;;) {
            _Reload();
            try {
                track();
            } catch (const UI::WindowResize&) {
                // Continue the loop, which calls _Reload()
            }
        }
    }
    
    void _Reload() {
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
            
            std::function<void(UI::RevColumn&)> undoAction;
            std::function<void(UI::RevColumn&)> redoAction;
            std::function<void(UI::RevColumn&)> snapshotsAction;
            
            if (h && !h->begin()) {
                undoAction = [&] (UI::RevColumn& col) { _UndoRedo(col, true); };
            }
            
            if (h && !h->end()) {
                redoAction = [&] (UI::RevColumn& col) { _UndoRedo(col, false); };
            }
            
            snapshotsAction = [&] (UI::RevColumn& col) { _TrackSnapshotsMenu(col); };
            
            UI::RevColumnPtr col = std::make_shared<UI::RevColumn>(_Colors);
            col->containerBounds    = bounds();
            col->repo               = _Repo;
            col->rev                = rev;
            col->head               = (rev.displayHead() == _Head.commit);
            col->offset             = UI::Size{OffsetX, 0};
            col->width              = ColumnWidth;
            col->undoAction         = undoAction;
            col->redoAction         = redoAction;
            col->snapshotsAction    = snapshotsAction;
            _Columns.push_back(col);
            
            OffsetX += ColumnWidth+ColumnSpacing;
        }
    }
    
    std::optional<Git::Op> _TrackRightMouse(const UI::Event& mouseDownEvent, UI::RevColumnPtr mouseDownColumn, UI::CommitPanelPtr mouseDownPanel) {
        // If the commit that was clicked isn't selected, set the selection to only that commit
        if (!_Selected(mouseDownColumn, mouseDownPanel)) {
            _Selection = {
                .rev = mouseDownColumn->rev,
                .commits = {mouseDownPanel->commit()},
            };
        }
        
        assert(!_Selection.commits.empty());
        
        // Draw once before we open the context menu, so that the selection is updated
        refresh();
        
        UI::Button* menuButton = nullptr;
        UI::ButtonPtr combineButton = _MakeContextMenuButton("Combine", "c", _SelectionCanCombine(), menuButton);
        UI::ButtonPtr editButton    = _MakeContextMenuButton("Edit", "ret", _SelectionCanEdit(), menuButton);
        UI::ButtonPtr deleteButton  = _MakeContextMenuButton("Delete", "del", _SelectionCanDelete(), menuButton);
        
        std::vector<UI::ButtonPtr> buttons = { combineButton, editButton, deleteButton };
        _ContextMenu = std::make_shared<UI::Menu>(_Colors);
        Defer(_ContextMenu = nullptr);
        
        _ContextMenu->buttons = buttons;
        _ContextMenu->size(_ContextMenu->sizeIntrinsic());
        _ContextMenu->position(mouseDownEvent.mouse.point);
        _ContextMenu->layout();
        _ContextMenu->track(_ContextMenu->convert(mouseDownEvent));
        
        // Handle the clicked button
        std::optional<Git::Op> gitOp;
        if (menuButton == combineButton.get()) {
            gitOp = Git::Op{
                .type = Git::Op::Type::Combine,
                .src = {
                    .rev = _Selection.rev,
                    .commits = _Selection.commits,
                },
            };
        
        } else if (menuButton == editButton.get()) {
            gitOp = Git::Op{
                .type = Git::Op::Type::Edit,
                .src = {
                    .rev = _Selection.rev,
                    .commits = _Selection.commits,
                },
            };
        
        } else if (menuButton == deleteButton.get()) {
            gitOp = Git::Op{
                .type = Git::Op::Type::Delete,
                .src = {
                    .rev = _Selection.rev,
                    .commits = _Selection.commits,
                },
            };
        }
        
        return gitOp;
    }
    
    // _TrackMouseInsideCommitPanel
    // Handles clicking/dragging a set of CommitPanels
    std::optional<Git::Op> _TrackMouseInsideCommitPanel(const UI::Event& mouseDownEvent, UI::RevColumnPtr mouseDownColumn, UI::CommitPanelPtr mouseDownPanel) {
        const UI::Rect mouseDownPanelFrame = mouseDownPanel->frame();
        const UI::Size mouseDownOffset = mouseDownPanelFrame.point - mouseDownEvent.mouse.point;
        const bool wasSelected = _Selected(mouseDownColumn, mouseDownPanel);
        const UI::Rect rootWinBounds = bounds();
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
                _Drag.titlePanel = std::make_shared<UI::CommitPanel>(_Colors, true, titlePanel->frame().size.x, titleCommit);
                
                // Create shadow panels
                UI::Size shadowSize = _Drag.titlePanel->frame().size;
                for (size_t i=0; i<_Selection.commits.size()-1; i++) {
                    _Drag.shadowPanels.push_back(std::make_shared<UI::BorderedPanel>(shadowSize));
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
                    _Drag.titlePanel->headerLabel(_Drag.copy ? "Copy" : "Move");
                }
                
                // Position title panel / shadow panels
                {
                    const UI::Point pos0 = p + mouseDownOffset;
                    
                    _Drag.titlePanel->position(pos0);
                    
                    // Position shadowPanels
                    int off = 1;
                    for (UI::PanelPtr panel : _Drag.shadowPanels) {
                        const UI::Point pos = pos0+off;
                        panel->position(pos);
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
            
            refresh();
            ev = nextEvent();
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
    void _TrackMouseOutsideCommitPanel(const UI::Event& mouseDownEvent) {
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
            
            refresh();
            ev = nextEvent();
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
    
    void _TrackSnapshotsMenu(UI::RevColumn& col) {
        UI::SnapshotButton* menuButton = nullptr;
        Git::Ref ref = col.rev.ref;
        std::vector<UI::ButtonPtr> buttons = {
            _MakeSnapshotMenuButton(_Repo, ref, _RepoState.initialSnapshot(ref), true, menuButton),
        };
        
        const std::vector<State::Snapshot>& snapshots = _RepoState.snapshots(ref);
        for (auto it=snapshots.rbegin(); it!=snapshots.rend(); it++) {
            // Creating the button will throw if we can't get the commit for the snapshot
            // If that happens, just don't shown the button representing the snapshot
            try {
                buttons.push_back(_MakeSnapshotMenuButton(_Repo, ref, *it, false, menuButton));
            } catch (...) {}
        }
        
        const int width = _SnapshotMenuWidth+UI::SnapshotMenu::Padding().x;
        const int px = col.offset.x + (col.width-width)/2;
        
        _SnapshotsMenu = std::make_shared<UI::SnapshotMenu>(_Colors);
        Defer(_SnapshotsMenu = nullptr);
        
        _SnapshotsMenu->title = "Session Start";
        _SnapshotsMenu->buttons = buttons;
        
        const UI::Point p = {px, 2};
        const int heightMax = size().y-p.y;
        _SnapshotsMenu->buttons = buttons;
        _SnapshotsMenu->size(_SnapshotsMenu->sizeIntrinsic(heightMax));
        _SnapshotsMenu->position(p);
        _SnapshotsMenu->layout();
        _SnapshotsMenu->track(_SnapshotsMenu->convert(eventCurrent));
        
        if (menuButton) {
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
    }
    
    void _UndoRedo(UI::RevColumn& col, bool undo) {
        Git::Rev rev = col.rev;
        State::History& h = _RepoState.history(col.rev.ref);
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
    
    void _ExecGitOp(const Git::Op& gitOp) {
        std::optional<Git::OpResult> opResult = Git::Exec<T_SpawnFn>(_Repo, gitOp);
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
    
private:
    struct _Selection {
        Git::Rev rev;
        std::set<Git::Commit> commits;
    };
    
    enum class _SelectState {
        False,
        True,
        Similar,
    };
    
    struct _InsertionPosition {
        UI::RevColumnPtr col;
        UI::CommitPanelVecIter iter;
    };
    
    static constexpr int _SnapshotMenuWidth = 26;
    static constexpr auto _DoubleClickThresh = std::chrono::milliseconds(300);
    static constexpr mmask_t _SelectionShiftKeys = BUTTON_CTRL | BUTTON_SHIFT;
    
    static Git::Commit _FindLatestCommit(Git::Commit head, const std::set<Git::Commit>& commits) {
        while (head) {
            if (commits.find(head) != commits.end()) return head;
            head = head.parent();
        }
        // Programmer error if it doesn't exist
        abort();
    }
    
    _SelectState _SelectStateGet(UI::RevColumnPtr col, UI::CommitPanelPtr panel) {
        bool similar = _Selection.commits.find(panel->commit()) != _Selection.commits.end();
        if (!similar) return _SelectState::False;
        return (col->rev==_Selection.rev ? _SelectState::True : _SelectState::Similar);
    }
    
    bool _Selected(UI::RevColumnPtr col, UI::CommitPanelPtr panel) {
        return _SelectStateGet(col, panel) == _SelectState::True;
    }
    
    struct _HitTestResult {
        UI::RevColumnPtr column;
        UI::CommitPanelPtr panel;
        operator bool() const {
            return (bool)column;
        }
    };
    
    _HitTestResult _HitTest(const UI::Point& p) {
        for (UI::RevColumnPtr col : _Columns) {
            UI::CommitPanelPtr panel = col->hitTest(p);
            if (panel) {
                return {
                    .column = col,
                    .panel = panel,
                };
            }
        }
        return {};
    }
    
    std::optional<_InsertionPosition> _FindInsertionPosition(const UI::Point& p) {
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
    
    UI::RevColumnPtr _ColumnForRev(Git::Rev rev) {
        for (UI::RevColumnPtr col : _Columns) {
            if (col->rev == rev) return col;
        }
        // Programmer error if it doesn't exist
        abort();
    }
    
    UI::CommitPanelPtr _PanelForCommit(UI::RevColumnPtr col, Git::Commit commit) {
        for (UI::CommitPanelPtr panel : col->panels) {
            if (panel->commit() == commit) return panel;
        }
        // Programmer error if it doesn't exist
        abort();
    }
    
    UI::ButtonPtr _MakeSnapshotMenuButton(Git::Repo repo, Git::Ref ref,
        const State::Snapshot& snap, bool sessionStart, UI::SnapshotButton*& chosen) {
        
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
        b->enabled        = true;
        b->activeSnapshot = activeSnapshot;
        b->action         = [&] (UI::Button& button) { chosen = (UI::SnapshotButton*)&button; };
        return b;
    }

    UI::ButtonPtr _MakeContextMenuButton(std::string_view label, std::string_view key, bool enabled, UI::Button*& chosen) {
        constexpr int ContextMenuWidth = 12;
        UI::ButtonPtr b = std::make_shared<UI::Button>(_Colors);
        b->label        = std::string(label);
        b->key          = std::string(key);
        b->enabled      = enabled;
        b->insetX       = 0;
        b->action       = [&] (UI::Button& button) { chosen = &button; };
        b->size({ContextMenuWidth, 1});
        return b;
    }

    bool _SelectionCanCombine() {
        bool selectionContainsMerge = false;
        for (Git::Commit commit : _Selection.commits) {
            if (commit.isMerge()) {
                selectionContainsMerge = true;
                break;
            }
        }
        return _Selection.rev.isMutable() && _Selection.commits.size()>1 && !selectionContainsMerge;
    }

    bool _SelectionCanEdit() {
        return _Selection.rev.isMutable() && _Selection.commits.size() == 1;
    }

    bool _SelectionCanDelete() {
        return _Selection.rev.isMutable();
    }
    
    const UI::ColorPalette& _Colors;
    State::RepoState& _RepoState;
    Git::Repo _Repo;
    Git::Rev _Head;
    std::vector<Git::Rev> _Revs;
    std::vector<UI::RevColumnPtr> _Columns;

    struct {
        UI::CommitPanelPtr titlePanel;
        std::vector<UI::BorderedPanelPtr> shadowPanels;
        std::optional<UI::Rect> insertionMarker;
        bool copy = false;
    } _Drag;

    struct {
        Git::Commit commit;
        std::chrono::steady_clock::time_point mouseUpTime;
    } _DoubleClickState;

    _Selection _Selection;
    std::optional<UI::Rect> _SelectionRect;

    UI::MenuPtr _ContextMenu;
    UI::SnapshotMenuPtr _SnapshotsMenu;

    UI::ModalPanelPtr _MessagePanel;
    UI::RegisterPanelPtr _RegisterPanel;

};

} // namespace UI
