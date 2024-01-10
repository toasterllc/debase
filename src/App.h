#pragma once
#include <thread>
#include <sys/types.h>
#include <sys/wait.h>
#include "Debase.h"
#include "git/Git.h"
#include "ui/Window.h"
#include "ui/Screen.h"
#include "ui/Color.h"
#include "ui/RevColumn.h"
#include "ui/SnapshotButton.h"
#include "ui/Menu.h"
#include "ui/SnapshotMenu.h"
#include "ui/Alert.h"
#include "ui/ButtonSpinner.h"
#include "ui/ErrorAlert.h"
#include "ui/ConflictPanel.h"
//#include "ui/TabPanel.h"
#include "state/StateDir.h"
#include "state/Theme.h"
#include "state/State.h"
#include "git/Conflict.h"
#include "lib/toastbox/String.h"
#include "xterm-256color.h"
#include "Terminal.h"
#include "Async.h"
#include "CurrentExecutablePath.h"
#include "PathIsInEnvironmentPath.h"
#include "ProcessPath.h"
#include "UserBinRelativePath.h"
#include "Syscall.h"
#include "Rev.h"
#include "ConflictText.h"
#include "OpenURL.h"

extern "C" {
    extern char** environ;
};

class App : public UI::Screen {
public:
    App(Git::Repo repo, const std::vector<Rev>& revs) :
    Screen(Uninit), _repo(repo), _revs(revs) {}
    
    // focusable(): override to affect cursor state
    bool focusable() const override { return true; }
    
    using UI::Screen::layout;
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
        
