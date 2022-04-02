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
#include "StateDir.h"
#include "xterm-256color.h"
#include "Terminal.h"

extern "C" {
    extern char** environ;
};

namespace UI {

class MainWindow : public Window {
public:
    MainWindow(Git::Repo repo, const std::vector<Git::Rev>& revs) : _repo(repo), _revs(revs) {}
    
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
                _SelectState selectState = _selectStateGet(col, panel);
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
                const _HitTestResult hitTest = _hitTest(ev.mouse.point);
                if (ev.mouseDown(Event::MouseButtons::Left)) {
                    const bool shift = (ev.mouse.bstate & _SelectionShiftKeys);
                    if (hitTest && !shift) {
                        if (hitTest.panel) {
                            // Mouse down inside of a CommitPanel, without shift key
                            std::optional<Git::Op> gitOp = _trackMouseInsideCommitPanel(ev, hitTest.column, hitTest.panel);
                            if (gitOp) _gitOpExec(*gitOp);
                        }
                    
                    } else {
                        // Mouse down outside of a CommitPanel, or mouse down anywhere with shift key
                        _trackMouseOutsideCommitPanel(ev);
                    }
                
                } else if (ev.mouseDown(Event::MouseButtons::Right)) {
                    if (hitTest) {
                        if (hitTest.panel) {
                            std::optional<Git::Op> gitOp = _trackContextMenu(ev, hitTest.column, hitTest.panel);
                            if (gitOp) _gitOpExec(*gitOp);
                        }
                    }
                }
                break;
            }
            
