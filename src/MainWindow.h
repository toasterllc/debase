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
class MainWindow : public Window {
public:
    MainWindow(const ColorPalette& colors, State::RepoState& repoState, Git::Repo repo, Git::Rev head, const std::vector<Git::Rev>& revs) :
    Window(::stdscr), _colors(colors), _repoState(repoState), _repo(repo), _head(head), _revs(revs) {
        
    }
    
    void layout() override {
        layoutNeeded = true;
        if (!layoutNeeded) return;
        Window::layout();
        
        const Color selectionColor = (_drag.copy ? _colors.selectionCopy : _colors.selection);
        
        if (_drag.titlePanel) {
            _drag.titlePanel->borderColor(selectionColor);
            
            for (BorderedPanelPtr panel : _drag.shadowPanels) {
                panel->setBorderColor(selectionColor);
            }
        }
        
        for (RevColumnPtr col : _columns) {
            col->layout();
        }
        
        bool dragging = (bool)_drag.titlePanel;
        bool copying = _drag.copy;
        for (RevColumnPtr col : _columns) {
            for (CommitPanelPtr panel : col->panels) {
                bool visible = false;
                std::optional<Color> borderColor;
                _SelectState selectState = _SelectStateGet(col, panel);
                if (selectState == _SelectState::True) {
                    visible = !dragging || copying;
                    if (dragging) borderColor = _colors.selectionSimilar;
                    else          borderColor = _colors.selection;
                
                } else {
                    visible = true;
                    if (!dragging && selectState==_SelectState::Similar) borderColor = _colors.selectionSimilar;
                }
                
                panel->visible(visible);
                panel->borderColor(borderColor);
            }
        }
        
        // Order all the title panel and shadow panels
        if (dragging) {
            for (auto it=_drag.shadowPanels.rbegin(); it!=_drag.shadowPanels.rend(); it++) {
                (*it)->orderFront();
            }
            _drag.titlePanel->orderFront();
        }
        
        if (_contextMenu) {
            _contextMenu->orderFront();
        }
        
        if (_snapshotsMenu) {
            _snapshotsMenu->orderFront();
        }
        
        if (_messagePanel) {
            constexpr int MessagePanelWidth = 40;
            _messagePanel->width = std::min(MessagePanelWidth, bounds().size.x);
            _messagePanel->size(_messagePanel->sizeIntrinsic());
            
            Size ps = _messagePanel->frame().size;
            Size rs = frame().size;
            Point p = {
                (rs.x-ps.x)/2,
                (rs.y-ps.y)/3,
            };
            _messagePanel->position(p);
            _messagePanel->orderFront();
            _messagePanel->layout();
        }
        
        if (_registerPanel) {
            constexpr int RegisterPanelWidth = 50;
            _registerPanel->width = std::min(RegisterPanelWidth, bounds().size.x);
            _registerPanel->size(_registerPanel->sizeIntrinsic());
            
            Size ps = _registerPanel->frame().size;
            Size rs = frame().size;
            Point p = {
                (rs.x-ps.x)/2,
                (rs.y-ps.y)/3,
            };
            _registerPanel->position(p);
            _registerPanel->orderFront();
            _registerPanel->layout();
        }
    }
    
    void draw() override {
        drawNeeded = true;
        if (!drawNeeded) return;
        Window::draw();
        
        const Color selectionColor = (_drag.copy ? _colors.selectionCopy : _colors.selection);
        
        erase();
        
        if (_drag.titlePanel) {
            _drag.titlePanel->draw();
            
            // Draw insertion marker
            if (_drag.insertionMarker) {
                Window::Attr color = attr(selectionColor);
                drawLineHoriz(_drag.insertionMarker->point, _drag.insertionMarker->size.x);
            }
        }
        
        for (BorderedPanelPtr panel : _drag.shadowPanels) {
            panel->draw();
        }
        
        if (_selectionRect) {
            Window::Attr color = attr(_colors.selection);
            drawRect(*_selectionRect);
        }
        
        for (RevColumnPtr col : _columns) {
            col->draw(*this);
        }
        
        if (_contextMenu) {
            _contextMenu->draw();
        }
        
        if (_snapshotsMenu) {
            _snapshotsMenu->draw();
        }
        
        if (_messagePanel) {
            _messagePanel->draw();
        }
        
        if (_registerPanel) {
            _registerPanel->draw();
        }
    }
    
