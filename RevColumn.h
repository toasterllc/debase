#pragma once
#include "Git.h"
#include "Panel.h"
#include "CommitPanel.h"
#include "MakeShared.h"
#include "Color.h"
#include "UTF8.h"

namespace UI {

struct RevColumnOptions {
    UI::Window win;
    ColorPalette colors;
    Git::Repo repo;
    Git::Rev rev;
    bool head = false;
    Point offset;
    int width = 0;
    bool undoEnabled = false;
    bool redoEnabled = false;
    bool snapshotsEnabled = false;
};

enum class RevColumnButton : int {
    None,
    Undo,
    Redo,
    Snapshots,
};

struct RevColumnHitTestResult {
    CommitPanel panel;
    RevColumnButton button = RevColumnButton::None;
    bool buttonEnabled = false;
    
    operator bool() const {
        return panel || button!=RevColumnButton::None;
    }
    
    bool operator==(const RevColumnHitTestResult& x) const {
        if (panel != x.panel) return false;
        if (button != x.button) return false;
        if (buttonEnabled != x.buttonEnabled) return false;
        return true;
    }
    
    bool operator!=(const RevColumnHitTestResult& x) const {
        return !(*this==x);
    }
};

// RevColumn: a column in the UI containing commits (of type CommitPanel)
// for a particular `Git::Rev` (commit/branch/tag)
class _RevColumn {
public:
    _RevColumn(const RevColumnOptions& opts) : _opts(opts) {
        // Set our column name
        {
            _name = _opts.rev.displayName();
            
            if (_opts.head && _name!="HEAD") {
                _name = _name + " (HEAD)";
            }
            
            // Truncate the name to our width
            _name.erase(std::min(UTF8::Strlen(_name), (size_t)_opts.width));
        }
        
        // Create our CommitPanels
        UI::Rect nameFrame = {_opts.offset, {_opts.width, 1}};
        _truncated = Intersection(_opts.win->bounds(), nameFrame) != nameFrame;
        if (!_truncated) {
            // Create panels for each commit
            int offY = _CommitsInsetY;
            Git::Commit commit = _opts.rev.commit;
            size_t skip = _opts.rev.skip;
            while (commit) {
                if (!skip) {
                    Point p = _opts.offset + Size{0,offY};
                    UI::CommitPanel panel = MakeShared<UI::CommitPanel>(_opts.colors, false, _opts.width, commit);
                    UI::Rect frame = {p, panel->frame().size};
                    // Check if any part of the window would be offscreen
                    if (Intersection(_opts.win->bounds(), frame) != frame) break;
                    panel->setPosition(p);
                    _panels.push_back(panel);
                    offY += panel->frame().size.y + 1;
                
                } else {
                    skip--;
                }
                
                commit = commit.parent();
            }
        }
        
        // Create our undo/redo buttons
        if (_opts.rev.isMutable()) {
            constexpr int UndoWidth      = 8;
            constexpr int RedoWidth      = 8;
            constexpr int SnapshotsWidth = 16;
            
            Rect undoFrame = {_opts.offset+Size{0, _ButtonsInsetY}, {UndoWidth,3}};
            Rect redoFrame = {_opts.offset+Size{UndoWidth, _ButtonsInsetY}, {RedoWidth,3}};
            Rect snapshotsFrame = {_opts.offset+Size{(_opts.width-SnapshotsWidth), _ButtonsInsetY}, {SnapshotsWidth,3}};
            
            {
                auto button = _buttons.emplace_back(MakeShared<UI::Button>());
                UI::ButtonOptions& opts = button->options();
                opts.colors = _opts.colors;
                opts.label = "Undo";
                opts.enabled = _opts.undoEnabled;
                opts.center = true;
                opts.drawBorder = true;
                opts.insetX = 1;
                opts.frame = undoFrame;
            }
            
            {
                auto button = _buttons.emplace_back(MakeShared<UI::Button>());
                UI::ButtonOptions& opts = button->options();
                opts.colors = _opts.colors;
                opts.label = "Redo";
                opts.enabled = _opts.redoEnabled;
                opts.center = true;
                opts.drawBorder = true;
                opts.insetX = 1;
                opts.frame = redoFrame;
            }
            
            {
                auto button = _buttons.emplace_back(MakeShared<UI::Button>());
                UI::ButtonOptions& opts = button->options();
                opts.colors = _opts.colors;
                opts.label = "Snapshots…";
                opts.enabled = _opts.snapshotsEnabled;
                opts.center = true;
                opts.drawBorder = true;
                opts.insetX = 1;
                opts.frame = snapshotsFrame;
            }
        }
    }
    