            case Event::Type::KeyDelete:
            case Event::Type::KeyFnDelete: {
                if (!_selectionCanDelete()) {
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
                _gitOpExec(gitOp);
                break;
            }
            
            case Event::Type::KeyC: {
                if (!_selectionCanCombine()) {
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
                _gitOpExec(gitOp);
                break;
            }
            
            case Event::Type::KeyReturn: {
                if (!_selectionCanEdit()) {
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
                _gitOpExec(gitOp);
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
        namespace fs = std::filesystem;
        
        _Theme = _ThemeRead();
        _head = _repo.head();
        
        // Create _RepoState
        std::set<Git::Ref> refs;
        for (Git::Rev rev : _revs) {
            if (rev.ref) refs.insert(rev.ref);
        }
        State::RepoState _RepoState(StateDir(), _repo, refs);
        
        // Determine if we need to detach head.
        // This is required when a ref (ie a branch or tag) is checked out, and the ref is specified in _revs.
        // In other words, we need to detach head when whatever is checked-out may be modified.
        bool detachHead = _head.ref && std::find(_revs.begin(), _revs.end(), _head)!=_revs.end();
        
        if (detachHead && _repo.dirty()) {
            throw Toastbox::RuntimeError("please commit or stash your outstanding changes before running debase on %s", _head.displayName().c_str());
        }
        
        // Detach HEAD if it's attached to a ref, otherwise we'll get an error if
        // we try to replace that ref.
        if (detachHead) _repo.headDetach();
        Defer(
            if (detachHead) {
                // Restore previous head on exit
                std::cout << "Restoring HEAD to " << _head.ref.name() << std::endl;
                std::string err;
                try {
                    _repo.checkout(_head.ref);
                } catch (const Git::ConflictError& e) {
                    err = "Error: checkout failed because these untracked files would be overwritten:\n";
                    for (const fs::path& path : e.paths) {
                        err += "  " + std::string(path) + "\n";
                    }
                    
                    err += "\n";
                    err += "Please move or delete them and run:\n";
                    err += "  git checkout " + _head.ref.name() + "\n";
                
                } catch (const std::exception& e) {
                    err = std::string("Error: ") + e.what();
                }
                
                std::cout << (!err.empty() ? err : "Done") << std::endl;
            }
        );
        
        try {
            _CursesInit();
            Defer(_CursesDeinit());
            
            Window::operator=(Window(::stdscr));
            
            for (;;) {
                _reload();
                try {
                    track();
                } catch (const WindowResize&) {
                    // Continue the loop, which calls _Reload()
                }
            }
            
        } catch (const UI::ExitRequest&) {
            // Nothing to do
        } catch (...) {
            throw;
        }
        
        _RepoState.write();
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
    
    struct _HitTestResult {
        RevColumnPtr column;
        CommitPanelPtr panel;
        operator bool() const {
            return (bool)column;
        }
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
    
    _SelectState _selectStateGet(RevColumnPtr col, CommitPanelPtr panel) {
        bool similar = _selection.commits.find(panel->commit()) != _selection.commits.end();
        if (!similar) return _SelectState::False;
        return (col->rev==_selection.rev ? _SelectState::True : _SelectState::Similar);
    }
    
    bool _selected(RevColumnPtr col, CommitPanelPtr panel) {
        return _selectStateGet(col, panel) == _SelectState::True;
    }
    
    _HitTestResult _hitTest(const Point& p) {
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
    
    std::optional<_InsertionPosition> _findInsertionPosition(const Point& p) {
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
            if (!_selected(icol, prevPanel)) break;
            iiter--;
        }
        
        return _InsertionPosition{icol, iiter};
    }
    
    RevColumnPtr _columnForRev(Git::Rev rev) {
        for (RevColumnPtr col : _columns) {
            if (col->rev == rev) return col;
        }
        // Programmer error if it doesn't exist
        abort();
    }
    
    CommitPanelPtr _panelForCommit(RevColumnPtr col, Git::Commit commit) {
        for (CommitPanelPtr panel : col->panels) {
            if (panel->commit() == commit) return panel;
        }
        // Programmer error if it doesn't exist
        abort();
    }
    
    ButtonPtr _makeSnapshotMenuButton(Git::Repo repo, Git::Ref ref,
        const State::Snapshot& snap, bool sessionStart, SnapshotButton*& chosen) {
        
        bool activeSnapshot = State::Convert(ref.commit()) == snap.head;
        SnapshotButtonPtr b = std::make_shared<SnapshotButton>(_colors, repo, snap, _SnapshotMenuWidth);
        b->enabled        = true;
        b->activeSnapshot = activeSnapshot;
        b->action         = [&] (Button& button) { chosen = (SnapshotButton*)&button; };
        return b;
    }
    
    ButtonPtr _makeContextMenuButton(std::string_view label, std::string_view key, bool enabled, Button*& chosen) {
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
    
    bool _selectionCanCombine() {
        bool selectionContainsMerge = false;
        for (Git::Commit commit : _selection.commits) {
            if (commit.isMerge()) {
                selectionContainsMerge = true;
                break;
            }
        }
        return _selection.rev.isMutable() && _selection.commits.size()>1 && !selectionContainsMerge;
    }
    
    bool _selectionCanEdit() {
        return _selection.rev.isMutable() && _selection.commits.size() == 1;
    }
    
    bool _selectionCanDelete() {
        return _selection.rev.isMutable();
    }
    
    void _reload() {
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
                undoAction = [&] (RevColumn& col) { _undoRedo(col, true); };
            }
            
            if (h && !h->end()) {
                redoAction = [&] (RevColumn& col) { _undoRedo(col, false); };
            }
            
            snapshotsAction = [&] (RevColumn& col) { _trackSnapshotsMenu(col); };
            
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
    
    // _trackMouseInsideCommitPanel
    // Handles clicking/dragging a set of CommitPanels
    std::optional<Git::Op> _trackMouseInsideCommitPanel(const Event& mouseDownEvent, RevColumnPtr mouseDownColumn, CommitPanelPtr mouseDownPanel) {
        const Rect mouseDownPanelFrame = mouseDownPanel->frame();
        const Size mouseDownOffset = mouseDownPanelFrame.point - mouseDownEvent.mouse.point;
        const bool wasSelected = _selected(mouseDownColumn, mouseDownPanel);
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
            RevColumnPtr selectionColumn = _columnForRev(_selection.rev);
            
            const Point p = ev.mouse.point;
            const Size delta = mouseDownEvent.mouse.point-p;
            const int w = std::abs(delta.x);
            const int h = std::abs(delta.y);
            // allow: cancel drag when mouse is moved to the edge (as an affordance to the user)
            const bool allow = HitTest(rootWinBounds, p, {-3,-3});
            mouseDragged |= w>1 || h>1;
            
            // Find insertion position
            ipos = _findInsertionPosition(p);
            
            if (!_drag.titlePanel && mouseDragged && allow) {
                Git::Commit titleCommit = _FindLatestCommit(_selection.rev.commit, _selection.commits);
                CommitPanelPtr titlePanel = _panelForCommit(selectionColumn, titleCommit);
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
    
    // _trackMouseOutsideCommitPanel
    // Handles updating the selection rectangle / selection state
    void _trackMouseOutsideCommitPanel(const Event& mouseDownEvent) {
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
    
    std::optional<Git::Op> _trackContextMenu(const Event& mouseDownEvent, RevColumnPtr mouseDownColumn, CommitPanelPtr mouseDownPanel) {
        // If the commit that was clicked isn't selected, set the selection to only that commit
        if (!_selected(mouseDownColumn, mouseDownPanel)) {
            _selection = {
                .rev = mouseDownColumn->rev,
                .commits = {mouseDownPanel->commit()},
            };
        }
        
        assert(!_selection.commits.empty());
        
        // Draw once before we open the context menu, so that the selection is updated
        refresh();
        
        Button* menuButton = nullptr;
        ButtonPtr combineButton = _makeContextMenuButton("Combine", "c", _selectionCanCombine(), menuButton);
        ButtonPtr editButton    = _makeContextMenuButton("Edit", "ret", _selectionCanEdit(), menuButton);
        ButtonPtr deleteButton  = _makeContextMenuButton("Delete", "del", _selectionCanDelete(), menuButton);
        
        MenuPtr menu = std::make_shared<Menu>(_colors);
        menu->buttons = { combineButton, editButton, deleteButton };
        menu->size(menu->sizeIntrinsic());
        menu->position(mouseDownEvent.mouse.point);
        menu->layout();
        menu->track(menu->convert(mouseDownEvent));
        
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
    
    void _trackSnapshotsMenu(RevColumn& col) {
        SnapshotButton* menuButton = nullptr;
        Git::Ref ref = col.rev.ref;
        std::vector<ButtonPtr> buttons = {
            _makeSnapshotMenuButton(_repo, ref, _repoState.initialSnapshot(ref), true, menuButton),
        };
        
        const std::vector<State::Snapshot>& snapshots = _repoState.snapshots(ref);
        for (auto it=snapshots.rbegin(); it!=snapshots.rend(); it++) {
            // Creating the button will throw if we can't get the commit for the snapshot
            // If that happens, just don't shown the button representing the snapshot
            try {
                buttons.push_back(_makeSnapshotMenuButton(_repo, ref, *it, false, menuButton));
            } catch (...) {}
        }
        
        const int width = _SnapshotMenuWidth+SnapshotMenu::Padding().x;
        const Point p = {col.offset.x+(col.width-width)/2, 2};
        const int heightMax = size().y-p.y;
        
        SnapshotMenuPtr menu = std::make_shared<SnapshotMenu>(_colors);
        menu->title = "Session Start";
        menu->buttons = buttons;
        menu->buttons = buttons;
        menu->size(menu->sizeIntrinsic(heightMax));
        menu->position(p);
        menu->layout();
        menu->track(menu->convert(eventCurrent));
        
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
                _reload();
            }
        }
    }
    
    void _undoRedo(RevColumn& col, bool undo) {
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
        
        _reload();
    }
    
    void _gitOpExec(const Git::Op& gitOp) {
        auto spawnFn = [&] (const char*const* argv) { _spawn(argv); };
        std::optional<Git::OpResult> opResult = Git::Exec(_repo, gitOp, spawnFn);
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
        
        _reload();
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

    void _CursesInit() noexcept {
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
            _colors = _ColorsCreate(_Theme);
        }
        
        _ColorsPrev = _ColorsSet(_colors);
        
        _CursorState = UI::CursorState(false, {});
        
    //    // Hide cursor
    //    ::curs_set(0);
        
        ::mousemask(ALL_MOUSE_EVENTS | REPORT_MOUSE_POSITION, NULL);
        ::mouseinterval(0);
        
        ::set_escdelay(0);
    }

    void _CursesDeinit() noexcept {
    //    ::mousemask(0, NULL);
        
        _CursorState.restore();
        
        _ColorsSet(_ColorsPrev);
        ::endwin();
        
    //    sleep(1);
    }

    void _spawn(const char*const* argv) {
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
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    Git::Repo _repo;
    std::vector<Git::Rev> _revs;
    
    ColorPalette _colors;
    UI::ColorPalette _ColorsPrev;
    State::RepoState _repoState;
    Git::Rev _head;
    std::vector<RevColumnPtr> _columns;
    UI::CursorState _CursorState;
    State::Theme _Theme = State::Theme::None;
    
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
    
    ModalPanelPtr _messagePanel;
    RegisterPanelPtr _registerPanel;
};

} // namespace UI
