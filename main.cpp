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
#include <spawn.h>
#include "lib/Toastbox/RuntimeError.h"
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

struct _Selection {
    Git::Rev rev;
    std::set<Git::Commit> commits;
};

static UI::ColorPalette _Colors;
static std::optional<UI::ColorPalette> _ColorsPrev;

static Git::Repo _Repo;
static std::vector<Git::Rev> _Revs;

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
static constexpr std::chrono::milliseconds _MenuStayOpenThresh(300);

static _Selection _Selection;
static std::optional<UI::Rect> _SelectionRect;

static UI::Menu _Menu;

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
        
        if (_Menu) {
            _Menu->orderFront();
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
        
        if (_Menu) {
            _Menu->drawIfNeeded();
        }
        
        if (_ErrorPanel) {
            _ErrorPanel->drawIfNeeded();
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

static std::optional<UI::Event> _WaitForMouseEvent(MEVENT& mouse, MouseButtons buttons=MouseButtons::Left) {
    const mmask_t mouseReleasedMask = _MouseButtonReleasedMask(buttons);
    // Wait for another mouse event
    for (;;) {
        UI::Event ev = _RootWindow->nextEvent();
        if (ev == UI::Event::KeyEscape) return UI::Event::KeyEscape;
        if (ev != UI::Event::Mouse) continue;
        int ir = ::getmouse(&mouse);
        if (ir != OK) continue;
        if (mouse.bstate & mouseReleasedMask) return std::nullopt;
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
    
    MEVENT mouse = mouseDownEvent;
    std::optional<_InsertionPosition> ipos;
    bool abort = false;
    bool mouseDragged = false;
    for (;;) {
        assert(!_Selection.commits.empty());
        UI::RevColumn selectionColumn = _ColumnForRev(_Selection.rev);
        
        const UI::Point p = {mouse.x, mouse.y};
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
                const bool copy = (mouse.bstate & BUTTON_ALT) || forceCopy;
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
        std::optional<UI::Event> ev = _WaitForMouseEvent(mouse);
        abort = (ev && *ev==UI::Event::KeyEscape);
        if (!ev || abort) break;
    }
    
    std::optional<Git::Op> gitOp;
    if (!abort) {
        if (_Drag.titlePanel && ipos) {
            Git::Commit dstCommit = ((ipos->iter != ipos->col->panels().end()) ? (*ipos->iter)->commit() : nullptr);
            gitOp = Git::Op{
                .repo = _Repo,
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
                        .repo = _Repo,
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
        std::optional<UI::Event> ev = _WaitForMouseEvent(mouse);
        abort = (ev && *ev==UI::Event::KeyEscape);
        if (!ev || abort) break;
    }
    
    // Reset state
    {
        _SelectionRect = std::nullopt;
    }
}

static std::optional<Git::Op> _TrackRightMouse(MEVENT mouseDownEvent, UI::RevColumn mouseDownColumn, UI::CommitPanel mouseDownPanel) {
    auto mouseDownTime = std::chrono::steady_clock::now();
    MEVENT mouse = mouseDownEvent;
    
    // If the commit that was clicked isn't selected, set the selection to only that commit
    if (!_Selected(mouseDownColumn, mouseDownPanel)) {
        _Selection = {
            .rev = mouseDownColumn->rev(),
            .commits = {mouseDownPanel->commit()},
        };
    }
    
    if (!_Selection.rev.isMutable()) {
        // No beep() because it feels too aggressive to beep in this case
        return std::nullopt;
    }
    
    static UI::MenuButton CombineButton = {"Combine", "c"};
    static UI::MenuButton EditButton    = {"Edit",    "ret"};
    static UI::MenuButton DeleteButton  = {"Delete",  "del"};
    
    assert(!_Selection.commits.empty());
    
    if (_Selection.commits.size() > 1) {
        static const UI::MenuButton* Buttons[] = {
            &CombineButton,
            &DeleteButton,
        };
        _Menu = MakeShared<UI::Menu>(_Colors, Buttons);
    
    } else {
        static const UI::MenuButton* Buttons[] = {
            &EditButton,
            &DeleteButton,
        };
        _Menu = MakeShared<UI::Menu>(_Colors, Buttons);
    }
    
    _Menu->setPosition({mouseDownEvent.x, mouseDownEvent.y});
    
    const UI::MenuButton* menuButton = nullptr;
    MouseButtons mouseUpButtons = MouseButtons::Right;
    for (;;) {
        _Draw();
        std::optional<UI::Event> ev = _WaitForMouseEvent(mouse, mouseUpButtons);
        // Check if we should abort
        if (ev && *ev==UI::Event::KeyEscape) {
            menuButton = nullptr;
            break;
        }
        // Handle mouse up
        if (!ev) {
            if (!(mouseUpButtons & MouseButtons::Left)) {
                // If the right-mouse-up occurs soon enough after right-mouse-down, the menu should
                // stay open and we should start listening for left-mouse-down events.
                // If the right-mouse-up occurs af
                auto duration = std::chrono::steady_clock::now()-mouseDownTime;
                if (duration >= _MenuStayOpenThresh) break;
                
                // Start listening for left mouse up
                mouseUpButtons |= MouseButtons::Left;
                
                // Right mouse up, but menu stays open
                // Now start tracking both left+right mouse down
            } else {
                break;
            }
        }
        
        menuButton = _Menu->updateMousePosition({mouse.x, mouse.y});
    }
    
    // Handle the clicked button
    std::optional<Git::Op> gitOp;
    if (menuButton == &CombineButton) {
        gitOp = Git::Op{
            .repo = _Repo,
            .type = Git::Op::Type::Combine,
            .src = {
                .rev = _Selection.rev,
                .commits = _Selection.commits,
            },
        };
    
    } else if (menuButton == &EditButton) {
        gitOp = Git::Op{
            .repo = _Repo,
            .type = Git::Op::Type::Edit,
            .src = {
                .rev = _Selection.rev,
                .commits = _Selection.commits,
            },
        };
    
    } else if (menuButton == &DeleteButton) {
        gitOp = Git::Op{
            .repo = _Repo,
            .type = Git::Op::Type::Delete,
            .src = {
                .rev = _Selection.rev,
                .commits = _Selection.commits,
            },
        };
    }
    
    // Reset state
    {
        _Menu = nullptr;
    }
    
    return gitOp;
}

// _ReloadRevs: re-create the revs backed by refs. This is necessary because after modifying a branch,
// the pre-existing git_reference's for that branch are stale (ie git_reference_target() doesn't
// reflect the changed branch). To get updated revs, we have to to re-lookup the refs (via
// Repo::refReload()), and recreate the rev from the new ref.
static void _ReloadRevs(Git::Repo repo, std::vector<Git::Rev>& revs) {
    for (Git::Rev& rev : revs) {
        if (rev.ref) {
            rev = Git::Rev(repo.refReload(rev.ref));
        }
    }
}

static void _RecreateColumns(UI::Window win, Git::Repo repo, std::vector<UI::RevColumn>& columns, std::vector<Git::Rev>& revs) {
    // Create a RevColumn for each specified branch
    constexpr int InsetX = 3;
    constexpr int ColumnWidth = 32;
    constexpr int ColumnSpacing = 6;
    
    bool showMutability = false;
    for (const Git::Rev& rev : revs) {
        if (!rev.isMutable()) {
            showMutability = true;
            break;
        }
    }
    
    columns.clear();
    int OffsetX = InsetX;
    for (const Git::Rev& rev : revs) {
        columns.push_back(MakeShared<UI::RevColumn>(_Colors, win, repo, rev, OffsetX, ColumnWidth, showMutability));
        OffsetX += ColumnWidth+ColumnSpacing;
    }
}

struct _SavedColor {
    short r = 0;
    short g = 0;
    short b = 0;
};

static std::optional<UI::ColorPalette> _ColorsSet(const UI::ColorPalette& colors, bool custom) {
    UI::ColorPalette colorsPrev;
    auto colorsAll = colors.all();
    auto colorsPrevAll = colorsPrev.all();
    for (auto i=colorsAll.begin(), ip=colorsPrevAll.begin(); i!=colorsAll.end(); i++, ip++) {
        UI::Color& c     = i->get();
        
        if (custom) {
            UI::Color& cprev = ip->get();
            cprev.idx = c.idx;
            color_content(cprev.idx, &cprev.r, &cprev.g, &cprev.b);
            ::init_color(c.idx, c.r, c.g, c.b);
        }
        
        ::init_pair(c.idx, c.idx, -1);
    }
    
    if (custom) return colorsPrev;
    return std::nullopt;
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
        colors.selectionMove    = UI::Color{Idx0+0,    0,    0, 1000};
        colors.selectionCopy    = UI::Color{Idx0+1,    0, 1000,    0};
        colors.selectionSimilar = UI::Color{Idx0+2,  550,  550, 1000};
        colors.subtitleText     = UI::Color{Idx0+3,  300,  300,  300};
        colors.menu             = UI::Color{Idx0+4,  800,  300,  300};
        colors.error            = UI::Color{Idx0+5, 1000,    0,    0};
    
    } else {
        // Colorspace: sRGB
        // These colors were derived by sampling the Apple_Terminal values when they're displayed on-screen
        colors.selectionMove    = UI::Color{Idx0+0,  463,  271, 1000};
        colors.selectionCopy    = UI::Color{Idx0+1,  165, 1000,  114};
        colors.selectionSimilar = UI::Color{Idx0+2,  671,  667, 1000};
        colors.subtitleText     = UI::Color{Idx0+3,  486,  486,  486};
        colors.menu             = UI::Color{Idx0+4,  969,  447,  431};
        colors.error            = UI::Color{Idx0+5, 1000,  298,  153};
    }
    
    return colors;
}

static UI::ColorPalette _ColorsDefaultCreate() {
    UI::ColorPalette colors;
    colors.selectionMove    = UI::Color{COLOR_BLUE};
    colors.selectionCopy    = UI::Color{COLOR_GREEN};
    colors.selectionSimilar = UI::Color{COLOR_BLACK};
    colors.subtitleText     = UI::Color{COLOR_BLACK};
    colors.menu             = UI::Color{COLOR_RED};
    colors.error            = UI::Color{COLOR_RED};
    return colors;
}

//static _SavedColor _SavedColors[8];
//
//static void _CursesSaveColors() {
//    for (size_t i=16; i<16+std::size(_SavedColors); i++) {
//        _SavedColor& c = _SavedColors[i];
//        int ir = color_content(i, &c.r, &c.g, &c.b);
//        assert(!ir);
//    }
//}
//
//static void _CursesRestoreColors() {
//    for (size_t i=1; i<std::size(_SavedColors); i++) {
//        _SavedColor& c = _SavedColors[i];
//        ::init_color(i, c.r, c.g, c.b);
//        ::init_pair(i, i, -1);
//    }
//}

static void _CursesInit() {
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
//    ::cbreak();
    ::raw();
    
    ::use_default_colors();
    ::start_color();
    
    const bool customColors = can_change_color();
//    const bool customColors = false;
    _Colors = (customColors ? _ColorsCreate() : _ColorsDefaultCreate());
    _ColorsPrev = _ColorsSet(_Colors, customColors);
    
    // Hide cursor
    ::curs_set(0);
    
    ::mousemask(ALL_MOUSE_EVENTS | REPORT_MOUSE_POSITION, NULL);
    ::mouseinterval(0);
    
    ::set_escdelay(0);
}

static void _CursesDeinit() {
//    ::mousemask(0, NULL);
    
    if (_ColorsPrev) {
        _ColorsSet(*_ColorsPrev, true);
        _ColorsPrev = std::nullopt;
    }
    
    ::endwin();
}

static void _GitInit(const std::vector<std::string>& revNames) {
    git_libgit2_init();
    
    _Repo = Git::Repo::Open(".");
    
    if (revNames.empty()) {
        _Revs.push_back(_Repo.head());
    
    } else {
        // Unique the supplied revs, because our code assumes a 1:1 mapping between Revs and RevColumns
        std::set<Git::Rev> unique;
        for (const std::string& revName : revNames) {
            Git::Rev rev = _Repo.revLookup(revName);
            if (unique.find(rev) == unique.end()) {
                _Revs.push_back(rev);
                unique.insert(rev);
            }
        }
    }
}

static void _GitDeinit() {
    git_libgit2_shutdown();
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

static void _EventLoop() {
    _RootWindow = MakeShared<UI::Window>(::stdscr);
    
    bool recreateCols = true;
    for (;;) {
        if (recreateCols) {
            _RecreateColumns(_RootWindow, _Repo, _Columns, _Revs);
            recreateCols = false;
        }
        
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
                    continue;
                }
            }
            
            const auto hitTest = _HitTest({mouse.x, mouse.y});
            if (mouse.bstate & BUTTON1_PRESSED) {
                const bool shift = (mouse.bstate & _SelectionShiftKeys);
                if (hitTest && !shift) {
                    // Mouse down inside of a CommitPanel, without shift key
                    gitOp = _TrackMouseInsideCommitPanel(mouse, hitTest->column, hitTest->panel);
                } else {
                    // Mouse down outside of a CommitPanel, or mouse down anywhere with shift key
                    _TrackMouseOutsideCommitPanel(mouse);
                }
            
            } else if (mouse.bstate & BUTTON3_PRESSED) {
                if (hitTest) {
                    gitOp = _TrackRightMouse(mouse, hitTest->column, hitTest->panel);
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
                .repo = _Repo,
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
                .repo = _Repo,
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
                .repo = _Repo,
                .type = Git::Op::Type::Edit,
                .src = {
                    .rev = _Selection.rev,
                    .commits = _Selection.commits,
                },
            };
            break;
        }
        
        case UI::Event::WindowResize: {
            recreateCols = true;
            break;
        }
        
        case UI::Event::KeyCtrlC:
        case UI::Event::KeyCtrlD: {
            return;
        }
        
        default: {
            break;
        }}
        
        if (gitOp) {
            try {
                Git::OpResult opResult = Git::Exec<_Spawn>(*gitOp);
                
                // Reload the UI
                _ReloadRevs(_Repo, _Revs);
                recreateCols = true;
                
                // Update the selection
                _Selection = {
                    .rev = opResult.dst.rev,
                    .commits = opResult.dst.commits,
                };
            
            } catch (const std::exception& e) {
                const int width = std::min(35, _RootWindow->bounds().size.x);
                _ErrorPanel = MakeShared<UI::ErrorPanel>(_Colors, width, "Error", e.what());
            }
        }
    }
}

int main(int argc, const char* argv[]) {
    #warning TODO: improve error messages: merge conflicts, deleting last branch commit
    
    #warning TODO: set_escdelay: not sure if we're going to encounter issues?
    
    #warning TODO: handle merge conflicts
    
    #warning TODO: figure out why moving/copying commits is slow sometimes
    
    #warning TODO: if we can't speed up git operations, show a progress indicator
    
    #warning TODO: backup all supplied revs before doing anything
    
    #warning TODO: properly handle moving/copying merge commits
    
    #warning TODO: show some kind of indication that a commit is a merge commit
    
    #warning TODO: make sure repo isn't modified when no changes are made to commit message
    
    #warning TODO: ensure that dragging commits to their current location is a no-op
    
    #warning TODO: when supplying refs on the command line in the form ref^ or ref~N, can we use a ref-backed rev (instead of using a commit-backed rev), and just offset the RevColumn, so that the rev is mutable?
    
//  Future:
    
    #warning TODO: move commits away from dragged commits to show where the commits will land
    
    #warning TODO: an undo/redo button
    
    #warning TODO: add column scrolling
    
    
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
    
//    volatile bool a = false;
//    while (!a);
    
    setlocale(LC_ALL, "");
    
    std::vector<std::string> revNames;
    for (int i=1; i<argc; i++) revNames.push_back(argv[i]);
    _GitInit(revNames);
    
    _CursesInit();
    _EventLoop();
    _CursesDeinit();
    
    _GitDeinit();
    
    return 0;
}
