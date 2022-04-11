#pragma once
#include <thread>
#include "Debase.h"
#include "git/Git.h"
#include "ui/Window.h"
#include "ui/Screen.h"
#include "ui/Color.h"
#include "ui/RevColumn.h"
#include "ui/SnapshotButton.h"
#include "ui/Menu.h"
#include "ui/SnapshotMenu.h"
#include "ui/WelcomePanel.h"
#include "ui/ModalPanel.h"
#include "ui/RegisterPanel.h"
#include "ui/ButtonSpinner.h"
#include "state/StateDir.h"
#include "state/Theme.h"
#include "state/State.h"
#include "license/License.h"
#include "license/Request.h"
#include "network/Network.h"
#include "xterm-256color.h"
#include "Terminal.h"
#include "Async.h"
#include <os/log.h>

extern "C" {
    extern char** environ;
};

class App : public UI::Screen {
public:
    App(Git::Repo repo, const std::vector<Git::Rev>& revs) :
    Screen(Uninit), _repo(repo), _revs(revs) {}
    
    void layout() override {
        
        // Layout columns
        int offX = _ColumnInsetX;
        for (UI::RevColumnPtr col : _columns) {
            col->frame({{offX, 0}, {_ColumnWidth, size().y}});
            offX += _ColumnWidth+_ColumnSpacing;
        }
        
//        col->frame({{offX, 0}, {ColumnWidth, size().y}});
//        
//        for (UI::RevColumnPtr col : _columns) {
//            for (UI::CommitPanelPtr panel : col->panels()) {
//                const _SelectState selectState = _selectStateGet(col, panel);
//                const bool visible = (selectState!=_SelectState::True ? true : (!dragging || copying));
//                panel->visible(visible);
//            }
//        }
        
        bool dragging = (bool)_drag.titlePanel;
        bool copying = _drag.copy;
        for (UI::RevColumnPtr col : _columns) {
            for (UI::CommitPanelPtr panel : col->panels()) {
                const _SelectState selectState = _selectStateGet(col, panel);
                const bool visible = (selectState!=_SelectState::True ? true : (!dragging || copying));
                panel->visible(visible);
            }
        }
        
//        {
//            constexpr int InsetX = 3;
//            constexpr int ColumnWidth = 32;
//            constexpr int ColumnSpacing = 6;
//            int offX = InsetX;
//            for (UI::RevColumnPtr col : _columns) {
//                col->frame({{offX, 0}, {ColumnWidth, size().y}});
//                col->layout(*this);
//                offX += ColumnWidth+ColumnSpacing;
//            }
//        }
        
//        const UI::Color selectionColor = (_drag.copy ? _colors.selectionCopy : _colors.selection);
        
//        // Order all the title panel and shadow panels
//        if (dragging) {
//            for (auto it=_drag.shadowPanels.rbegin(); it!=_drag.shadowPanels.rend(); it++) {
//                (*it)->orderFront();
//            }
//            _drag.titlePanel->orderFront();
//        }
        
        if (_welcomePanel) {
            constexpr int WelcomePanelWidth = 40;
            _layoutModalPanel(_welcomePanel, WelcomePanelWidth);
        }
        
        if (_registerPanel) {
            constexpr int RegisterPanelWidth = 50;
            _layoutModalPanel(_registerPanel, RegisterPanelWidth);
        }
        
        if (_messagePanel) {
            constexpr int MessagePanelWidth = 40;
            _layoutModalPanel(_messagePanel, MessagePanelWidth);
        }
    }
    
    void draw() override {
        const UI::Color selectionColor = (_drag.copy ? _colors.selectionCopy : _colors.selection);
        
        if (_drag.titlePanel) {
            _drag.titlePanel->borderColor(selectionColor);
            
            for (UI::PanelPtr panel : _drag.shadowPanels) {
                panel->borderColor(selectionColor);
            }
        }
        
        bool dragging = (bool)_drag.titlePanel;
        for (UI::RevColumnPtr col : _columns) {
            for (UI::CommitPanelPtr panel : col->panels()) {
                UI::Color borderColor;
                _SelectState selectState = _selectStateGet(col, panel);
                if (selectState == _SelectState::True) {
                    borderColor = (dragging ? _colors.selectionSimilar : _colors.selection);
                
                } else {
                    borderColor = (!dragging && selectState==_SelectState::Similar ? _colors.selectionSimilar : _colors.normal);
                }
                
                panel->borderColor(borderColor);
            }
        }
        
        if (_drag.titlePanel) {
//            _drag.titlePanel->draw(*_drag.titlePanel);
            
            // Draw insertion marker
            if (_drag.insertionMarker) {
                Attr color = attr(selectionColor);
                drawLineHoriz(_drag.insertionMarker->origin, _drag.insertionMarker->size.x);
//                os_log(OS_LOG_DEFAULT, "DRAW INSERTION MARKER @ p={%d %d} w=%d",
//                    _drag.insertionMarker->origin.x, _drag.insertionMarker->origin.y, _drag.insertionMarker->size.x);
            }
        }
        
//        for (UI::PanelPtr panel : _drag.shadowPanels) {
//            panel->draw(*panel);
//        }
        
        if (_selectionRect) {
            Attr color = attr(_colors.selection);
            drawRect(*_selectionRect);
        }
        
//        if (_messagePanel) {
//            _messagePanel->draw(*_messagePanel);
//        }
//        
//        if (_welcomePanel) {
//            _welcomePanel->draw(*_welcomePanel);
//        }
//        
//        if (_registerPanel) {
//            _registerPanel->draw(*_registerPanel);
//        }
    }
    