    void draw() {
        // Don't draw anything if our column is truncated
        if (_truncated) return;
        
        // Draw branch name
        {
            UI::Attr attr(_opts.win, A_UNDERLINE);
            const Point p = _opts.offset + Size{(_opts.width-(int)UTF8::Strlen(_name))/2, _TitleInsetY};
            _opts.win->drawText(p, "%s", _name.c_str());
        }
        
        if (!_opts.rev.isMutable()) {
            UI::Attr attr(_opts.win, _opts.colors.error);
            const char immutableText[] = "read-only";
            const Point p = _opts.offset + Size{std::max(0, (_opts.width-(int)(std::size(immutableText)-1))/2), _ReadonlyInsetY};
            _opts.win->drawText(p, "%s", immutableText);
        }
        
        for (UI::CommitPanel p : _panels) {
            p->drawIfNeeded();
        }
        
        for (UI::Button button : _buttons) {
            button->draw(_opts.win);
        }
    }
    
//    void updateMouse(const Point& p) {
//        for (UI::Button& button : _buttons) {
//            button.updateMouse(p);
//        }
//    }
    
    RevColumnHitTestResult updateMouse(const UI::Point& p) {
        for (UI::Button button : _buttons) {
            button->options().highlight = false;
        }
        
//        UI::Button* hitButton = nullptr;
//        int hitButtonNum = (int)RevColumnButton::None+1;
//        for (UI::Button& button : _buttons) {
//            bool hit = button.hitTest(p);
//            if (hit && !hitButton) {
//                button.highlight(true);
//                hitButton = &button;
//            } else {
//                button.highlight(false);
//            }
//            
//            if (!hitButton) hitButtonNum++;
//        }
        
        for (UI::CommitPanel panel : _panels) {
            if (panel->hitTest(p)) return RevColumnHitTestResult{ .panel = panel };
        }
        
        int bnum = (int)RevColumnButton::None+1;
        for (UI::Button& button : _buttons) {
            if (button->hitTest(p)) {
                button->options().highlight = true;
                return RevColumnHitTestResult{
                    .button = (RevColumnButton)bnum,
                    .buttonEnabled = button->options().enabled,
                };
            }
            bnum++;
        }
        
        
//        if (hitButton) {
//            return RevColumnHitTestResult{
//                .button = (RevColumnButton)hitButtonNum,
//                .buttonEnabled = hitButton->opts().enabled,
//            };
//        }
        return {};
    }
    
    Git::Rev rev() { return _opts.rev; }
    UI::CommitPanelVec& panels() { return _panels; }
    const RevColumnOptions& opts() { return _opts; }
    
private:
    static constexpr int _ButtonsInsetY  = 0;
    static constexpr int _ReadonlyInsetY = 1;
    static constexpr int _TitleInsetY    = 3;
    static constexpr int _CommitsInsetY  = 5;
    static constexpr int _ButtonWidth    = 8;
    RevColumnOptions _opts;
    std::string _name;
    bool _truncated = false;
    UI::CommitPanelVec _panels;
    std::vector<UI::Button> _buttons;
//    UI::Button* _undoButton = nullptr;
//    UI::Button* _redoButton = nullptr;
//    UI::Button* _snapshotsButton = nullptr;
//    std::optional<UI::Button> _undoButton;
//    std::optional<UI::Button> _redoButton;
//    std::optional<UI::Button> _snapshotsButton;
};

using RevColumn = std::shared_ptr<_RevColumn>;

} // namespace UI