        for (UI::PanelPtr panel : _panels) {
            _layoutPanel(panel);
        }
    }
    
    void draw() override {
        const UI::Color selectionColor = (_drag.copy ? colors().selectionCopy : colors().selection);
        
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
                    borderColor = (dragging ? colors().selectionSimilar : colors().selection);
                
                } else {
                    borderColor = (!dragging && selectState==_SelectState::Similar ? colors().selectionSimilar : colors().normal);
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
            }
        }
        
        if (_selectionRect) {
            Attr color = attr(colors().selection);
            drawRect(*_selectionRect);
        }
    }
    
    bool handleEvent(const UI::Event& ev) override {
        switch (ev.type) {
        case UI::Event::Type::Mouse: {
            const _HitTestResult hitTest = _hitTest(ev.mouse.origin);
            if (ev.mouseDown(UI::Event::MouseButtons::Left)) {
                const bool shift = (ev.mouse.bstate & _SelectionShiftKeys);
                if (hitTest && !shift) {
                    if (hitTest.panel) {
                        // Mouse down inside of a CommitPanel, without shift key
                        std::optional<_GitOp> gitOp = _trackMouseInsideCommitPanel(ev, hitTest.column, hitTest.panel);
                        if (gitOp) _gitOpExec(*gitOp);
                    }
                
                } else {
                    // Mouse down outside of a CommitPanel, or mouse down anywhere with shift key
                    _trackSelectionRect(ev);
                }
            
            } else if (ev.mouseDown(UI::Event::MouseButtons::Right)) {
                if (hitTest) {
                    if (hitTest.nameField) {
                        _trackContextMenuRevNameField(ev, hitTest.column);
                    
                    } else if (hitTest.panel) {
                        std::optional<_GitOp> gitOp = _trackContextMenuCommitPanel(ev, hitTest.column, hitTest.panel);
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
            
            _GitOp gitOp = {
                .type = _GitOp::Type::Delete,
                .src = {
                    .rev = _selection.rev,
                    .commits = _selection.commits,
                },
            };
            _gitOpExec(gitOp);
            break;
        }
        
        case UI::Event::Type::KeyB: {
            if (!_selectionCanBranch()) {
                beep();
                break;
            }
            
            assert(_selection.commits.size() == 1);
            const Rev& rev = _selection.rev;
            const Git::Commit& commit = *_selection.commits.begin();
            _revCreate(_RevCreateType::Branch, rev, commit);
            break;
        }
        
        case UI::Event::Type::KeyC: {
            if (!_selectionCanCombine()) {
                beep();
                break;
            }
            
            _GitOp gitOp = {
                .type = _GitOp::Type::Combine,
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
            
            _GitOp gitOp = {
                .type = _GitOp::Type::Edit,
                .src = {
                    .rev = _selection.rev,
                    .commits = _selection.commits,
                },
            };
            _gitOpExec(gitOp);
            break;
        }
        
        default: break;
        }
        
        return true;
    }
    
    void _runNoRepo() {
        try {
            _cursesInit();
            Defer(_cursesDeinit());
            
            // Create our window now that ncurses is initialized
            Window::operator =(Window(::stdscr));
            
            _reload();
            _moveOffer();
            _errorMessageRun("debase must be run from a git repository.");
            
        } catch (const UI::ExitRequest&) {
            // Nothing to do
        } catch (...) {
            throw;
        }
    }
    
    void run() {
        _theme = State::ThemeRead();
        
        // Handle being run outside of a git repo
        // We show a dialog in this case, and still offer to move debase to ~/bin, so that
        // the first-run experience is decent even when run outside of a git repo, which
        // is likely on the first invocation
        if (!_repo) {
            _runNoRepo();
            return;
        }
        
        _head = _repo.headResolved();
        
        // Create _repoState
        std::set<Git::Ref> refs;
        for (const Rev& rev : _revs) {
            if (rev.ref) refs.insert(rev.ref);
        }
        _repoState = State::RepoState(StateDir(), _repo, refs);
        
        // If the repo has outstanding changes, prevent the currently checked-out
        // branch from being modified, since we can't clobber the uncommitted
        // changes. We do this by marking all refs that match HEAD's ref as
        // immutable.
        if (_repo.dirty() && _head.ref) {
            for (Rev& rev : _revs) {
                if (rev.ref && rev.ref==_head.ref) {
                    rev.mutability = Rev::Mutability::DisallowedUncommittedChanges;
                }
            }
        }
        
        // Reattach head upon return
        // We do this via Defer() so that it executes even if there's an exception
        Defer(
            // We have a separate check for `_head`, which is necessary because we allow branches to be deleted,
            // so if the branch that _head is attached to is deleted, _head is empty (ie _head==Rev()), in which
            // case we don't want to try to reattach HEAD to it.
            if (_headReattach && _head) {
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
            
            _reload();
            _moveOffer();
            track();
            
        } catch (const UI::ExitRequest&) {
            // Nothing to do
        } catch (...) {
            throw;
        }
        
        _repoState.write();
    }
    
    void track(Deadline deadline=Forever) override {
        for (;;) {
            std::string errorMsg;
            
            try {
                Screen::track(deadline);
                break; // Deadline passed; break out of loop
            
            } catch (const UI::WindowResize&) {
                _revColumnNameSetFocused(nullptr, false, false);
                _reload();
            
            // Bubble up
            } catch (const UI::ExitRequest&) {
                throw;
            
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
                if (errorMsg.back() != '.') errorMsg += '.';
                _errorMessageRun(errorMsg);
            }
        }
    }
    
private:
    using _Path = std::filesystem::path;
    using _GitModify = Git::Modify<Rev>;
    using _GitOp = _GitModify::Op;
    
    struct _Selection {
        Rev rev;
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
        UI::TextFieldPtr nameField;
        UI::CommitPanelPtr panel;
        operator bool() const {
            return (bool)column;
        }
    };
    
    // _PanelPresenter(): RAII class to handle pushing/popping an panel from our _panels deque
    template <typename T>
    class _PanelPresenter {
    public:
        _PanelPresenter(std::deque<UI::PanelPtr>& panels, T panel) : _s{.panels=&panels, .panel=panel} {
            assert(panel);
            _s.panels->push_back(panel);
        }
        
        ~_PanelPresenter() {
            if (_s.panel) {
                assert(!_s.panels->empty());
                assert(_s.panels->back() == _s.panel);
                _s.panels->pop_back();
            }
        }
        
        _PanelPresenter(const _PanelPresenter& x) = delete;
        
        _PanelPresenter(_PanelPresenter&& x) {
            swap(x);
        }
        
        void swap(_PanelPresenter& x) {
            std::swap(_s, x._s);
        }
        
        operator T&() { return _s.panel; }
        T operator ->() { return _s.panel; }
        
        T& panel() { return _s.panel; }
    
    private:
        struct {
            std::deque<UI::PanelPtr>* panels = nullptr;
            T panel;
        } _s;
    };
    
    template <typename T, typename ...T_Args>
    _PanelPresenter<std::shared_ptr<T>> _panelPresent(T_Args&&... args) {
        std::shared_ptr<T> panel = subviewCreate<T>(std::forward<T_Args>(args)...);
        return _PanelPresenter(_panels, panel);
    }
    
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
    
    static UI::ColorPalette _ColorsCreate(State::Theme theme) {
        const char* termProgEnv = getenv("TERM_PROGRAM");
        const std::string termProg = termProgEnv ? termProgEnv : "";
        const bool themeDark = (theme==State::Theme::None || theme == State::Theme::Dark);
        
        UI::ColorPalette colors;
        if (termProg == "Apple_Terminal") {
            // Colorspace: unknown
            // There's no simple relation between these numbers and the resulting colors because the macOS
            // Terminal.app applies some kind of filtering on top of these numbers. These values were
            // manually chosen based on their appearance.
            if (themeDark) {
                colors.normal           = UI::Color();
                colors.dimmed           = colors.pairNew(colors.colorNew( 77,  77,  77));
                colors.selection        = colors.pairNew(colors.colorNew(  0,   2, 255));
                colors.selectionSimilar = colors.pairNew(colors.colorNew(140, 140, 255));
                colors.selectionCopy    = colors.pairNew(colors.colorNew(  0, 229, 130));
                colors.menu             = colors.selectionCopy;
                colors.error            = colors.pairNew(colors.colorNew(194,   0,  71));
                
                colors.conflictTextMain = colors.pairNew(colors.colorNew(255, 255, 255), colors.colorNew( 40,  40,  40));
                colors.conflictTextDim  = colors.pairNew(colors.colorNew( 35,  35,  35));
            
            } else {
                colors.normal           = UI::Color();
                colors.dimmed           = colors.pairNew(colors.colorNew(128, 128, 128));
                colors.selection        = colors.pairNew(colors.colorNew(  0,   2, 255));
                colors.selectionSimilar = colors.pairNew(colors.colorNew(140, 140, 255));
                colors.selectionCopy    = colors.pairNew(colors.colorNew( 52, 167,   0));
                colors.menu             = colors.pairNew(colors.colorNew(194,   0,  71));
                colors.error            = colors.menu;
                
                colors.conflictTextMain = colors.pairNew(colors.colorNew(0, 0, 0), colors.colorNew( 220,  220,  220));
                colors.conflictTextDim  = colors.pairNew(colors.colorNew( 120,  120,  120));
            }
        
        } else {
            // Colorspace: sRGB
            // These colors were derived by sampling the Apple_Terminal values when they're displayed on-screen
            
            if (themeDark) {
                colors.normal           = UI::Color();
                colors.dimmed           = colors.pairNew(colors.colorNew(.486*255, .486*255, .486*255));
                colors.selection        = colors.pairNew(colors.colorNew(.463*255, .275*255, 1.00*255));
                colors.selectionSimilar = colors.pairNew(colors.colorNew(.663*255, .663*255, 1.00*255));
                colors.selectionCopy    = colors.pairNew(colors.colorNew(.204*255, .965*255, .569*255));
                colors.menu             = colors.selectionCopy;
                colors.error            = colors.pairNew(colors.colorNew(.969*255, .298*255, .435*255));
                
                // conflictTextMain/conflictTextDim: manually chosen to look good in iTerm
                // (which has a black background instead of dark gray)
                colors.conflictTextMain = colors.pairNew(colors.colorNew(255, 255, 255), colors.colorNew( 20,  20,  20));
                colors.conflictTextDim  = colors.pairNew(colors.colorNew( 70,  70,  70));
            
            } else {
                colors.normal           = UI::Color();
                colors.dimmed           = colors.pairNew(colors.colorNew(.592*255, .592*255, .592*255));
                colors.selection        = colors.pairNew(colors.colorNew(.369*255, .208*255, 1.00*255));
                colors.selectionSimilar = colors.pairNew(colors.colorNew(.627*255, .627*255, 1.00*255));
                colors.selectionCopy    = colors.pairNew(colors.colorNew(.306*255, .737*255, .153*255));
                colors.menu             = colors.pairNew(colors.colorNew(.969*255, .298*255, .435*255));
                colors.error            = colors.menu;
                
                // Same as Apple_Terminal version because when specifying a background color,
                // Apple_Terminal doesn't apply filtering, so we get the actual value that we
                // specify (unlike when we don't specify a background color, which causes
                // filtering to be applied on the colors we specify)
                colors.conflictTextMain = colors.pairNew(colors.colorNew(0, 0, 0), colors.colorNew( 220,  220,  220));
                colors.conflictTextDim  = colors.pairNew(colors.colorNew( 120,  120,  120));
            }
        }
        
        return colors;
    }
    
    static UI::ColorPalette _ColorsCreateDefault() {
        UI::ColorPalette colors;
        colors.normal           = UI::Color();
        colors.dimmed           = UI::Color();
        colors.selection        = colors.pairNew(COLOR_BLUE);
        colors.selectionSimilar = UI::Color();
        colors.selectionCopy    = colors.pairNew(COLOR_GREEN);
        colors.menu             = colors.pairNew(COLOR_RED);
        colors.error            = colors.pairNew(COLOR_RED);
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
            const UI::Point cp = View::SubviewConvert(*col, p);
            UI::TextFieldPtr nameField = col->hitTestNameField(cp);
            if (nameField) {
                return {
                    .column = col,
                    .nameField = nameField,
                };
            }
            
            UI::CommitPanelPtr panel = col->hitTestCommit(cp);
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
    
    UI::RevColumnPtr _columnForRev(const Rev& rev) {
        for (UI::RevColumnPtr col : _columns) {
            if (col->rev() == rev) return col;
        }
        return nullptr;
//        // Programmer error if it doesn't exist
//        abort();
    }
    
    UI::CommitPanelPtr _panelForCommit(UI::RevColumnPtr col, Git::Commit commit) {
        for (UI::CommitPanelPtr panel : col->panels()) {
            if (panel->commit() == commit) return panel;
        }
        // Programmer error if it doesn't exist
        abort();
    }
    
    UI::ButtonPtr _makeSnapshotMenuButton(const Git::Ref& ref, const State::Snapshot& snap,
        bool sessionStart, UI::SnapshotButton*& chosen) {
        
        const State::History& h = _repoState.history(ref);
        const bool activeSnapshot = h.get().refState == snap.refState;
        const UI::SnapshotButtonPtr b = std::make_shared<UI::SnapshotButton>(_repo, snap, _SnapshotMenuWidth);
        b->activeSnapshot(activeSnapshot);
        b->action([&] (UI::Button& button) { chosen = (UI::SnapshotButton*)&button; });
        return b;
    }
    
    UI::ButtonPtr _makeContextMenuButton(std::string_view label, std::string_view key, bool enabled, UI::Button*& chosen) {
        constexpr int ContextMenuWidth = 12;
        UI::ButtonPtr b = std::make_shared<UI::Button>();
        b->label()->text(label);
        b->label()->align(UI::Align::Left);
        b->key()->text(key);
        b->action([&] (UI::Button& button) { chosen = &button; });
        b->enabled(enabled);
        b->size({ContextMenuWidth, 1});
        return b;
    }
    
    bool _selectionCanBranch() {
        return _selection.commits.size() == 1;
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
    
    void _revColumnNameSetFocused(UI::RevColumnPtr fcol, bool commit, bool seekCursor) {
        // If the given column isn't mutable, then pretend as if we were given nullptr
        if (fcol && !fcol->rev().isMutable()) {
            fcol = nullptr;
        }
        
        // Short-circuit if the column's name is already focused
        if (_columnNameFocused == fcol) return;
        
        // Commit existing name change
        if (commit) {
            if (_columnNameFocused) {
                const std::string name = _columnNameFocused->nameField()->value();
                const std::string namePrev = _columnNameFocused->name(true);
                Rev rev = _columnNameFocused->rev();
                if (name != namePrev) {
                    // Name changed
                    try {
                        rev.ref = _gitRefRename(rev.ref, name);
                        
                        State::History& h = _repoState.history(rev.ref);
                        h.push(State::HistoryRefState(rev.ref));
                        // We made a modification -- push the initial snapshot
                        _repoState.snapshotInitialPush(rev.ref);
                        
                        _reload();
                    
                    } catch (const Git::Error& e) {
                        std::string errorMsg;
                        switch (e.error) {
                        case GIT_EINVALIDSPEC:
                            errorMsg = "The ref name '" + name + "' contains invalid characters.";
                            break;
                        case GIT_EEXISTS:
                            errorMsg = "A ref named '" + name + "' already exists.";
                            break;
                        default:
                            throw;
                        }
                        
                        std::optional<bool> clicked;
                        auto alert = _panelPresent<UI::ErrorAlert>();
                        alert->width                            (45);
                        alert->message()->text                  (errorMsg);
                        alert->okButton()->label()->text        ("Edit");
                        alert->dismissButton()->label()->text   ("Revert");
                        alert->okButton()->action               ( [&] (UI::Button&) { clicked = true; } );
                        alert->dismissButton()->action          ( [&] (UI::Button&) { clicked = false; } );
                        
                        // Wait until the user clicks a button
                        while (!clicked) track(Once);
                        
                        // If the user chose 'Edit', then act as if this function were
                        // never called by simply returning.
                        if (*clicked) return;
                        
                        // The user chose 'Cancel': we don't rename the ref,
                        // but we continue setting the focus.
                    }
                }
            }
        }
        
        if (_columnNameFocused) {
            _columnNameFocused->nameField()->value(_columnNameFocused->name(false));
            _columnNameFocused->nameField()->focused(false);
        }
        
        _columnNameFocused = fcol;
        
        if (_columnNameFocused) {
            _columnNameFocused->nameField()->value(_columnNameFocused->name(true));
            if (seekCursor) _columnNameFocused->nameField()->seek(UI::Align::Right);
            _columnNameFocused->nameField()->focused(true);
        }
    }
    
    void _revColumnNameFocus(UI::RevColumnPtr col) {
        _revColumnNameSetFocused(col, true, false);
    }
    
    void _revColumnNameUnfocus(UI::RevColumnPtr col, UI::TextField::UnfocusReason reason) {
        switch (reason) {
        case UI::TextField::UnfocusReason::Tab:
            // Ignore tabs
            break;
        case UI::TextField::UnfocusReason::Return:
        case UI::TextField::UnfocusReason::Click:
            _revColumnNameSetFocused(nullptr, true, false);
            break;
        case UI::TextField::UnfocusReason::Escape:
            _revColumnNameSetFocused(nullptr, false, false);
            break;
        default:
            abort();
        }
    }
//    
//    void _revColumnNameCommit(UI::RevColumnPtr col) {
//        switch (reason) {
//        case UnfocusReason::Return:
//            
//            return;
//        
//        case UnfocusReason::Escape:
//        default:
//            // Cancel editing
//            return;
//        }
//    }
    
    void _reload() {
        // We allow ourself to be called outside of a git repo, so we need
        // to check _repo for null
        if (_repo) {
            // Reload head's ref
            _head = _repo.revReload(_head);
        }
        
        // Reload all ref-based revs
        for (Rev& rev : _revs) {
            (Git::Rev&)rev = _repo.revReload(rev);
        }
        
        // Create columns
        std::list<UI::ViewPtr> sv;
        int offX = _ColumnInsetX;
        size_t colCount = 0;
        for (const Rev& rev : _revs) {
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
                
                col->nameField()->valueChangedAction ([=] (UI::TextField& field) {  });
                col->nameField()->focusAction ([=] (UI::TextField& field) {
                    auto col = weakCol.lock();
                    if (col) _revColumnNameFocus(col);
                });
                
                col->nameField()->unfocusAction ([=] (UI::TextField& field, UI::TextField::UnfocusReason reason) {
                    auto col = weakCol.lock();
                    if (col) _revColumnNameUnfocus(col, reason);
                });
            
            } else {
                col = _columns[colCount];
            }
            
            col->rev(rev); // Ensure all columns' revs are up to date (since refs become stale if they're modified)
            col->head(rev.displayHead() == _head.commit);
            col->undoButton()->enabled(h && !h->begin());
            col->redoButton()->enabled(h && !h->end());
            col->reload({_ColumnWidth, size().y});
            sv.push_back(col);
            
            offX += _ColumnWidth+_ColumnSpacing;
            colCount++;
        }
        
        // Erase columns that aren't visible
        _columns.erase(_columns.begin()+colCount, _columns.end());
        
        // Update subviews
        for (UI::PanelPtr panel : _panels) {
            sv.push_back(panel);
        }
        
        subviews(sv);
        
        layoutNeeded(true);
        eraseNeeded(true);
    }
    
    // _trackMouseInsideCommitPanel
    // Handles clicking/dragging a set of CommitPanels
    std::optional<_GitOp> _trackMouseInsideCommitPanel(const UI::Event& mouseDownEvent, UI::RevColumnPtr mouseDownColumn, UI::CommitPanelPtr mouseDownPanel) {
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
        if (!selectionColumn) abort();
        
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
            const bool allow = HitTest(rootWinBounds, p, UI::Size{3,3});
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
                    const UI::Size size = _drag.titlePanel->sizeIntrinsic({titlePanel->size().x, ConstraintNone});
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
            ev = eventNext();
            abort = (ev.type != UI::Event::Type::Mouse);
            // Check if we should abort
            if (abort || ev.mouseUp()) {
                break;
            }
        }
        
        std::optional<_GitOp> gitOp;
        if (!abort) {
            if (_drag.titlePanel && ipos) {
                Git::Commit dstCommit = ((ipos->iter != ipos->col->panels().end()) ? (*ipos->iter)->commit() : nullptr);
                gitOp = _GitOp{
                    .type = (_drag.copy ? _GitOp::Type::Copy : _GitOp::Type::Move),
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
                const Rev& rev = _selection.rev;
                const Git::Commit& commit = *_selection.commits.begin();
                const bool doubleClicked =
                    doubleClickStatePrev.rev                                            &&
                    doubleClickStatePrev.rev==rev                                       &&
                    doubleClickStatePrev.commit==commit                                 &&
                    doubleClickStatePrev.mouseUpOrigin-ev.mouse.origin == UI::Point{}   &&
                    currentTime-doubleClickStatePrev.mouseUpTime < _DoubleClickThresh   ;
                const bool validTarget = _selection.rev.isMutable();
                
                if (doubleClicked) {
                    if (validTarget) {
                        gitOp = {
                            .type = _GitOp::Type::Edit,
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
                    .mouseUpOrigin = ev.mouse.origin,
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
            ev = eventNext();
            // Check if we should abort
            if (ev.type!=UI::Event::Type::Mouse || ev.mouseUp()) {
                break;
            }
        }
        
        // Reset state
        eraseNeeded(true); // We need one more erase to erase the selection rect upon mouse-up
        _selectionRect = std::nullopt;
    }
    
    enum class _RevCreateType {
        MatchTemplate,
        Branch,
    };
    
    // _revCreate(): copies a given rev:
    //   branch          -> branch
    //   tag             -> tag
    //   detached commit -> branch
    void _revCreate(_RevCreateType type, const Rev& revTemplate, const Git::Commit& commit) {
        std::string name;
        bool suffixRequired = true;
        if (revTemplate.ref) {
            if (revTemplate.ref.isBranch()) {
                name = _repo.branchNameLocal(Git::Branch::ForRef(revTemplate.ref));
            } else {
                name = revTemplate.ref.name();
            }
        } else {
            name = "NewBranch";
            suffixRequired = false;
        }
        
        assert(!name.empty());
        
        // Keep appending a new suffix until we find an unused ref name
        Git::Ref ref;
        for (size_t i=0; i<20; i++) {
            // If a suffix is required and this iteration is the non-suffix
            // iteration (i=0), then skip this iteration.
            if (!i && suffixRequired) continue;
            
            std::string fullName = name;
            // When i=0, don't append a suffix
            // This is to handle the case where !revTemplate.ref, so we use the default name (NewBranch)
            if (i) {
                const std::string suffix = std::to_string(i+1);
                if (isdigit(name.back())) {
                    fullName += "-" + suffix;
                } else {
                    fullName += suffix;
                }
            }
            
            try {
                switch (type) {
                case _RevCreateType::MatchTemplate:
                    if (revTemplate.ref) {
                        ref = _repo.refCopy(revTemplate.ref, fullName, commit);
                    } else {
                        ref = _repo.branchCreate(fullName, commit);
                    }
                    break;
                case _RevCreateType::Branch:
                    ref = _repo.branchCreate(fullName, commit);
                    break;
                }
                
            } catch (const Git::Error& e) {
                if (e.error == GIT_EEXISTS) continue;
                throw;
            }
            break;
        }
        
        if (!ref) throw Toastbox::RuntimeError("failed to find an unused ref name");
        
        // Remember the new ref in the reflog, so it's visible in subsequent debase launches
        _repo.reflogRememberRef(ref);
        
        const Rev rev(ref);
        auto it = std::find(_revs.begin(), _revs.end(), revTemplate);
        assert(it != _revs.end());
        _revs.insert(it+1, rev);
        
        // Set our selection to the first commit of the new ref
        _selection = {
            .rev = rev,
            .commits = {commit},
        };
        
        _reload();
        
        const UI::RevColumnPtr col = _columnForRev(rev);
        if (!col) return; // It's possible that col==null because the new column is offscreen!
        _revColumnNameSetFocused(col, false, true);
    }
    
    bool _columnCanRename(UI::RevColumnPtr col) {
        return col->rev().isMutable();
    }
    
    bool _columnCanDuplicate(UI::RevColumnPtr col) {
        return true;
    }
    
    bool _columnCanDelete(UI::RevColumnPtr col) {
        return col->rev().isMutable();
    }
    
    void _trackContextMenuRevNameField(const UI::Event& mouseDownEvent, UI::RevColumnPtr mouseDownColumn) {
        UI::Button* menuButton = nullptr;
        UI::ButtonPtr renameButton      = _makeContextMenuButton("Rename",      "", _columnCanRename(mouseDownColumn),      menuButton);
        UI::ButtonPtr duplicateButton   = _makeContextMenuButton("Duplicate",   "", _columnCanDuplicate(mouseDownColumn),   menuButton);
        UI::ButtonPtr deleteButton      = _makeContextMenuButton("Delete…",     "", _columnCanDelete(mouseDownColumn),      menuButton);
        
        UI::MenuPtr menu = subviewCreate<UI::Menu>();
        menu->buttons({ renameButton, duplicateButton, deleteButton });
        menu->sizeToFit(ConstraintNoneSize);
        menu->origin(mouseDownEvent.mouse.origin);
        menu->track();
        
        if (menuButton == renameButton.get()) {
            menu = nullptr; // Hide menu before renaming begins
            _revColumnNameSetFocused(mouseDownColumn, true, true);
        
        } else if (menuButton == duplicateButton.get()) {
            menu = nullptr; // Hide menu before renaming begins
            const Rev& rev = mouseDownColumn->rev();
            const Git::Commit& commit = rev.commit;
            _revCreate(_RevCreateType::MatchTemplate, rev, commit);
        
        } else if (menuButton == deleteButton.get()) {
            std::optional<bool> clicked;
            auto alert = _panelPresent<UI::ErrorAlert>();
            alert->width                        (50);
            alert->title()->text                ("Confirm Delete");
            alert->message()->text              ("Are you sure you want to delete this ref?\n\nDeleting refs cannot be undone.");
            alert->okButton()->label()->text    ("Delete");
            alert->okButton()->action           ( [&] (UI::Button&) { clicked = true; } );
            alert->dismissButton()->action      ( [&] (UI::Button&) { clicked = false; } );
            
            // Wait until the user clicks a button
            while (!clicked) track(Once);
            
            if (*clicked) {
                // Delete the ref
                _gitRefDelete(mouseDownColumn->rev().ref);
                _reload();
            }
        }
    }
    
    std::optional<_GitOp> _trackContextMenuCommitPanel(const UI::Event& mouseDownEvent, UI::RevColumnPtr mouseDownColumn, UI::CommitPanelPtr mouseDownPanel) {
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
        UI::ButtonPtr createBranchButton    = _makeContextMenuButton("Branch",  "b",    _selectionCanBranch(),  menuButton);
        UI::ButtonPtr combineButton         = _makeContextMenuButton("Combine", "c",    _selectionCanCombine(), menuButton);
        UI::ButtonPtr editButton            = _makeContextMenuButton("Edit…",   "ret",  _selectionCanEdit(),    menuButton);
        UI::ButtonPtr deleteButton          = _makeContextMenuButton("Delete",  "del",  _selectionCanDelete(),  menuButton);
        
        UI::MenuPtr menu = subviewCreate<UI::Menu>();
        menu->buttons({ createBranchButton, combineButton, editButton, deleteButton });
        menu->sizeToFit(ConstraintNoneSize);
        menu->origin(mouseDownEvent.mouse.origin);
        menu->track();
        
        // Handle the clicked button
        std::optional<_GitOp> gitOp;
        
        if (menuButton == createBranchButton.get()) {
            assert(_selection.commits.size() == 1);
            menu = nullptr;
            const Rev& rev = mouseDownColumn->rev();
            const Git::Commit& commit = *_selection.commits.begin();
            _revCreate(_RevCreateType::Branch, rev, commit);
        
        } else if (menuButton == combineButton.get()) {
            gitOp = _GitOp{
                .type = _GitOp::Type::Combine,
                .src = {
                    .rev = _selection.rev,
                    .commits = _selection.commits,
                },
            };
        
        } else if (menuButton == editButton.get()) {
            gitOp = _GitOp{
                .type = _GitOp::Type::Edit,
                .src = {
                    .rev = _selection.rev,
                    .commits = _selection.commits,
                },
            };
        
        } else if (menuButton == deleteButton.get()) {
            gitOp = _GitOp{
                .type = _GitOp::Type::Delete,
                .src = {
                    .rev = _selection.rev,
                    .commits = _selection.commits,
                },
            };
        }
        
        return gitOp;
    }
    
    enum class _RefStateRestoreSelection {
        None,
        
    };
    
    Git::Ref _refStateRestore(const Git::Ref& refPrev, const State::RefState& refState) {
        Git::Ref ref = refPrev;
        Git::Commit commit;
        try {
            commit = State::Convert(_repo, refState.head);
        } catch (...) {
            throw Toastbox::RuntimeError("failed to find commit %s", refState.head.c_str());
        }
        
        // Set the ref's commit if it changed
        if (ref.commit() != commit) {
            ref = _gitRefReplace(ref, commit);
        }
        
        // Rename the ref if the name changed
        if (ref.name() != refState.name) {
            ref = _gitRefRename(ref, refState.name);
        }
        
        return ref;
    }
    
    void _trackSnapshotsMenu(UI::RevColumnPtr col) {
        UI::SnapshotButton* menuButton = nullptr;
        Git::Ref ref = col->rev().ref;
        std::vector<UI::ButtonPtr> buttons = {
            _makeSnapshotMenuButton(ref, _repoState.snapshotInitial(ref), true, menuButton),
        };
        
        const std::vector<State::Snapshot>& snapshots = _repoState.snapshots(ref);
        for (auto it=snapshots.rbegin(); it!=snapshots.rend(); it++) {
            // Creating the button will throw if we can't get the commit for the snapshot
            // If that happens, just don't shown the button representing the snapshot
            try {
                buttons.push_back(_makeSnapshotMenuButton(ref, *it, false, menuButton));
            } catch (...) {}
        }
        
        const int width = _SnapshotMenuWidth+UI::SnapshotMenu::Padding().x;
        const UI::Point p = {col->origin().x+(col->size().x-width)/2, 2};
        const int heightMax = size().y-p.y;
        
        UI::SnapshotMenuPtr menu = subviewCreate<UI::SnapshotMenu>();
        menu->title()->text("Session Start");
        menu->buttons(buttons);
        menu->size(menu->sizeIntrinsic({ConstraintNone, heightMax}));
        menu->origin(p);
//        layout(*this, {});
        menu->track();
        
        if (menuButton) {
            State::History& h = _repoState.history(ref);
            const State::RefState& refState = menuButton->snapshot().refState;
            const State::RefState& refStatePrev = h.get().refState;
            
            if (refState != refStatePrev) {
                ref = _refStateRestore(ref, refState);
                
                // Re-get `h` (the history) here!
                // This is necessary because _refStateRestore() -> _gitRefRename() -> RepoState::refReplace()
                // invalidates the existing history, so the pre-existing `h` is no longer valid.
                State::History& h = _repoState.history(ref);
                h.push(State::HistoryRefState(ref));
                // We made a modification -- push the initial snapshot
                _repoState.snapshotInitialPush(ref);
                
                // Clear the selection when restoring a snapshot
                _selection = {};
                _reload();
            }
        }
    }
    
    void _undoRedo(UI::RevColumnPtr col, bool undo) {
        Rev rev = col->rev();
        State::History& h = _repoState.history(rev.ref);
        const State::HistoryRefState refStatePrev = h.get();
        const State::HistoryRefState refState = (undo ? h.prevPeek() : h.nextPeek());
        
        try {
            (Git::Rev&)rev = _refStateRestore(rev.ref, refState.refState);
            
            const std::set<Git::Commit> selection = State::Convert(_repo,
                (undo ? refStatePrev.selectionPrev : refState.selection));
            
            _selection = {
                .rev = rev,
                .commits = selection,
            };
            
            // Re-get `h` (the history) here!
            // This is necessary because _gitRefRename() calls RepoState::refReplace(),
            // which invalidates the existing history, so the pre-existing `h` is no
            // longer valid.
            State::History& h = _repoState.history(rev.ref);
            if (undo) h.prev();
            else      h.next();
            
            // We made a modification -- push the initial snapshot
            _repoState.snapshotInitialPush(rev.ref);
        
        } catch (const std::exception& e) {
            if (undo) throw Toastbox::RuntimeError("undo failed: %s", e.what());
            else      throw Toastbox::RuntimeError("redo failed: %s", e.what());
        }
        
        _reload();
    }
    
    void _gitOpExec(const _GitOp& gitOp) {
        const _GitModify::Ctx ctx = {
            .repo = _repo,
            .refReplace = [&] (const Git::Ref& ref, const Git::Commit& commit) { return _gitRefReplace(ref, commit); },
            .spawn = [&] (const char*const* argv) { _gitSpawn(argv); },
            .conflictsResolve = [&] (const Git::Index& index, const std::vector<Git::Conflict>& fcs) { _gitConflictsResolve(gitOp, index, fcs); },
        };
        
        auto opResult = _GitModify::Exec(ctx, gitOp);
        if (!opResult) return;
        
        Rev srcRevPrev = gitOp.src.rev;
        Rev dstRevPrev = gitOp.dst.rev;
        Rev srcRev = opResult->src.rev;
        Rev dstRev = opResult->dst.rev;
        assert((bool)srcRev.ref == (bool)srcRevPrev.ref);
        assert((bool)dstRev.ref == (bool)dstRevPrev.ref);
        
        State::History* srcHistory = (srcRev.ref ? &_repoState.history(srcRev.ref) : nullptr);
        if (srcHistory && srcRev.commit!=srcRevPrev.commit) {
            State::HistoryRefState refState(srcRev.ref);
            refState.selection = State::Convert(opResult->src.selection);
            refState.selectionPrev = State::Convert(opResult->src.selectionPrev);
            srcHistory->push(refState);
            // We made a modification -- push the initial snapshot
            _repoState.snapshotInitialPush(srcRev.ref);
        }
        
        State::History* dstHistory = (dstRev.ref ? &_repoState.history(dstRev.ref) : nullptr);
        if (dstHistory && dstRev.commit!=dstRevPrev.commit) {
            State::HistoryRefState refState(dstRev.ref);
            refState.selection = State::Convert(opResult->dst.selection);
            refState.selectionPrev = State::Convert(opResult->dst.selectionPrev);
            dstHistory->push(refState);
            // We made a modification -- push the initial snapshot
            _repoState.snapshotInitialPush(dstRev.ref);
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
            colors(_ColorsCreate(_theme));
        } else {
            colors(_ColorsCreateDefault());
        }
        
//        _colorsPrev = _ColorsSet(_colors);
        
        // Set _colors as the color pallette used by all Views
//        View::Colors(_colors);
        
//        _cursorState = UI::CursorState(false, {});
        
        // Hide cursor
        ::curs_set(0);
        
        ::mousemask(ALL_MOUSE_EVENTS | REPORT_MOUSE_POSITION, NULL);
        ::mouseinterval(0);
        
        ::set_escdelay(0);
    }

    void _cursesDeinit() noexcept {
    //    ::mousemask(0, NULL);
        
//        _cursorState.restore();
        
        // Restore colors
        colors({});
        
//        _ColorsSet(_colorsPrev);
        ::endwin();
        
    //    sleep(1);
    }
    
    bool _gitDetachHeadIfEqual(const Git::Ref& ref) {
        assert(ref);
        if (_head.ref != ref) return false;
        if (!_headReattach) {
            _headReattach = true;
            _repo.headDetach();
        }
        return true;
    }
    
    Git::Ref _gitRefRename(const Git::Ref& refPrev, const std::string& name) {
        const Git::Ref ref = _repo.refCopy(refPrev, name);
        
        // Update all revs in _revs
        for (Rev& rev : _revs) {
            if (rev.ref == refPrev) {
                rev.ref = ref;
            }
        }
        
        // Update _repoState
        _repoState.refReplace(refPrev, ref);
        
        // Update _head
        if (_gitDetachHeadIfEqual(refPrev)) {
            _head.ref = ref;
        }
        
        // Update _selection
        if (_selection.rev.ref == refPrev) {
            (Git::Rev&)_selection.rev = ref;
        }
        
        // Remember the new ref so it appears in subsequent debase launches
        _repo.reflogRememberRef(ref);
        
        // Delete the original ref
        _repo.refDelete(refPrev);
        return ref;
    }
    
    void _gitRefDelete(const Git::Ref& ref) {
        // Update _revs by removing revs that match `ref`
        _revs.erase(
            std::remove_if(_revs.begin(), _revs.end(), [&] (const Rev& rev) { return rev.ref==ref; }),
        _revs.end());
        
        // Update _repoState
        _repoState.refRemove(ref);
        
        // Update _head
        if (_gitDetachHeadIfEqual(ref)) {
            _head = {};
        }
        
        // Update _selection
        if (_selection.rev.ref == ref) {
            _selection = {};
        }
        
        // Delete the ref
        _repo.refDelete(ref);
    }
    
    Git::Ref _gitRefReplace(const Git::Ref& ref, const Git::Commit& commit) {
        // Detach HEAD if it's attached to the ref that we're modifying, otherwise
        // we'll get an error when we try to replace that ref.
        _gitDetachHeadIfEqual(ref);
        return _repo.refReplace(ref, commit);
    }
    
    void _gitSpawn(const char*const* argv) {
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
    
    std::string _gitConflictResolveInEditor(const Git::Conflict& fc) {
        const Git::ConflictMarkers markers = Git::ConflictMarkers{
            .start      = Git::Repo::MergeMarkerBareStart,
            .separator  = Git::Repo::MergeMarkerBareSeparator,
            .end        = Git::Repo::MergeMarkerBareEnd,
        };
        std::string conflictStr = Git::ConflictStringFromHunks(markers, fc.hunks);
        
        auto spawn = [&] (const char*const* argv) { _gitSpawn(argv); };
        for (;;) {
            conflictStr = Git::EditorRun(_repo, spawn, conflictStr);
            
            // Verify that there are no conflict markers
            if (Git::ConflictStringContainsConflictMarkers(markers, conflictStr)) {
                std::optional<bool> clicked;
                auto alert = _panelPresent<UI::ErrorAlert>();
                alert->width                        (50);
                alert->message()->text              ("The file still contains conflict markers. Continue editing?");
                alert->okButton()->label()->text    ("Edit");
                alert->okButton()->action           ( [&] (UI::Button&) { clicked = true; } );
                alert->dismissButton()->action      ( [&] (UI::Button&) { clicked = false; } );
                
                // Wait until the user clicks a button
                while (!clicked) track(Once);
                
                // If the user canceled, let the caller know
                if (!*clicked) throw _GitModify::ConflictResolveCanceled();
                continue;
            }
            
            // If this is a submodule conflict, validate the hash
            if (fc.type == Git::Conflict::Type::Submodule) {
                const Git::Repo submoduleRepo = Git::Repo::Open(_repo.path() / fc.path);
                try {
                    conflictStr = Toastbox::String::Trim(conflictStr); // Remove whitespace from the hash
                    submoduleRepo.commitLookup(Git::IdFromString(conflictStr));
                
                } catch (...) {
                    // Submodule hash is invalid
                    std::optional<bool> clicked;
                    auto alert = _panelPresent<UI::ErrorAlert>();
                    alert->width                        (50);
                    alert->message()->text              ("The submodule commit hash is invalid. Continue editing?");
                    alert->okButton()->label()->text    ("Edit");
                    alert->okButton()->action           ( [&] (UI::Button&) { clicked = true; } );
                    alert->dismissButton()->action      ( [&] (UI::Button&) { clicked = false; } );
                    
                    // Wait until the user clicks a button
                    while (!clicked) track(Once);
                    
                    // If the user canceled, let the caller know
                    if (!*clicked) throw _GitModify::ConflictResolveCanceled();
                    continue;
                }
            }
            
            // No issues, we're done
            break;
        }
        return conflictStr;
    }
    
    std::optional<std::string> _gitRunConflictPanel(
        UI::ConflictPanel::Layout layout,
        size_t conflictIdx, size_t conflictCount,
        const Rev& revOurs, const Rev& revTheirs,
        const Git::Conflict& fc) {
        
        std::vector<std::string> lines;
        for (size_t i=0; i<fc.hunks.size(); i++) {
            const Git::Conflict::Hunk& hunk = fc.hunks[i];
            
            // If it's a normal hunk (ie not a conflict), just append the lines
            if (hunk.type == Git::Conflict::Hunk::Type::Normal) {
                lines.insert(lines.end(), hunk.normal.lines.begin(), hunk.normal.lines.end());
                continue;
            }
            
            const auto& linesOurs = hunk.conflict.linesOurs;
            const auto& linesTheirs = hunk.conflict.linesTheirs;
            const std::string title = "Conflict [" + std::to_string(conflictIdx+1) + "/" + std::to_string(conflictCount) + "]";
            auto panel = _panelPresent<UI::ConflictPanel>(
                layout, title,
                revOurs.displayName(), revTheirs.displayName(),
                fc, i
            );
            
            std::optional<UI::ConflictPanel::Result> panelResult;
            panel->doneAction() = [&] (UI::ConflictPanel::Result r) {
                panelResult = r;
            };
            
            // The following loop only exists for the case where the user: chooses 'Open in Editor',
            // then leaves conflict markers in the file, and then hits 'Cancel' in the ensuring
            // error window.
            // In that case: keep showing the conflict panel (see 'continue', below).
            // In all other cases: break after first iteration.
            for (;;) {
                panelResult = std::nullopt;
                
                // Wait until the user clicks a button
                while (!panelResult) track(Once);
                
                switch (*panelResult) {
                case UI::ConflictPanel::Result::ChooseOurs:
                    lines.insert(lines.end(), linesOurs.begin(), linesOurs.end());
                    break;
                case UI::ConflictPanel::Result::ChooseTheirs:
                    lines.insert(lines.end(), linesTheirs.begin(), linesTheirs.end());
                    break;
                case UI::ConflictPanel::Result::OpenInEditor:
                    try {
                        const std::string editorContent = _gitConflictResolveInEditor(fc);
                        // If the user cleared the conflict file, and there's a branch of the
                        // conflict that represents a non-existent file, then consider the
                        // file as non-existent, rather than an empty file.
                        const bool noFile = fc.noFile(Git::Conflict::Side::Ours) || fc.noFile(Git::Conflict::Side::Theirs);
                        if (editorContent.empty() && noFile) {
                            return std::nullopt;
                        }
                        return editorContent;
                    } catch (const _GitModify::ConflictResolveCanceled&) {
                        // User canceled editing; keep displaying conflict panel
                        continue;
                    } catch (...) {
                        throw;
                    }
                    break;
                case UI::ConflictPanel::Result::Cancel:
                    throw _GitModify::ConflictResolveCanceled();
                default:
                    abort();
                }
                break;
            }
            
            conflictIdx++;
        }
        
        if (lines.empty()) return std::nullopt;
        return Toastbox::String::Join(lines, "\n");
    }
    
    void _gitConflictsResolve(const _GitOp& op, const Git::Index& index, const std::vector<Git::Conflict>& fcs) {
        // op.dst.rev is optional (depending on the git operation), so if it doesn't exist,
        // fallback to op.src.rev (which is required)
        const Rev revOurs = (op.dst.rev ? op.dst.rev : op.src.rev);
        const Rev revTheirs = op.src.rev;
//        std::string refNameOurs = op.dst.rev.displayName();
//        std::string refNameTheirs = op.src.rev.displayName();
        
        // Determine the conflict panel layout (ie which rev is on the left vs right)
        UI::ConflictPanel::Layout layout = UI::ConflictPanel::Layout::LeftOurs;
        for (const Rev& rev : _revs) {
            if (rev == revOurs) {
                layout = UI::ConflictPanel::Layout::LeftOurs;
                break;
            } else if (rev == revTheirs) {
                layout = UI::ConflictPanel::Layout::RightOurs;
                break;
            }
        }
        
        // Count the total number of conflict
        size_t conflictCount = 0;
        for (const Git::Conflict& fc : fcs) {
            conflictCount += fc.conflictCount();
        }
        
        // Show the conflict panel for each hunk within each Conflict
        size_t conflictIdx = 0;
        for (const Git::Conflict& fc : fcs) {
            const std::optional<std::string> content = _gitRunConflictPanel(layout, conflictIdx, conflictCount, revOurs, revTheirs, fc);
            Git::ConflictResolve(_repo, index, fc, content);
            conflictIdx += fc.conflictCount();
        }
    }
    
    void _layoutPanel(UI::PanelPtr panel) {
        panel->size(panel->sizeIntrinsic(bounds().size));
        
        UI::Size ps = panel->size();
        UI::Size rs = size();
        UI::Point p = {
            (rs.x-ps.x)/2,
            (rs.y-ps.y)/3,
        };
        panel->origin(p);
    }
    
    template <typename T_Async>
    void _waitForAsync(const T_Async& async, Deadline deadline=Forever, UI::PanelPtr panel=nullptr, UI::ButtonPtr button=nullptr) {
        
        if (panel) panel->enabled(false);
        Defer( if (panel) panel->enabled(true); );
        
        // Animate until we get a response
        UI::ButtonSpinnerPtr spinner = (button ? UI::ButtonSpinner::Create(button) : nullptr);
        Deadline nextDeadline;
        
        for (;;) {
            if (async.done()) break;
            
            std::chrono::steady_clock::time_point time = std::chrono::steady_clock::now();
            if (deadline!=Forever && time>deadline) break;
            
            if (time > nextDeadline) {
                if (spinner) spinner->animate();
                nextDeadline = std::chrono::steady_clock::now()+std::chrono::milliseconds(100);
            }
            
            track(nextDeadline);
        }
    }
    
    void _errorMessageRun(std::string_view msg) {
        bool done = false;
        
        auto alert = _panelPresent<UI::ErrorAlert>();
        alert->width                (40);
        alert->message()->text      (msg);
        alert->okButton()->action   ( [&] (UI::Button&) { done = true; } );
        
        // Sleep 10ms to prevent an odd flicker that occurs when showing a panel
        // as a result of pressing a keyboard key. For some reason showing panels
        // due to a mouse event doesn't trigger the flicker, only keyboard events.
        // For some reason sleeping a short time fixes it.
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        
        // Wait until the user clicks a button
        while (!done) track(Once);
    }
    
//    void _conflictRun() {
//        
//        Git::Conflict fc = {
//            .path = "/Some/Really/Long/Path/That/We/Want/To/Truncate/FDStream.h",
//            .hunks = {
//                Git::Conflict::Hunk{
//                    .type = Git::Conflict::Hunk::Type::Normal,
//                    .normal = {
//                        .lines = {
//                            "#pragma once",
//                            "#include <fstream>",
//                            "#include <fstream>",
//                            "#include <fstream>",
//                            "#include <fstream>",
//                            "#include <fstream>",
//                            "#include <fstream>",
//                            "#include <fstream>",
//                            "#include <fstream>",
//                            "#include <fstream>",
//                            "#include <fstream>",
//                            "#include <fstream>",
//                        },
//                    },
//                },
//                
//                Git::Conflict::Hunk{
//                    .type = Git::Conflict::Hunk::Type::Conflict,
//                    .conflict = {
//                        .linesOurs = {
//                            "#include <memory>",
//                            "#include <cassert>",
//                            "",
//                            "#ifdef __GLIBCXX__",
//                            "#include <ext/stdio_filebuf.h>",
//                            "#endif",
//                        },
//                        
//                        .linesTheirs = {
////                            "hello",
//                        },
//                    },
//                },
//                
//                Git::Conflict::Hunk{
//                    .type = Git::Conflict::Hunk::Type::Normal,
//                    .normal = {
//                        .lines = {
//                            "",
//                            "namespace Toastbox {",
//                            "",
//                            "// FDStream allows an istream/ostream/iostream to be created from a file descriptor.",
//                            "// The FDStream object takes ownership of the file descriptor (actually, the",
//                            "// underlying platform-specific filebuf object) and closes it when FDStream is",
//                            "// destroyed.",
//                            "",
//                            "class FDStream {",
//                            "protected:",
//                            "#ifdef __GLIBCXX__",
//                            "    // GCC (libstdc++)",
//                            "    using _Filebuf = __gnu_cxx::stdio_filebuf<char>;",
//                            "    _Filebuf* _initFilebuf(int fd, std::ios_base::openmode mode) {",
//                            "//        if (fd >= 0) {",
//                            "//            _filebuf = std::make_unique<_Filebuf>(fd, mode);",
//                            "//        } else {",
//                            "//            _filebuf = std::make_unique<_Filebuf>();",
//                            "//        }",
//                            "//        return _filebuf.get();",
//                            "        ",
//                            "//        if (fd < 0) return nullptr;",
//                            "//        _filebuf = std::make_unique<_Filebuf>(fd, mode);",
//                            "//        return _filebuf.get();",
//                            "        ",
//                            "        assert(fd >= 0);",
//                            "        _filebuf = std::make_unique<_Filebuf>(fd, mode);",
//                            "        return _filebuf.get();",
//                            "    }",
//                            "#else",
//                            "    // Clang (libc++)",
//                            "    using _Filebuf = std::basic_filebuf<char>;",
//                            "    _Filebuf* _initFilebuf(int fd, std::ios_base::openmode mode) {",
//                            "//        _filebuf = std::make_unique<_Filebuf>();",
//                            "//        if (fd >= 0) {",
//                            "//            _filebuf->__open(fd, mode);",
//                            "//        }",
//                            "//        return _filebuf.get();",
//                            "        ",
//                            "//        if (fd < 0) return nullptr;",
//                            "//        _filebuf = std::make_unique<_Filebuf>();",
//                            "//        _filebuf->__open(fd, mode);",
//                            "//        return _filebuf.get();",
//                            "        ",
//                            "        assert(fd >= 0);",
//                            "        _filebuf = std::make_unique<_Filebuf>();",
//                            "        _filebuf->__open(fd, mode);",
//                            "        return _filebuf.get();",
//                            "    }",
//                            "#endif",
//                            "    ",
//                            "    void _swap(FDStream& x) {",
//                            "        std::swap(_filebuf, x._filebuf);",
//                            "    }",
//                            "    ",
//                            "    // _Filebuf needs to be a unique_ptr so that the pointer returned by",
//                            "    // _initFilebuf() stays valid after swap() is called. Otherwise, if",
//                            "    // _filebuf was an inline ivar, the pointer given to the iostream",
//                            "    // constructor would reference a different object after swap() was",
//                            "    // called.",
//                            "    std::unique_ptr<_Filebuf> _filebuf;",
//                            "};",
//                            "",
//                            "//class FDStreamIn : public FDStream, public std::istream {",
//                            "//public:",
//                            "//    FDStreamIn(int fd=-1) : std::istream(_initFilebuf(fd, std::ios::in)) {}",
//                            "//};",
//                            "//",
//                            "//class FDStreamOut : public FDStream, public std::ostream {",
//                            "//public:",
//                            "//    FDStreamOut(int fd=-1) : std::ostream(_initFilebuf(fd, std::ios::out)) {}",
//                            "//};",
//                            "",
//                            "class FDStreamInOut : public FDStream, public std::iostream {",
//                            "public:",
//                            "    FDStreamInOut() : std::iostream(nullptr) {}",
//                            "    FDStreamInOut(int fd) : std::iostream(_initFilebuf(fd, std::ios::in|std::ios::out)) {}",
//                            "    FDStreamInOut(FDStreamInOut&& x) : FDStreamInOut() {",
//                            "        swap(x);",
//                            "    }",
//                            "    ",
//                            "    // Move assignment operator",
//                            "    FDStreamInOut& operator=(FDStreamInOut&& x) {",
//                            "        swap(x);",
//                            "        return *this;",
//                            "    }",
//                            "    ",
//                            "    void swap(FDStreamInOut& x) {",
//                            "        FDStreamInOut::_swap(x);",
//                            "        std::iostream::swap(x);",
//                            "        rdbuf(_filebuf.get());",
//                            "        x.rdbuf(x._filebuf.get());",
//                            "    }",
//                            "};",
//                            "",
//                            "}",
//                            "",
//                        },
//                    },
//                },
//            },
//        };
//        
//        UI::ConflictPanel::Layout layout = UI::ConflictPanel::Layout::LeftOurs;
//        auto panel = _panelPresent<UI::ConflictPanel>(layout, "master", "SomeBranch", fc, 1);
//        track();
//    }
    
    void _moveOffer() {
        namespace fs = std::filesystem;
        
        // Short-circuit if we've already asked the user to move this version (or a newer version) of debase
        {
            State::State state(StateDir());
            if (state.lastMoveOfferVersion() >= DebaseVersion) return;
        }
        
        _Path currentExecutablePath;
        try {
            currentExecutablePath = CurrentExecutablePath();
        } catch (...) {
            return;
        }
        
        // If our executable doesn't contain the string 'debase', then don't offer to move it.
        // This is a safety mechanism since we're going to copy+remove ourself to the ~/bin
        // directory below, and we want to make sure we never remove anything unless it's
        // called "debase", to prevent any kind of situation where we accidentally delete
        // the wrong file.
        if (currentExecutablePath.filename().string().find(DebaseFilename) == std::string::npos) return;
        
        // Short-circuit if we're already in user's PATH
        if (PathIsInEnvironmentPath(currentExecutablePath.parent_path())) return;
        
        // Bail if we couldn't get the user's home dir
        const char* homeEnv = getenv("HOME");
        if (!homeEnv) return;
        
        const _Path homePath = homeEnv;
        const _Path binDirRelativePath = UserBinRelativePath();
        const _Path binDirPath = homePath / binDirRelativePath;
        bool createBinDir = false;
        try {
            // exists() can throw if binDirPath isn't accessible (eg because of parent directory permissions)
            createBinDir = !fs::exists(binDirPath);
        } catch (...) {
            return;
        }
        
        // We don't want to update the shell path is XDG is detected
        bool updateShellPath = !PathIsInEnvironmentPath(binDirPath);
        
        // We removed the code below that sets updateShellPath=false if XDG is detected,
        // because on Ubuntu at least, ~/.local/bin is added to PATH in .profile and only
        // if the directory already exists. So if the directory doesn't exist (and
        // therefore we created it), the user would have to logout+login in order to reload
        // .profile and have it update PATH now that ~/.local/bin exists.
        // That's crappy UX so, so we add ~/.local/bin to PATH in .bashrc, which is reloaded
        // upon every new shell. That'll cause ~/.local/bin to be harmlessly repeated once
        // in PATH (once from ~/.profile, and once from ~/.bashrc), but that seems better
        // than the crappy UX of having to logout. (We could add a check to only add
        // ~/.local/bin if it's not already in PATH though, but we didn't do that for
        // simplicity.)
        
//#if __linux__
//        // On Linux, if it looks like the system is XDG compliant, don't update
//        // the shell PATH because XDG should already include the bin directory.
//        // From the XDG spec:
//        // 
//        //     User-specific executable files may be stored in $HOME/.local/bin.
//        //     Distributions should ensure this directory shows up in the UNIX
//        //     $PATH environment variable, at an appropriate place.
//        //
//        if (XDGDetected()) updateShellPath = false;
//#endif
        constexpr const char* ShellBash = "bash";
        constexpr const char* ShellZsh  = "zsh";
        
        struct ShellInfo {
            _Path profilePath;
            std::string updatePathCommand;
            std::string restartShellCommand;
        };
        
        ShellInfo shell;
        std::string shellName;
        try {
            shellName = ProcessPathGet(getppid()).filename();
        } catch (...) {
            return;
        }
        
        if (shellName == ShellBash) {
            shell = {
                .profilePath         = homePath / ".bashrc",
                .updatePathCommand   = "PATH=\"$HOME/" + binDirRelativePath.string() + ":$PATH\"",
                .restartShellCommand = "exec bash",
            };
        
        } else if (shellName == ShellZsh) {
            shell = {
                .profilePath         = homePath / ".zshrc",
                .updatePathCommand   = "path+=\"$HOME/" + binDirRelativePath.string() + "\"",
                .restartShellCommand = "exec zsh",
            };
        
        } else {
            // Unknown shell
            return;
        }
        
        constexpr const char* Title = "Move debase";
        std::string message = "Would you like to move debase to ~/" + binDirRelativePath.string() + " so that it can be invoked by typing `debase` in your shell?";
        
        if (createBinDir && updateShellPath) {
            message +=
                "\n\n"
                "The ~/" + binDirRelativePath.string() + " directory will be created, and PATH will be updated in ~/" + shell.profilePath.filename().string() + ".";
        
        } else if (createBinDir) {
            message +=
                "\n\n"
                "The ~/" + binDirRelativePath.string() + " directory will be created.";
        
        } else if (updateShellPath) {
            message +=
                "\n\n"
                "PATH will be updated in ~/" + shell.profilePath.filename().string() + ".";
        }
        
        std::optional<bool> moveChoice;
        auto alert = _panelPresent<UI::Alert>();
        alert->width                            (50);
        alert->color                            (colors().menu);
        alert->title()->text                    (Title);
        alert->message()->text                  (message);
        alert->okButton()->label()->text        ("Move");
        alert->dismissButton()->label()->text   ("Don't Move");
        alert->okButton()->action               ( [&] (UI::Button& b) { moveChoice = true; } );
        alert->dismissButton()->action          ( [&] (UI::Button& b) { moveChoice = false; } );
        
        // Wait until the user clicks a button
        while (!moveChoice) track(Once);
        
        // We offerred to move debase; update State so we remember that we did so
        {
            State::State state(StateDir());
            state.lastMoveOfferVersion(DebaseVersion);
            state.write();
        }
        
        // Bail if the user doesn't want to move
        if (!*moveChoice) return;
        
        // Create the ~/bin directory
        fs::create_directories(binDirPath);
        
        // Copy+remove
        // Not using fs::rename because it'll fail if `currentExecutablePath` is on a different volume than ~/bin
        const auto copyOptions = fs::copy_options::overwrite_existing;
        fs::copy(currentExecutablePath, binDirPath/DebaseFilename, copyOptions);
        fs::remove(currentExecutablePath);
        
        // Update shell path
        // Do this after the move, so that we don't do it if the move fails for some reason
        if (updateShellPath) {
            std::ofstream shellProfile(shell.profilePath, std::ios_base::app|std::ios_base::out);
            shellProfile << "\n";
            shellProfile << "# Update PATH to include ~/bin; added by debase" << "\n";
            shellProfile << shell.updatePathCommand << "\n";
        }
        
        static struct {
            ShellInfo shell;
            _Path binDirRelativePath;
            bool updateShellPath = false;
        } ctx = {
            shell,
            binDirRelativePath,
            updateShellPath,
        };
        
//        static {
//            .shell              = shell,
//            .binDirRelativePath = binDirRelativePath,
//            .updateShellPath    = updateShellPath,
//        } ctx;
        
        atexit(+[]() {
            std::cout     << "\n";
            std::cout     << "*** debase was moved to ~/" << (ctx.binDirRelativePath/DebaseFilename).string() << "\n";
            std::cout     << "*** \n";
            
            if (ctx.updateShellPath) {
                std::cout << "*** To invoke again by typing `debase`, you may need to restart your shell with:" << "\n";
                std::cout << "*** \n";
                std::cout << "***   " << ctx.shell.restartShellCommand << "\n";
                std::cout << "*** \n";
                std::cout << "*** to ensure that your shell can find the new location." << "\n";
            
            } else {
                std::cout << "*** You can now invoke debase by simply typing `debase`.\n";
            }
            
            std::cout << "\n";
        });
    }
    
    Git::Repo _repo;
    std::vector<Rev> _revs;
    
//    UI::ColorPalette _colors;
//    UI::ColorPalette _colorsPrev;
    State::RepoState _repoState;
    Git::Rev _head;
    bool _headReattach = false;
    std::vector<UI::RevColumnPtr> _columns;
    UI::RevColumnPtr _columnNameFocused;
//    UI::CursorState _cursorState;
    State::Theme _theme = State::Theme::None;
    
    struct {
        UI::CommitPanelPtr titlePanel;
        std::vector<UI::PanelPtr> shadowPanels;
        std::optional<UI::Rect> insertionMarker;
        bool copy = false;
    } _drag;
    
    struct {
        Rev rev;
        Git::Commit commit;
        UI::Point mouseUpOrigin;
        std::chrono::steady_clock::time_point mouseUpTime;
    } _doubleClickState;
    
    _Selection _selection;
    std::optional<UI::Rect> _selectionRect;
    
    std::deque<UI::PanelPtr> _panels;
};