    bool handleEvent(const UI::Event& ev) override {
        std::string errorMsg;
        try {
//            if (_messagePanel) {
//                return _messagePanel->handleEvent(win, _messagePanel->convert(ev));
//            }
//            
//            if (_welcomePanel) {
//                return _welcomePanel->handleEvent(win, _welcomePanel->convert(ev));
//            }
//            
//            if (_registerPanel) {
//                return _registerPanel->handleEvent(win, _registerPanel->convert(ev));
//            }
            
//            // Let every column handle the event
//            for (UI::RevColumnPtr col : _columns) {
//                bool handled = col->handleEvent(ev);
//                if (handled) return true;
//            }
            
            switch (ev.type) {
            case UI::Event::Type::Mouse: {
                const _HitTestResult hitTest = _hitTest(ev.mouse.origin);
                if (ev.mouseDown(UI::Event::MouseButtons::Left)) {
                    const bool shift = (ev.mouse.bstate & _SelectionShiftKeys);
                    if (hitTest && !shift) {
                        if (hitTest.panel) {
                            // Mouse down inside of a CommitPanel, without shift key
                            std::optional<Git::Op> gitOp = _trackMouseInsideCommitPanel(ev, hitTest.column, hitTest.panel);
                            if (gitOp) _gitOpExec(*gitOp);
                        }
                    
                    } else {
                        // Mouse down outside of a CommitPanel, or mouse down anywhere with shift key
                        _trackSelectionRect(ev);
                    }
                
                } else if (ev.mouseDown(UI::Event::MouseButtons::Right)) {
                    if (hitTest) {
                        if (hitTest.panel) {
                            std::optional<Git::Op> gitOp = _trackContextMenu(ev, hitTest.column, hitTest.panel);
                            if (gitOp) _gitOpExec(*gitOp);
                        }
                    }
                }
                break;
            }
            
            case UI::Event::Type::KeyDelete:
            case UI::Event::Type::KeyFnDelete: {
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
            
            case UI::Event::Type::KeyC: {
//                {
//                    _registerPanel = std::make_shared<UI::RegisterPanel>(_colors);
//                    _registerPanel->color           = _colors.menu;
//                    _registerPanel->messageInsetY   = 1;
//                    _registerPanel->title           = "Register";
//                    _registerPanel->message         = "Please register debase";
//                }
//                std::this_thread::sleep_for(std::chrono::milliseconds(10));
//                break;
                
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
            
            case UI::Event::Type::KeyReturn: {
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
        
        } catch (const std::exception& e) {
            errorMsg = e.what();
        }
        
        if (!errorMsg.empty()) {
            errorMsg[0] = toupper(errorMsg[0]);
            _errorMessageShow(errorMsg);
        }
        
        return true;
    }
    
    void run() {
        _theme = State::ThemeRead();
        _head = _repo.headResolved();
        
        // Create _repoState
        std::set<Git::Ref> refs;
        for (Git::Rev rev : _revs) {
            if (rev.ref) refs.insert(rev.ref);
        }
        _repoState = State::RepoState(StateDir(), _repo, refs);
        
        // Determine if we need to detach head.
        // This is required when a ref (ie a branch or tag) is checked out, and the ref is specified in _revs.
        // In other words, we need to detach head when whatever is checked-out may be modified.
        bool detachHead = _head.ref && refs.find(_head.ref)!=refs.end();
        
        if (detachHead && _repo.dirty()) {
            throw Toastbox::RuntimeError(
                "can't run debase on the checked-out branch (%s) while there are outstanding changes.\n"
                "\n"
                "Please commit or stash your changes, or detach HEAD (git checkout -d).\n",
            _head.displayName().c_str());
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
                    _repo.headAttach(_head);
                } catch (const Git::ConflictError& e) {
                    err = "Error: checkout failed because these untracked files would be overwritten:\n";
                    for (const _Path& path : e.paths) {
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
            _cursesInit();
            Defer(_cursesDeinit());
            
            // Create our window now that ncurses is initialized
            Window::operator =(Window(::stdscr));
            
            _licenseCheck();
            
            _reload();
            track({});
            
        } catch (const UI::ExitRequest&) {
            // Nothing to do
        } catch (...) {
            throw;
        }
        
        _repoState.write();
    }
    
    void track(const UI::Event& ev, Deadline deadline=Forever) override {
        for (;;) {
            try {
                Screen::track({}, deadline);
                break;
            } catch (const UI::WindowResize&) {
                _reload();
            }
        }
    }
    
private:
    using _Path = std::filesystem::path;
    
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
        UI::CommitPanelIter iter;
    };
    
    struct _HitTestResult {
        UI::RevColumnPtr column;
        UI::CommitPanelPtr panel;
        operator bool() const {
            return (bool)column;
        }
    };
    
    static constexpr int _ColumnInsetX = 3;
    static constexpr int _ColumnWidth = 32;
    static constexpr int _ColumnSpacing = 6;
    
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
    
    _SelectState _selectStateGet(UI::RevColumnPtr col, UI::CommitPanelPtr panel) {
        bool similar = _selection.commits.find(panel->commit()) != _selection.commits.end();
        if (!similar) return _SelectState::False;
        return (col->rev()==_selection.rev ? _SelectState::True : _SelectState::Similar);
    }
    
    bool _selected(UI::RevColumnPtr col, UI::CommitPanelPtr panel) {
        return _selectStateGet(col, panel) == _SelectState::True;
    }
    
    _HitTestResult _hitTest(const UI::Point& p) {
        for (UI::RevColumnPtr col : _columns) {
            UI::CommitPanelPtr panel = col->hitTest(View::SubviewConvert(*col, p));
            if (panel) {
                return {
                    .column = col,
                    .panel = panel,
                };
            }
        }
        return {};
    }
    
    std::optional<_InsertionPosition> _findInsertionPosition(const UI::Point& p) {
        UI::RevColumnPtr icol;
        UI::CommitPanelIter iiter;
        std::optional<int> leastDistance;
        for (UI::RevColumnPtr col : _columns) {
            const UI::CommitPanelVec& panels = col->panels();
            // Ignore empty columns (eg if the window is too small to fit a column, it may not have any panels)
            if (panels.empty()) continue;
            const UI::Rect lastFrame = SuperviewConvert(*col, panels.back()->frame());
            const int midX = lastFrame.origin.x + lastFrame.size.x/2;
            const int endY = lastFrame.origin.y + lastFrame.size.y;
            
            for (auto it=panels.begin();; it++) {
                UI::CommitPanelPtr panel = (it!=panels.end() ? *it : nullptr);
                UI::Rect panelFrame = (panel ? SuperviewConvert(*col, panel->frame()) : UI::Rect{});
                const int x = (panel ? panelFrame.origin.x+panelFrame.size.x/2 : midX);
                const int y = (panel ? panelFrame.origin.y : endY);
                const int dist = (p.x-x)*(p.x-x)+(p.y-y)*(p.y-y);
                
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
        const UI::CommitPanelVec& icolPanels = icol->panels();
        for (;;) {
            if (iiter == icolPanels.begin()) break;
            UI::CommitPanelPtr prevPanel = *std::prev(iiter);
            if (!_selected(icol, prevPanel)) break;
            iiter--;
        }
        
        return _InsertionPosition{icol, iiter};
    }
    
    UI::RevColumnPtr _columnForRev(Git::Rev rev) {
        for (UI::RevColumnPtr col : _columns) {
            if (col->rev() == rev) return col;
        }
        // Programmer error if it doesn't exist
        abort();
    }
    
    UI::CommitPanelPtr _panelForCommit(UI::RevColumnPtr col, Git::Commit commit) {
        for (UI::CommitPanelPtr panel : col->panels()) {
            if (panel->commit() == commit) return panel;
        }
        // Programmer error if it doesn't exist
        abort();
    }
    
    UI::ButtonPtr _makeSnapshotMenuButton(Git::Repo repo, Git::Ref ref,
        const State::Snapshot& snap, bool sessionStart, UI::SnapshotButton*& chosen) {
        
        bool activeSnapshot = State::Convert(ref.commit()) == snap.head;
        UI::SnapshotButtonPtr b = std::make_shared<UI::SnapshotButton>(repo, snap, _SnapshotMenuWidth);
        b->activeSnapshot = activeSnapshot;
        b->action([&] (UI::Button& button) { chosen = (UI::SnapshotButton*)&button; });
        b->enabled(true);
        return b;
    }
    
    UI::ButtonPtr _makeContextMenuButton(std::string_view label, std::string_view key, bool enabled, UI::Button*& chosen) {
        constexpr int ContextMenuWidth = 12;
        UI::ButtonPtr b = std::make_shared<UI::Button>();
        b->label()->text(std::string(label));
        b->label()->align(UI::Align::Left);
        b->key()->text(std::string(key));
        b->action([&] (UI::Button& button) { chosen = &button; });
        b->enabled(enabled);
        b->size({ContextMenuWidth, 1});
        return b;
    }
    
    bool _selectionCanCombine() {
        if (_selection.commits.empty()) return false;
        
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
        if (_selection.commits.empty()) return false;
        return _selection.rev.isMutable() && _selection.commits.size() == 1;
    }
    
    bool _selectionCanDelete() {
        if (_selection.commits.empty()) return false;
        return _selection.rev.isMutable();
    }
    
    void _reload() {
        // Reload head's ref
        if (_head.ref) {
            _head = _repo.revReload(_head);
        }
        
        // Reload all ref-based revs
        for (Git::Rev& rev : _revs) {
            if (rev.ref) {
                rev = _repo.revReload(rev);
            }
        }
        
        // Create columns
        std::list<UI::ViewPtr> sv;
        int offX = _ColumnInsetX;
        size_t colCount = 0;
        for (const Git::Rev& rev : _revs) {
            State::History* h = (rev.ref ? &_repoState.history(rev.ref) : nullptr);
            const int rem = size().x-offX;
            if (rem < _ColumnWidth) break;
            
            // Create the column if it doesn't exist yet
            bool create = (colCount >= _columns.size());
            
            UI::RevColumnPtr col;
            if (create) {
                col = std::make_shared<UI::RevColumn>();
                _columns.push_back(col);
                
                col->repo(_repo);
                col->head((rev.displayHead() == _head.commit));
                
                std::weak_ptr<UI::RevColumn> weakCol = col;
                col->undoButton()->action([=] (UI::Button&) {
                    auto col = weakCol.lock();
                    if (col) _undoRedo(col, true);
                });
                
                col->redoButton()->action([=] (UI::Button&) {
                    auto col = weakCol.lock();
                    if (col) _undoRedo(col, false);
                });
                
                col->snapshotsButton()->action([=] (UI::Button&) {
                    auto col = weakCol.lock();
                    if (col) _trackSnapshotsMenu(col);
                });
            
            } else {
                col = _columns[colCount];
            }
            
            col->rev(rev); // Ensure all columns' revs are up to date (since refs become stale if they're modified)
            col->undoButton()->enabled(h && !h->begin());
            col->redoButton()->enabled(h && !h->end());
            col->snapshotsButton()->enabled(true);
            col->reload({_ColumnWidth, size().y});
            sv.push_back(col);
            
            offX += _ColumnWidth+_ColumnSpacing;
            colCount++;
        }
        
        // Erase columns that aren't visible
        _columns.erase(_columns.begin()+colCount, _columns.end());
        
        // Update subviews
        if (_welcomePanel) sv.push_back(_welcomePanel);
        if (_registerPanel) sv.push_back(_registerPanel);
        if (_messagePanel) sv.push_back(_messagePanel);
        subviews(sv);
        
        layoutNeeded(true);
        eraseNeeded(true);
    }
    
    // _trackMouseInsideCommitPanel
    // Handles clicking/dragging a set of CommitPanels
    std::optional<Git::Op> _trackMouseInsideCommitPanel(const UI::Event& mouseDownEvent, UI::RevColumnPtr mouseDownColumn, UI::CommitPanelPtr mouseDownPanel) {
        const UI::Rect mouseDownPanelFrame = mouseDownPanel->frame();
        const UI::Size mouseDownOffset = SuperviewConvert(*mouseDownColumn, mouseDownPanelFrame.origin) - mouseDownEvent.mouse.origin;
        const bool wasSelected = _selected(mouseDownColumn, mouseDownPanel);
        const UI::Rect rootWinBounds = bounds();
        const auto doubleClickStatePrev = _doubleClickState;
        _doubleClickState = {};
        
        // Reset the selection to solely contain the mouse-down CommitPanel if:
        //   - there's no selection; or
        //   - the mouse-down CommitPanel is in a different column than the current selection; or
        //   - an unselected CommitPanel was clicked
        if (_selection.commits.empty() || (_selection.rev != mouseDownColumn->rev()) || !wasSelected) {
            _selection = {
                .rev = mouseDownColumn->rev(),
                .commits = {mouseDownPanel->commit()},
            };
        
        } else {
            assert(!_selection.commits.empty() && (_selection.rev == mouseDownColumn->rev()));
            _selection.commits.insert(mouseDownPanel->commit());
        }
        
        UI::RevColumnPtr selectionColumn = _columnForRev(_selection.rev);
        Git::Commit titleCommit = _FindLatestCommit(_selection.rev.commit, _selection.commits);
        UI::CommitPanelPtr titlePanel = _panelForCommit(selectionColumn, titleCommit);
        
        UI::Event ev = mouseDownEvent;
        std::optional<_InsertionPosition> ipos;
        bool abort = false;
        bool mouseDragged = false;
        for (;;) {
            assert(!_selection.commits.empty());
            
            const UI::Point p = ev.mouse.origin;
            const UI::Size delta = mouseDownEvent.mouse.origin-p;
            const int w = std::abs(delta.x);
            const int h = std::abs(delta.y);
            // allow: cancel drag when mouse is moved to the edge (as an affordance to the user)
            const bool allow = HitTest(rootWinBounds, p, {-3,-3});
            mouseDragged |= w>1 || h>1;
            
            // Find insertion position
            ipos = _findInsertionPosition(p);
            
            if (!_drag.titlePanel && mouseDragged && allow) {
                _drag.titlePanel = std::make_shared<UI::CommitPanel>();
                _drag.titlePanel->commit(titleCommit);
                
                // Create shadow panels
                for (size_t i=0; i<_selection.commits.size()-1; i++) {
                    _drag.shadowPanels.push_back(std::make_shared<UI::Panel>());
                }
                
                for (auto it=_drag.shadowPanels.rbegin(); it!=_drag.shadowPanels.rend(); it++) {
                    subviewAdd(*it);
                }
                subviewAdd(_drag.titlePanel);
                
                // The titlePanel/shadowPanels need layout
                layoutNeeded(true);
            
            } else if (!allow) {
                _drag = {};
                // The titlePanel/shadowPanels need layout
                layoutNeeded(true);
            }
            
            if (_drag.titlePanel) {
                // Update _Drag.copy depending on whether the option key is held
                {
                    // forceCopy: require copying if the source column isn't mutable (and therefore commits
                    // can't be moved away from it, because that would require deleting the commits from
                    // the source column)
                    const bool forceCopy = !selectionColumn->rev().isMutable();
                    const bool copy = (ev.mouse.bstate & BUTTON_ALT) || forceCopy;
                    const bool copyPrev = _drag.copy;
                    _drag.copy = copy;
                    _drag.titlePanel->header()->text(_drag.copy ? "Copy" : "Move");
                    if (_drag.copy != copyPrev) {
                        layoutNeeded(true);
                    }
                }
                
                // Position/size title panel / shadow panels
                {
                    const UI::Point pos0 = p + mouseDownOffset + UI::Size{0,-1}; // -1 to account for the additional header line while dragging
                    const UI::Size size = _drag.titlePanel->sizeIntrinsic({titlePanel->size().x, 0});
                    _drag.titlePanel->frame({pos0, size});
                    
                    // Position/size shadowPanels
                    int off = 1;
                    for (UI::PanelPtr panel : _drag.shadowPanels) {
                        const UI::Point pos = pos0+off;
                        panel->frame({pos, size});
                        off++;
                    }
                }
                
                // Update insertion marker
                if (ipos) {
                    constexpr int InsertionExtraWidth = 6;
                    const UI::CommitPanelVec& ipanels = ipos->col->panels();
                    const UI::Rect lastFrame = SuperviewConvert(*ipos->col, ipanels.back()->frame());
                    const int endY = lastFrame.origin.y + lastFrame.size.y;
                    const int insertY = (ipos->iter!=ipanels.end() ? (*ipos->iter)->frame().origin.y : endY+1);
                    
                    _drag.insertionMarker = {
                        .origin = {lastFrame.origin.x-InsertionExtraWidth/2, insertY-1},
                        .size = {lastFrame.size.x+InsertionExtraWidth, 0},
                    };
                
                } else {
                    _drag.insertionMarker = std::nullopt;
                }
            }
            
            eraseNeeded(true); // Need to erase the insertion marker
            ev = nextEvent();
            abort = (ev.type != UI::Event::Type::Mouse);
            // Check if we should abort
            if (abort || ev.mouseUp()) {
                break;
            }
        }
        
        std::optional<Git::Op> gitOp;
        if (!abort) {
            if (_drag.titlePanel && ipos) {
                Git::Commit dstCommit = ((ipos->iter != ipos->col->panels().end()) ? (*ipos->iter)->commit() : nullptr);
                gitOp = Git::Op{
                    .type = (_drag.copy ? Git::Op::Type::Copy : Git::Op::Type::Move),
                    .src = {
                        .rev = _selection.rev,
                        .commits = _selection.commits,
                    },
                    .dst = {
                        .rev = ipos->col->rev(),
                        .position = dstCommit,
                    }
                };
            
            // If this was a mouse-down + mouse-up without dragging in between,
            // set the selection to the commit that was clicked
            } else if (!mouseDragged) {
                _selection = {
                    .rev = mouseDownColumn->rev(),
                    .commits = {mouseDownPanel->commit()},
                };
                
                auto currentTime = std::chrono::steady_clock::now();
                Git::Rev rev = _selection.rev;
                Git::Commit commit = *_selection.commits.begin();
                const bool doubleClicked =
                    doubleClickStatePrev.rev &&
                    doubleClickStatePrev.rev==rev &&
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
                    .rev = _selection.rev,
                    .commit = *_selection.commits.begin(),
                    .mouseUpTime = currentTime,
                };
            }
        }
        
        // Reset state
        // The dragged panels need layout
        layoutNeeded(true);
        // We need one more erase to erase the insertion marker
        eraseNeeded(true);
        _drag = {};
        
        return gitOp;
    }
    
    // _trackSelectionRect
    // Handles updating the selection rectangle / selection state
    void _trackSelectionRect(const UI::Event& mouseDownEvent) {
        auto selectionOld = _selection;
        UI::Event ev = mouseDownEvent;
        
        for (;;) {
            UI::Point mouseDownPos = mouseDownEvent.mouse.origin;
            UI::Point pos = ev.mouse.origin;
            const int x = std::min(mouseDownPos.x, pos.x);
            const int y = std::min(mouseDownPos.y, pos.y);
            const int w = std::abs(mouseDownPos.x - pos.x);
            const int h = std::abs(mouseDownPos.y - pos.y);
            const bool dragStart = w>1 || h>1;
            
            // Mouse-down outside of a commit:
            // Handle selection rect drawing / selecting commits
            const UI::Rect selectionRect = {{x,y}, {std::max(2,w),std::max(2,h)}};
            
            if (_selectionRect || dragStart) {
                _selectionRect = selectionRect;
            }
            
            // Update selection
            {
                struct _Selection selectionNew;
                for (UI::RevColumnPtr col : _columns) {
                    for (UI::CommitPanelPtr panel : col->panels()) {
                        const UI::Rect panelFrame = SuperviewConvert(*col, panel->frame());
                        if (!Empty(Intersection(selectionRect, panelFrame))) {
                            selectionNew.rev = col->rev();
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
            
            eraseNeeded(true); // Need to erase the selection rect
            ev = nextEvent();
            // Check if we should abort
            if (ev.type!=UI::Event::Type::Mouse || ev.mouseUp()) {
                break;
            }
        }
        
        // Reset state
        eraseNeeded(true); // We need one more erase to erase the selection rect upon mouse-up
        _selectionRect = std::nullopt;
    }
    
    std::optional<Git::Op> _trackContextMenu(const UI::Event& mouseDownEvent, UI::RevColumnPtr mouseDownColumn, UI::CommitPanelPtr mouseDownPanel) {
        // If the commit that was clicked isn't selected, set the selection to only that commit
        if (!_selected(mouseDownColumn, mouseDownPanel)) {
            _selection = {
                .rev = mouseDownColumn->rev(),
                .commits = {mouseDownPanel->commit()},
            };
        }
        
        assert(!_selection.commits.empty());
        
        // Draw once before we open the context menu, so that the selection is updated
        drawNeeded(true);
        
        UI::Button* menuButton = nullptr;
        UI::ButtonPtr combineButton = _makeContextMenuButton("Combine", "c", _selectionCanCombine(), menuButton);
        UI::ButtonPtr editButton    = _makeContextMenuButton("Edit", "ret", _selectionCanEdit(), menuButton);
        UI::ButtonPtr deleteButton  = _makeContextMenuButton("Delete", "del", _selectionCanDelete(), menuButton);
        
        UI::MenuPtr menu = subviewCreate<UI::Menu>();
        menu->buttons({ combineButton, editButton, deleteButton });
        menu->size(menu->sizeIntrinsic({}));
        menu->origin(mouseDownEvent.mouse.origin);
        menu->track(mouseDownEvent);
        
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
    
    void _trackSnapshotsMenu(UI::RevColumnPtr col) {
        UI::SnapshotButton* menuButton = nullptr;
        Git::Ref ref = col->rev().ref;
        std::vector<UI::ButtonPtr> buttons = {
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
        
        const int width = _SnapshotMenuWidth+UI::SnapshotMenu::Padding().x;
        const UI::Point p = {col->origin().x+(col->size().x-width)/2, 2};
        const int heightMax = size().y-p.y;
        
        UI::SnapshotMenuPtr menu = subviewCreate<UI::SnapshotMenu>();
        menu->title()->text("Session Start");
        menu->buttons(buttons);
        menu->size(menu->sizeIntrinsic({0, heightMax}));
        menu->origin(p);
//        layout(*this, {});
        menu->track(eventCurrent());
        
        if (menuButton) {
            State::History& h = _repoState.history(ref);
            State::Commit commitNew = menuButton->snapshot.head;
            State::Commit commitCur = h.get().head;
            
            if (commitNew != commitCur) {
                Git::Commit commit = State::Convert(_repo, commitNew);
                h.push(State::RefState{.head = commitNew});
                _repo.refReplace(ref, commit);
                // Clear the selection when restoring a snapshot
                _selection = {};
                _reload();
            }
        }
    }
    
    void _undoRedo(UI::RevColumnPtr col, bool undo) {
        Git::Rev rev = col->rev();
        State::History& h = _repoState.history(col->rev().ref);
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
    
    void _cursesInit() noexcept {
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
            _colors = _ColorsCreate(_theme);
        }
        
        _colorsPrev = _ColorsSet(_colors);
        
        // Set _colors as the color pallette used by all Views
        View::Colors(_colors);
        
        _cursorState = UI::CursorState(false, {});
        
    //    // Hide cursor
    //    ::curs_set(0);
        
        ::mousemask(ALL_MOUSE_EVENTS | REPORT_MOUSE_POSITION, NULL);
        ::mouseinterval(0);
        
        ::set_escdelay(0);
    }

    void _cursesDeinit() noexcept {
    //    ::mousemask(0, NULL);
        
        _cursorState.restore();
        
        _ColorsSet(_colorsPrev);
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
        if (!preserveTerminal) _cursesDeinit();
        
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
        
        if (!preserveTerminal) _cursesInit();
    }
    
    static License::Context _LicenseContext(const _Path& repoDir) {
        using namespace std::chrono;
        using namespace std::filesystem;
        License::MachineId machineId = License::MachineIdCalc(DebaseProductId);
        
//        // Find t = max(time(), <time of each file in repoDir>),
//        // to help prevent time-rollback attacks.
//        auto t = std::chrono::system_clock::now();
//        try {
//            for (const _Path& p : Fs::directory_iterator(repoDir)) {
//                try {
//                    auto writeTimeSinceEpoch = std::chrono::time_point_cast<std::chrono::system_clock::duration>(Fs::last_write_time(p));
//                    if (writeTimeSinceEpoch > t) {
//                        t = writeTimeSinceEpoch;
//                    }
//                } catch (...) {} // Continue to next file if an error occurs
//            }
//        } catch (...) {} // Suppress failures while getting latest write time in repo dir
        
        
        
        // Find t = max(time(), <time of each file in repoDir>),
        // to help prevent time-rollback attacks.
        seconds latestTime = duration_cast<seconds>(system_clock::now().time_since_epoch());
        try {
            for (const path& p : directory_iterator(repoDir)) {
                try {
                    seconds writeTimeSinceEpoch = duration_cast<seconds>(last_write_time(p).time_since_epoch());
                    latestTime = std::max(latestTime, writeTimeSinceEpoch);
                } catch (...) {} // Continue to next file if an error occurs
            }
        } catch (...) {} // Suppress failures while getting latest write time in repo dir
        
        const int64_t time = latestTime.count();
        return License::Context{
            .machineId = machineId,
            .version = DebaseVersion,
            .time = time,
        };
    }
    
//    void _trialStart(State::State& state) {
//        
//    }
    
    const License::Context& _licenseCtxGet() {
        if (!_licenseCtx) _licenseCtx = _LicenseContext(_repo.path());
        return *_licenseCtx;
    }
    
    License::Status _licenseUnseal(const License::SealedLicense& sealed, License::License& license) {
        License::Status st = License::Unseal(DebaseKeyPublic, sealed, license);
        if (st != License::Status::Valid) return st;
        return License::Validate(_licenseCtxGet(), license);
    }
    
    void _layoutModalPanel(UI::ModalPanelPtr panel, int width) {
        panel->size(panel->sizeIntrinsic({std::min(width, bounds().size.x), 0}));
        
        UI::Size ps = panel->size();
        UI::Size rs = size();
        UI::Point p = {
            (rs.x-ps.x)/2,
            (rs.y-ps.y)/3,
        };
        panel->origin(p);
//        panel->orderFront();
    }
    
    void _welcomePanelTrial() {
        #warning TODO: show animation while performing network IO
        assert(_welcomePanel);
        
        const License::Context& ctx = _licenseCtxGet();
        const License::Request req = {
            .machineId = ctx.machineId,
        };
        
        _welcomePanel->dropEvents(true);
        
        // Request license
        License::RequestResponse resp;
        Async async([&] () { Network::Request(DebaseLicenseURL, req, resp); for (;;) sleep(1); });
        
        // Animate until we get a response
        UI::ButtonSpinner spinner(_welcomePanel->trialButton());
        std::chrono::steady_clock::time_point nextFrameTime;
        while (!async.done()) {
            if (std::chrono::steady_clock::now() > nextFrameTime) {
                spinner.animate();
                nextFrameTime = std::chrono::steady_clock::now()+std::chrono::milliseconds(100);
            }
            
            track({}, nextFrameTime);
        }
        
        _welcomePanel->dropEvents(false);
        
        
        
        
        
        
        
//        for (;;) {
//            try {
//                int fds[] = { STDIN_FILENO, async.signal() };
//                const bool ready = Toastbox::Select(fds, std::size(fds), nullptr, 0, std::chrono::milliseconds(100));
//                if (ready) {
//                    // Iterate over the fds that are ready for reading
//                    // (Select() modifies its input array)
//                    for (int fd : fds) {
//                        if (fd == STDIN_FILENO) {
//                            os_log(OS_LOG_DEFAULT, "STDIN CAN READ");
//                            
//                            dispatchEvent(nextEvent());
//                        }
//                    }
//                }
//                
//                // Update animation
//                spinner.animate();
//                refresh();
//            
//            } catch (const UI::WindowResize&) {
//                // Continue the loop, which calls _reload()
//                os_log(OS_LOG_DEFAULT, "TERMINAL RESIZED");
//                _reload();
//            }
//        }
        
        
        
        
        
        // Wait on the async or for stdin input
//        while (!async.done()) {
//        for (;;) {
//            try {
//                int fds[] = { STDIN_FILENO, async.signal() };
//                const bool ready = Toastbox::Select(fds, std::size(fds), nullptr, 0, std::chrono::milliseconds(100));
//                if (ready) {
//                    // Iterate over the fds that are ready for reading
//                    // (Select() modifies its input array)
//                    for (int fd : fds) {
//                        if (fd == STDIN_FILENO) {
//                            os_log(OS_LOG_DEFAULT, "STDIN CAN READ");
//                            
//                            dispatchEvent(nextEvent());
//                        }
//                    }
//                }
//                
//                // Update animation
//                spinner.animate();
//                refresh();
//            
//            } catch (const UI::WindowResize&) {
//                // Continue the loop, which calls _reload()
//                os_log(OS_LOG_DEFAULT, "TERMINAL RESIZED");
//                _reload();
//            }
//        }
        
        try {
            async.get();
        } catch (const std::exception& e) {
            _errorMessageShow(std::string("A network error occurred: ") + e.what());
            return;
        }
        
        if (!resp.error.empty()) {
            _errorMessageShow(resp.error);
            return;
        }
        
        // Validate response
        License::SealedLicense sealed = resp.license;
        License::License license;
        License::Status st = _licenseUnseal(sealed, license);
        if (st != License::Status::Valid) {
            _errorMessageShow("The license supplied by the server is invalid.");
            return;
        }
        
        // License is valid; save it and dismiss
        State::State state(StateDir());
        state.license(sealed);
        state.write();
        
        _welcomePanel = nullptr;
    }
    
    void _welcomePanelRegister() {
        _registerPanelShow();
    }
    
    void _welcomePanelShow() {
        _welcomePanel = subviewCreate<UI::WelcomePanel>();
        _welcomePanel->color                    (View::Colors().menu);
        _welcomePanel->title()->text            ("");
        _welcomePanel->message()->text          ("Welcome to debase!");
        _welcomePanel->trialButton()->action    (std::bind(&App::_welcomePanelTrial, this));
        _welcomePanel->registerButton()->action (std::bind(&App::_welcomePanelRegister, this));
    }
    
    void _registerPanelRegister() {
        assert(_registerPanel);
        
        const License::Context& ctx = _licenseCtxGet();
        const std::string email = _registerPanel->email()->value();
        const std::string registerCode = _registerPanel->code()->value();
        const License::UserId userId = License::UserIdForEmail(DebaseProductId, email);
        
        const License::Request req = {
            .machineId = ctx.machineId,
            .userId = userId,
            .registerCode = registerCode,
        };
        
        License::RequestResponse resp;
        try {
            Network::Request(DebaseLicenseURL, req, resp);
        } catch (const std::exception& e) {
            _errorMessageShow(std::string("A network error occurred: ") + e.what());
            return;
        }
        
        if (!resp.error.empty()) {
            _errorMessageShow(resp.error);
            return;
        }
        
        // Validate response
        License::SealedLicense sealed = resp.license;
        License::License license;
        License::Status st = _licenseUnseal(sealed, license);
        if (st != License::Status::Valid) {
            _errorMessageShow("The license supplied by the server is invalid.");
            return;
        }
        
        // License is valid; save it and dismiss
        State::State state(StateDir());
        state.license(sealed);
        state.write();
        
        _infoMessageShow("Thank you for supporting debase!");
        _registerPanel = nullptr;
    }
    
    void _registerPanelDismiss() {
        assert(_registerPanel);
        _registerPanel = nullptr;
    }
    
    void _registerPanelShow(std::string_view title, std::string_view message, bool dismissAllowed) {
        _registerPanel = subviewCreate<UI::RegisterPanel>();
        _registerPanel->color               (View::Colors().menu);
        _registerPanel->title()->text       (title);
        _registerPanel->message()->text     (message);
        _registerPanel->okButton()->action  (std::bind(&App::_registerPanelRegister, this));
        if (dismissAllowed) {
            _registerPanel->dismissAction   (std::bind(&App::_registerPanelDismiss, this));
        }
    }
    
    void _infoMessageShow(std::string_view msg) {
        _messagePanel = subviewCreate<UI::ModalPanel>();
        _messagePanel->color            (_colors.menu);
//        _messagePanel->title()->text    ("Error");
        _messagePanel->message()->text  (msg);
        _messagePanel->dismissAction    ([=] (UI::ModalPanel&) { _messagePanel = nullptr; });
        
        // Sleep 10ms to prevent an odd flicker that occurs when showing a panel
        // as a result of pressing a keyboard key. For some reason showing panels
        // due to a mouse event doesn't trigger the flicker, only keyboard events.
        // For some reason sleeping a short time fixes it.
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    
    void _errorMessageShow(std::string_view msg) {
        _messagePanel = subviewCreate<UI::ModalPanel>();
        _messagePanel->color            (_colors.error);
        _messagePanel->title()->text    ("Error");
        _messagePanel->message()->text  (msg);
        _messagePanel->dismissAction    ([=] (UI::ModalPanel&) { _messagePanel = nullptr; });
//        _messagePanel->orderFront();
        
        // Sleep 10ms to prevent an odd flicker that occurs when showing a panel
        // as a result of pressing a keyboard key. For some reason showing panels
        // due to a mouse event doesn't trigger the flicker, only keyboard events.
        // For some reason sleeping a short time fixes it.
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    
    void _trialExpiredPanelShow() {
        constexpr const char* Title     = "Trial Expired";
        constexpr const char* Message   = "Thank you for trying debase. Please register to continue.";
        _registerPanelShow(Title, Message, false);
    }
    
    void _registerPanelShow() {
        constexpr const char* Title     = "Register debase";
        constexpr const char* Message   = "Please enter your registration information.";
        _registerPanelShow(Title, Message, true);
    }
    
    void _licenseCheck(bool networkAllowed=true) {
        State::State state(StateDir());
        
        License::License license;
        License::Status st = _licenseUnseal(state.license(), license);
        if (st == License::Status::Empty) {
            if (!state.trialExpired()) {
                _welcomePanelShow();
            
            } else {
                // Show trial-expired panel
                _trialExpiredPanelShow();
            }
        
        } else if (st==License::Status::InvalidMachineId && !license.expiration) {
            #warning TODO: renew license for this machine
        
        } else if (st == License::Status::Expired) {
            // Delete license, set expired=1, show trial-expired panel
            state.license({});
            state.trialExpired(true);
            state.write();
            
            _trialExpiredPanelShow();
        
        } else if (st == License::Status::Valid) {
            // Done; debase is registered
        
        } else {
            // Delete license, show welcome panel
            state.license({});
            state.write();
            
            _welcomePanelShow();
        }
    }
    
    
    
    
//    License::Status _licenseCheck(const License::SealedLicense& sealed, bool networkAllowed=true) {
//        License::License license;
//        License::Status st = _LicenseUnseal(_repo.path(), sealed, license);
//        
//        // If the license is valid except for the machine id, and it's not a trial license,
//        // request a new license from the server with our machine id
//        if (st==License::Status::InvalidMachineId && !license.expiration) {
//            if (!networkAllowed) return st;
//            
//            const License::Request req = {
//                .machineId = License::MachineIdCalc(DebaseProductId),
//                .userId = license.userId,
//                .registerCode = license.registerCode,
//            };
//            
//            License::RequestResponse resp;
//            try {
//                Network::Request(DebaseLicenseURL, req, resp);
//            } catch (const std::exception& e) {
//                #warning TODO: license is authentic but isn't valid for this machine, but we couldn't reach the server to renew it
//            }
//            
//            if (!resp.error.empty()) {
//                #warning TODO: license is authentic but isn't valid for this machine, and the server gave us an error when we tried to renew it
//                return;
//            }
//            
//            return _licenseCheck(resp.license, false);
//        }
//        
//        if (st != License::Status::Valid) {
//            switch (st) {
//            // License expired:
//            // Show license-expired dialog
//            case License::Status::Expired:
//                #warning TODO: Show license-expired dialog
//                break;
//            
//            // Generic license-invalid handling:
//            // Assume trial mode
//            default:
//                const License::Request req = {
//                    .machineId = License::MachineIdCalc(DebaseProductId),
//                };
//                
//                License::RequestResponse resp;
//                Network::Request(DebaseLicenseURL, req, resp);
//                if (!resp.error.empty()) {
//                    #warning TODO: show error panel with `resp.error`
//                    break;
//                }
//                
//                // Validate response
//                _LicenseUnseal(_repo.path(), resp.license, license);
//                
//                License::Context ctx = _LicenseContext(_repo.path());
//                st = License::Validate(license, ctx);
//                
//                break;
//            }
//        }
//        
//        // If the license is expired, show the expired dialog
//        if (st == License::Status::Expired) {
//            
//        } else {
//            
//        }
//        
//        // The license is invalid
//        // Assume trial mode
//        if (st != License::Status::Valid) {
//            
//        }
//        
//        if (st == License::Status::Valid) {
//            
//        
//        
//        
//        } else {
//            // Invalid license
//            // Assume trial mode
//            
//        }
//        
////        if (st==License::Status::InvalidMachineId && )
////        
////        switch (st) {
////        case InvalidMachineId:
////            // The license is valid except for the machine id
////            // If it's not a trial license, ask the server for a new license for the current machine id
////            
////            break;
////        default:
////        }
////        if (st == )
//    }

    
//    License::Status _licenseCheck(const License::SealedLicense& sealed, bool networkAllowed=true) {
//        State::State state(StateDir());
//        
//        License::License license;
//        License::Status st = _SealedLicenseValidate(_repo.path(), state.license(), license);
//        
//        // If the license is valid except for the machine id, and it's not a trial license,
//        // request a new license from the server with our machine id
//        if (st==License::Status::InvalidMachineId && !license.expiration) {
//            
//        } else if (st != License::Status::Valid) {
//            switch (st) {
//            // License expired:
//            // Show license-expired dialog
//            case License::Status::Expired:
//                #warning TODO: Show license-expired dialog
//                break;
//            
//            // Generic license-invalid handling:
//            // Assume trial mode
//            default:
//                const License::Request req = {
//                    .machineId = License::MachineIdCalc(DebaseProductId),
//                };
//                
//                License::RequestResponse resp;
//                Network::Request(DebaseLicenseURL, req, resp);
//                if (!resp.error.empty()) {
//                    #warning TODO: show error panel with `resp.error`
//                    break;
//                }
//                
//                // Validate response
//                _SealedLicenseValidate(_repo.path(), resp.license, license);
//                
//                License::Context ctx = _LicenseContext(_repo.path());
//                st = License::Validate(license, ctx);
//                
//                break;
//            }
//        }
//        
//        // If the license is expired, show the expired dialog
//        if (st == License::Status::Expired) {
//            
//        } else {
//            
//        }
//        
//        // The license is invalid
//        // Assume trial mode
//        if (st != License::Status::Valid) {
//            
//        }
//        
//        if (st == License::Status::Valid) {
//            
//        
//        
//        
//        } else {
//            // Invalid license
//            // Assume trial mode
//            
//        }
//        
////        if (st==License::Status::InvalidMachineId && )
////        
////        switch (st) {
////        case InvalidMachineId:
////            // The license is valid except for the machine id
////            // If it's not a trial license, ask the server for a new license for the current machine id
////            
////            break;
////        default:
////        }
////        if (st == )
//    }
    
    Git::Repo _repo;
    std::vector<Git::Rev> _revs;
    
    UI::ColorPalette _colors;
    UI::ColorPalette _colorsPrev;
    std::optional<License::Context> _licenseCtx;
    State::RepoState _repoState;
    Git::Rev _head;
    std::vector<UI::RevColumnPtr> _columns;
    UI::CursorState _cursorState;
    State::Theme _theme = State::Theme::None;
    
    struct {
        UI::CommitPanelPtr titlePanel;
        std::vector<UI::PanelPtr> shadowPanels;
        std::optional<UI::Rect> insertionMarker;
        bool copy = false;
    } _drag;
    
    struct {
        Git::Rev rev;
        Git::Commit commit;
        std::chrono::steady_clock::time_point mouseUpTime;
    } _doubleClickState;
    
    _Selection _selection;
    std::optional<UI::Rect> _selectionRect;
    
    UI::WelcomePanelPtr _welcomePanel;
    UI::RegisterPanelPtr _registerPanel;
    UI::ModalPanelPtr _messagePanel;
};