    bool handleEvent(const Event& ev) override {
        std::string errorMsg;
        try {
            if (_messagePanel) {
                bool handled = _messagePanel->handleEvent(ev);
                if (!handled) {
                    _messagePanel = nullptr;
                    return false;
                }
                return true;
            }
            
            if (_registerPanel) {
                bool handled = _registerPanel->handleEvent(ev);
                if (!handled) {
                    _registerPanel = nullptr;
                    return false;
                }
                return true;
            }
            
            // Let every column handle the event
            for (RevColumnPtr col : _columns) {
                bool handled = col->handleEvent(*this, ev);
                if (handled) return true;
            }
            
            switch (ev.type) {
            case Event::Type::Mouse: {
                const _HitTestResult hitTest = _HitTest(ev.mouse.point);
                if (ev.mouseDown(Event::MouseButtons::Left)) {
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
                
                } else if (ev.mouseDown(Event::MouseButtons::Right)) {
                    if (hitTest) {
                        if (hitTest.panel) {
                            std::optional<Git::Op> gitOp = _TrackRightMouse(ev, hitTest.column, hitTest.panel);
                            if (gitOp) _ExecGitOp(*gitOp);
                        }
                    }
                }
                break;
            }
            
            case Event::Type::KeyDelete:
            case Event::Type::KeyFnDelete: {
                if (!_SelectionCanDelete()) {
                    beep();
                    break;
                }
                
                Git::Op gitOp = {
                    .type = Git::Op::Type::Delete,
                    .src = {
                        .rev = _selection.rev,
                        .commits = _selection.commits,
                    },
                };
                _ExecGitOp(gitOp);
                break;
            }
            
            case Event::Type::KeyC: {
                if (!_SelectionCanCombine()) {
                    beep();
                    break;
                }
                
                Git::Op gitOp = {
                    .type = Git::Op::Type::Combine,
                    .src = {
                        .rev = _selection.rev,
                        .commits = _selection.commits,
                    },
                };
                _ExecGitOp(gitOp);
                break;
            }
            
            case Event::Type::KeyReturn: {
                if (!_SelectionCanEdit()) {
                    beep();
                    break;
                }
                
                Git::Op gitOp = {
                    .type = Git::Op::Type::Edit,
                    .src = {
                        .rev = _selection.rev,
                        .commits = _selection.commits,
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
        
        } catch (const WindowResize&) {
            throw; // Bubble up
        
        } catch (const ExitRequest&) {
            throw; // Bubble up
        
        } catch (const std::exception& e) {
            errorMsg = e.what();
        }
        
        if (!errorMsg.empty()) {
            errorMsg[0] = toupper(errorMsg[0]);
            
            _messagePanel = std::make_shared<ModalPanel>(_colors);
            _messagePanel->color    = _colors.error;
            _messagePanel->align    = ModalPanel::TextAlign::CenterSingleLine;
            _messagePanel->title    = "Error";
            _messagePanel->message  = errorMsg;
        }
        
        return true;
    }
    
    void run() {
        for (;;) {
            _Reload();
            try {
                track();
            } catch (const WindowResize&) {
                // Continue the loop, which calls _Reload()
            }
        }
    }
    
    void _Reload() {
        // Create a RevColumn for each specified branch
        constexpr int InsetX = 3;
        constexpr int ColumnWidth = 32;
        constexpr int ColumnSpacing = 6;
        
        if (_head.ref) {
            _head = _repo.revReload(_head);
        }
        
        for (Git::Rev& rev : _revs) {
            if (rev.ref) {
                rev = _repo.revReload(rev);
            }
        }
        
        _columns.clear();
        int OffsetX = InsetX;
        for (const Git::Rev& rev : _revs) {
            State::History* h = (rev.ref ? &_repoState.history(rev.ref) : nullptr);
            
            std::function<void(RevColumn&)> undoAction;
            std::function<void(RevColumn&)> redoAction;
            std::function<void(RevColumn&)> snapshotsAction;
            
            if (h && !h->begin()) {
                undoAction = [&] (RevColumn& col) { _UndoRedo(col, true); };
            }
            
            if (h && !h->end()) {
                redoAction = [&] (RevColumn& col) { _UndoRedo(col, false); };
            }
            
            snapshotsAction = [&] (RevColumn& col) { _TrackSnapshotsMenu(col); };
            
            RevColumnPtr col = std::make_shared<RevColumn>(_colors);
            col->containerBounds    = bounds();
            col->repo               = _repo;
            col->rev                = rev;
            col->head               = (rev.displayHead() == _head.commit);
            col->offset             = Size{OffsetX, 0};
            col->width              = ColumnWidth;
            col->undoAction         = undoAction;
            col->redoAction         = redoAction;
            col->snapshotsAction    = snapshotsAction;
            _columns.push_back(col);
            
            OffsetX += ColumnWidth+ColumnSpacing;
        }
    }
    
    std::optional<Git::Op> _TrackRightMouse(const Event& mouseDownEvent, RevColumnPtr mouseDownColumn, CommitPanelPtr mouseDownPanel) {
        // If the commit that was clicked isn't selected, set the selection to only that commit
        if (!_Selected(mouseDownColumn, mouseDownPanel)) {
            _selection = {
                .rev = mouseDownColumn->rev,
                .commits = {mouseDownPanel->commit()},
            };
        }
        
        assert(!_selection.commits.empty());
        
        // Draw once before we open the context menu, so that the selection is updated
        refresh();
        
        Button* menuButton = nullptr;
        ButtonPtr combineButton = _MakeContextMenuButton("Combine", "c", _SelectionCanCombine(), menuButton);
        ButtonPtr editButton    = _MakeContextMenuButton("Edit", "ret", _SelectionCanEdit(), menuButton);
        ButtonPtr deleteButton  = _MakeContextMenuButton("Delete", "del", _SelectionCanDelete(), menuButton);
        
        std::vector<ButtonPtr> buttons = { combineButton, editButton, deleteButton };
        _contextMenu = std::make_shared<Menu>(_colors);
        Defer(_contextMenu = nullptr);
        
        _contextMenu->buttons = buttons;
        _contextMenu->size(_contextMenu->sizeIntrinsic());
        _contextMenu->position(mouseDownEvent.mouse.point);
        _contextMenu->layout();
        _contextMenu->track(_contextMenu->convert(mouseDownEvent));
        
        // Handle the clicked button
        std::optional<Git::Op> gitOp;
        if (menuButton == combineButton.get()) {
            gitOp = Git::Op{
                .type = Git::Op::Type::Combine,
                .src = {
                    .rev = _selection.rev,
                    .commits = _selection.commits,
                },
            };
        
        } else if (menuButton == editButton.get()) {
            gitOp = Git::Op{
                .type = Git::Op::Type::Edit,
                .src = {
                    .rev = _selection.rev,
                    .commits = _selection.commits,
                },
            };
        
        } else if (menuButton == deleteButton.get()) {
            gitOp = Git::Op{
                .type = Git::Op::Type::Delete,
                .src = {
                    .rev = _selection.rev,
                    .commits = _selection.commits,
                },
            };
        }
        
        return gitOp;
    }
    
    // _TrackMouseInsideCommitPanel
    // Handles clicking/dragging a set of CommitPanels
    std::optional<Git::Op> _TrackMouseInsideCommitPanel(const Event& mouseDownEvent, RevColumnPtr mouseDownColumn, CommitPanelPtr mouseDownPanel) {
        const Rect mouseDownPanelFrame = mouseDownPanel->frame();
        const Size mouseDownOffset = mouseDownPanelFrame.point - mouseDownEvent.mouse.point;
        const bool wasSelected = _Selected(mouseDownColumn, mouseDownPanel);
        const Rect rootWinBounds = bounds();
        const auto doubleClickStatePrev = _doubleClickState;
        _doubleClickState = {};
        
        // Reset the selection to solely contain the mouse-down CommitPanel if:
        //   - there's no selection; or
        //   - the mouse-down CommitPanel is in a different column than the current selection; or
        //   - an unselected CommitPanel was clicked
        if (_selection.commits.empty() || (_selection.rev != mouseDownColumn->rev) || !wasSelected) {
            _selection = {
                .rev = mouseDownColumn->rev,
                .commits = {mouseDownPanel->commit()},
            };
        
        } else {
            assert(!_selection.commits.empty() && (_selection.rev == mouseDownColumn->rev));
            _selection.commits.insert(mouseDownPanel->commit());
        }
        
        Event ev = mouseDownEvent;
        std::optional<_InsertionPosition> ipos;
        bool abort = false;
        bool mouseDragged = false;
        for (;;) {
            assert(!_selection.commits.empty());
            RevColumnPtr selectionColumn = _ColumnForRev(_selection.rev);
            
            const Point p = ev.mouse.point;
            const Size delta = mouseDownEvent.mouse.point-p;
            const int w = std::abs(delta.x);
            const int h = std::abs(delta.y);
            // allow: cancel drag when mouse is moved to the edge (as an affordance to the user)
            const bool allow = HitTest(rootWinBounds, p, {-3,-3});
            mouseDragged |= w>1 || h>1;
            
            // Find insertion position
            ipos = _FindInsertionPosition(p);
            
            if (!_drag.titlePanel && mouseDragged && allow) {
                Git::Commit titleCommit = _FindLatestCommit(_selection.rev.commit, _selection.commits);
                CommitPanelPtr titlePanel = _PanelForCommit(selectionColumn, titleCommit);
                _drag.titlePanel = std::make_shared<CommitPanel>(_colors, true, titlePanel->frame().size.x, titleCommit);
                
                // Create shadow panels
                Size shadowSize = _drag.titlePanel->frame().size;
                for (size_t i=0; i<_selection.commits.size()-1; i++) {
                    _drag.shadowPanels.push_back(std::make_shared<BorderedPanel>(shadowSize));
                }
                
                // Order all the title panel and shadow panels
                for (auto it=_drag.shadowPanels.rbegin(); it!=_drag.shadowPanels.rend(); it++) {
                    (*it)->orderFront();
                }
                _drag.titlePanel->orderFront();
            
            } else if (!allow) {
                _drag = {};
            }
            
            if (_drag.titlePanel) {
                // Update _Drag.copy depending on whether the option key is held
                {
                    // forceCopy: require copying if the source column isn't mutable (and therefore commits
                    // can't be moved away from it, because that would require deleting the commits from
                    // the source column)
                    const bool forceCopy = !selectionColumn->rev.isMutable();
                    const bool copy = (ev.mouse.bstate & BUTTON_ALT) || forceCopy;
                    _drag.copy = copy;
                    _drag.titlePanel->headerLabel(_drag.copy ? "Copy" : "Move");
                }
                
                // Position title panel / shadow panels
                {
                    const Point pos0 = p + mouseDownOffset;
                    
                    _drag.titlePanel->position(pos0);
                    
                    // Position shadowPanels
                    int off = 1;
                    for (PanelPtr panel : _drag.shadowPanels) {
                        const Point pos = pos0+off;
                        panel->position(pos);
                        off++;
                    }
                }
                
                // Update insertion marker
                if (ipos) {
                    constexpr int InsertionExtraWidth = 6;
                    CommitPanelVec& ipanels = ipos->col->panels;
                    const Rect lastFrame = ipanels.back()->frame();
                    const int endY = lastFrame.point.y + lastFrame.size.y;
                    const int insertY = (ipos->iter!=ipanels.end() ? (*ipos->iter)->frame().point.y : endY+1);
                    
                    _drag.insertionMarker = {
                        .point = {lastFrame.point.x-InsertionExtraWidth/2, insertY-1},
                        .size = {lastFrame.size.x+InsertionExtraWidth, 0},
                    };
                
                } else {
                    _drag.insertionMarker = std::nullopt;
                }
            }
            
            refresh();
            ev = nextEvent();
            abort = (ev.type != Event::Type::Mouse);
            // Check if we should abort
            if (abort || ev.mouseUp()) {
                break;
            }
        }
        
        std::optional<Git::Op> gitOp;
        if (!abort) {
            if (_drag.titlePanel && ipos) {
                Git::Commit dstCommit = ((ipos->iter != ipos->col->panels.end()) ? (*ipos->iter)->commit() : nullptr);
                gitOp = Git::Op{
                    .type = (_drag.copy ? Git::Op::Type::Copy : Git::Op::Type::Move),
                    .src = {
                        .rev = _selection.rev,
                        .commits = _selection.commits,
                    },
                    .dst = {
                        .rev = ipos->col->rev,
                        .position = dstCommit,
                    }
                };
            
            // If this was a mouse-down + mouse-up without dragging in between,
            // set the selection to the commit that was clicked
            } else if (!mouseDragged) {
                _selection = {
                    .rev = mouseDownColumn->rev,
                    .commits = {mouseDownPanel->commit()},
                };
                
                auto currentTime = std::chrono::steady_clock::now();
                Git::Commit commit = *_selection.commits.begin();
                const bool doubleClicked =
                    doubleClickStatePrev.commit &&
                    doubleClickStatePrev.commit==commit &&
                    currentTime-doubleClickStatePrev.mouseUpTime < _DoubleClickThresh;
                const bool validTarget = _selection.rev.isMutable();
                
                if (doubleClicked) {
                    if (validTarget) {
                        gitOp = {
                            .type = Git::Op::Type::Edit,
                            .src = {
                                .rev = _selection.rev,
                                .commits = _selection.commits,
                            },
                        };
                    
                    } else {
                        beep();
                    }
                }
                
                _doubleClickState = {
                    .commit = *_selection.commits.begin(),
                    .mouseUpTime = currentTime,
                };
            }
        }
        
        // Reset state
        {
            _drag = {};
        }
        
        return gitOp;
    }
    
    // _TrackMouseOutsideCommitPanel
    // Handles updating the selection rectangle / selection state
    void _TrackMouseOutsideCommitPanel(const Event& mouseDownEvent) {
        auto selectionOld = _selection;
        Event ev = mouseDownEvent;
        
        for (;;) {
            const int x = std::min(mouseDownEvent.mouse.point.x, ev.mouse.point.x);
            const int y = std::min(mouseDownEvent.mouse.point.y, ev.mouse.point.y);
            const int w = std::abs(mouseDownEvent.mouse.point.x - ev.mouse.point.x);
            const int h = std::abs(mouseDownEvent.mouse.point.y - ev.mouse.point.y);
            const bool dragStart = w>1 || h>1;
            
            // Mouse-down outside of a commit:
            // Handle selection rect drawing / selecting commits
            const Rect selectionRect = {{x,y}, {std::max(2,w),std::max(2,h)}};
            
            if (_selectionRect || dragStart) {
                _selectionRect = selectionRect;
            }
            
            // Update selection
            {
                struct _Selection selectionNew;
                for (RevColumnPtr col : _columns) {
                    for (CommitPanelPtr panel : col->panels) {
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
                    
                    _selection = selection;
                
                } else {
                    _selection = selectionNew;
                }
            }
            
            refresh();
            ev = nextEvent();
            // Check if we should abort
            if (ev.type!=Event::Type::Mouse || ev.mouseUp()) {
                break;
            }
        }
        
        // Reset state
        {
            _selectionRect = std::nullopt;
        }
    }
    
    void _TrackSnapshotsMenu(RevColumn& col) {
        SnapshotButton* menuButton = nullptr;
        Git::Ref ref = col.rev.ref;
        std::vector<ButtonPtr> buttons = {
            _MakeSnapshotMenuButton(_repo, ref, _repoState.initialSnapshot(ref), true, menuButton),
        };
        
        const std::vector<State::Snapshot>& snapshots = _repoState.snapshots(ref);
        for (auto it=snapshots.rbegin(); it!=snapshots.rend(); it++) {
            // Creating the button will throw if we can't get the commit for the snapshot
            // If that happens, just don't shown the button representing the snapshot
            try {
                buttons.push_back(_MakeSnapshotMenuButton(_repo, ref, *it, false, menuButton));
            } catch (...) {}
        }
        
        const int width = _SnapshotMenuWidth+SnapshotMenu::Padding().x;
        const int px = col.offset.x + (col.width-width)/2;
        
        _snapshotsMenu = std::make_shared<SnapshotMenu>(_colors);
        Defer(_snapshotsMenu = nullptr);
        
        _snapshotsMenu->title = "Session Start";
        _snapshotsMenu->buttons = buttons;
        
        const Point p = {px, 2};
        const int heightMax = size().y-p.y;
        _snapshotsMenu->buttons = buttons;
        _snapshotsMenu->size(_snapshotsMenu->sizeIntrinsic(heightMax));
        _snapshotsMenu->position(p);
        _snapshotsMenu->layout();
        _snapshotsMenu->track(_snapshotsMenu->convert(eventCurrent));
        
        if (menuButton) {
            State::History& h = _repoState.history(ref);
            State::Commit commitNew = menuButton->snapshot.head;
            State::Commit commitCur = h.get().head;
            
            if (commitNew != commitCur) {
                Git::Commit commit = Convert(_repo, commitNew);
                h.push(State::RefState{.head = commitNew});
                _repo.refReplace(ref, commit);
                // Clear the selection when restoring a snapshot
                _selection = {};
                _Reload();
            }
        }
    }
    
    void _UndoRedo(RevColumn& col, bool undo) {
        Git::Rev rev = col.rev;
        State::History& h = _repoState.history(col.rev.ref);
        State::RefState refStatePrev = h.get();
        State::RefState refState = (undo ? h.prevPeek() : h.nextPeek());
        
        try {
            Git::Commit commit;
            try {
                commit = State::Convert(_repo, refState.head);
            } catch (...) {
                throw Toastbox::RuntimeError("failed to find commit %s", refState.head.c_str());
            }
            
            std::set<Git::Commit> selection = State::Convert(_repo, (!undo ? refState.selection : refStatePrev.selectionPrev));
            rev = _repo.revReplace(rev, commit);
            _selection = {
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
        std::optional<Git::OpResult> opResult = Git::Exec<T_SpawnFn>(_repo, gitOp);
        if (!opResult) return;
        
        Git::Rev srcRevPrev = gitOp.src.rev;
        Git::Rev dstRevPrev = gitOp.dst.rev;
        Git::Rev srcRev = opResult->src.rev;
        Git::Rev dstRev = opResult->dst.rev;
        assert((bool)srcRev.ref == (bool)srcRevPrev.ref);
        assert((bool)dstRev.ref == (bool)dstRevPrev.ref);
        
        if (srcRev && srcRev.commit!=srcRevPrev.commit) {
            State::History& h = _repoState.history(srcRev.ref);
            h.push({
                .head = State::Convert(srcRev.commit),
                .selection = State::Convert(opResult->src.selection),
                .selectionPrev = State::Convert(opResult->src.selectionPrev),
            });
        }
        
        if (dstRev && dstRev.commit!=dstRevPrev.commit && dstRev.commit!=srcRev.commit) {
            State::History& h = _repoState.history(dstRev.ref);
            h.push({
                .head = State::Convert(dstRev.commit),
                .selection = State::Convert(opResult->dst.selection),
                .selectionPrev = State::Convert(opResult->dst.selectionPrev),
            });
        }
        
        // Update the selection
        if (opResult->dst.rev) {
            _selection = {
                .rev = opResult->dst.rev,
                .commits = opResult->dst.selection,
            };
        
        } else {
            _selection = {
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
        RevColumnPtr col;
        CommitPanelVecIter iter;
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
    
    _SelectState _SelectStateGet(RevColumnPtr col, CommitPanelPtr panel) {
        bool similar = _selection.commits.find(panel->commit()) != _selection.commits.end();
        if (!similar) return _SelectState::False;
        return (col->rev==_selection.rev ? _SelectState::True : _SelectState::Similar);
    }
    
    bool _Selected(RevColumnPtr col, CommitPanelPtr panel) {
        return _SelectStateGet(col, panel) == _SelectState::True;
    }
    
    struct _HitTestResult {
        RevColumnPtr column;
        CommitPanelPtr panel;
        operator bool() const {
            return (bool)column;
        }
    };
    
    _HitTestResult _HitTest(const Point& p) {
        for (RevColumnPtr col : _columns) {
            CommitPanelPtr panel = col->hitTest(p);
            if (panel) {
                return {
                    .column = col,
                    .panel = panel,
                };
            }
        }
        return {};
    }
    
    std::optional<_InsertionPosition> _FindInsertionPosition(const Point& p) {
        RevColumnPtr icol;
        CommitPanelVecIter iiter;
        std::optional<int> leastDistance;
        for (RevColumnPtr col : _columns) {
            CommitPanelVec& panels = col->panels;
            // Ignore empty columns (eg if the window is too small to fit a column, it may not have any panels)
            if (panels.empty()) continue;
            const Rect lastFrame = panels.back()->frame();
            const int midX = lastFrame.point.x + lastFrame.size.x/2;
            const int endY = lastFrame.point.y + lastFrame.size.y;
            
            for (auto it=panels.begin();; it++) {
                CommitPanelPtr panel = (it!=panels.end() ? *it : nullptr);
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
        CommitPanelVec& icolPanels = icol->panels;
        for (;;) {
            if (iiter == icolPanels.begin()) break;
            CommitPanelPtr prevPanel = *std::prev(iiter);
            if (!_Selected(icol, prevPanel)) break;
            iiter--;
        }
        
        return _InsertionPosition{icol, iiter};
    }
    
    RevColumnPtr _ColumnForRev(Git::Rev rev) {
        for (RevColumnPtr col : _columns) {
            if (col->rev == rev) return col;
        }
        // Programmer error if it doesn't exist
        abort();
    }
    
    CommitPanelPtr _PanelForCommit(RevColumnPtr col, Git::Commit commit) {
        for (CommitPanelPtr panel : col->panels) {
            if (panel->commit() == commit) return panel;
        }
        // Programmer error if it doesn't exist
        abort();
    }
    
    ButtonPtr _MakeSnapshotMenuButton(Git::Repo repo, Git::Ref ref,
        const State::Snapshot& snap, bool sessionStart, SnapshotButton*& chosen) {
        
        bool activeSnapshot = State::Convert(ref.commit()) == snap.head;
        
    //    Button::Options buttonOpts = {
    //        .enabled = true,
    //    };
    //    
    //    SnapshotButton::Options snapButtonOpts = {
    //        .repo           = repo,
    //        .snapshot       = snap,
    //        .width          = _SnapshotMenuWidth,
    //        .activeSnapshot = activeSnapshot,
    //    };
        
        SnapshotButtonPtr b = std::make_shared<SnapshotButton>(_colors, repo, snap, _SnapshotMenuWidth);
        b->enabled        = true;
        b->activeSnapshot = activeSnapshot;
        b->action         = [&] (Button& button) { chosen = (SnapshotButton*)&button; };
        return b;
    }
    
    ButtonPtr _MakeContextMenuButton(std::string_view label, std::string_view key, bool enabled, Button*& chosen) {
        constexpr int ContextMenuWidth = 12;
        ButtonPtr b = std::make_shared<Button>(_colors);
        b->label        = std::string(label);
        b->key          = std::string(key);
        b->enabled      = enabled;
        b->insetX       = 0;
        b->action       = [&] (Button& button) { chosen = &button; };
        b->size({ContextMenuWidth, 1});
        return b;
    }
    
    bool _SelectionCanCombine() {
        bool selectionContainsMerge = false;
        for (Git::Commit commit : _selection.commits) {
            if (commit.isMerge()) {
                selectionContainsMerge = true;
                break;
            }
        }
        return _selection.rev.isMutable() && _selection.commits.size()>1 && !selectionContainsMerge;
    }
    
    bool _SelectionCanEdit() {
        return _selection.rev.isMutable() && _selection.commits.size() == 1;
    }
    
    bool _SelectionCanDelete() {
        return _selection.rev.isMutable();
    }
    
    const ColorPalette& _colors;
    State::RepoState& _repoState;
    Git::Repo _repo;
    Git::Rev _head;
    std::vector<Git::Rev> _revs;
    std::vector<RevColumnPtr> _columns;
    
    struct {
        CommitPanelPtr titlePanel;
        std::vector<BorderedPanelPtr> shadowPanels;
        std::optional<Rect> insertionMarker;
        bool copy = false;
    } _drag;
    
    struct {
        Git::Commit commit;
        std::chrono::steady_clock::time_point mouseUpTime;
    } _doubleClickState;
    
    _Selection _selection;
    std::optional<Rect> _selectionRect;
    
    MenuPtr _contextMenu;
    SnapshotMenuPtr _snapshotsMenu;
    
    ModalPanelPtr _messagePanel;
    RegisterPanelPtr _registerPanel;
};

} // namespace UI
