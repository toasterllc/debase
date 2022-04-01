#pragma once
#include "Git.h"
#include "Panel.h"
#include "CommitPanel.h"
#include "MakeShared.h"
#include "Color.h"
#include "UTF8.h"

namespace UI {

struct RevColumnOptions {
    const ColorPalette& colors;
    Rect containerBounds;
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
        _truncated = Intersection(_opts.containerBounds, nameFrame) != nameFrame;
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
                    if (Intersection(_opts.containerBounds, frame) != frame) break;
                    panel->setPosition(p);
                    _panels.push_back(panel);
                    offY += panel->frame().size.y + _CommitSpacing;
                
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
                auto button = _buttons.emplace_back(std::make_shared<UI::Button>(_opts.colors));
                button->label = "Undo";
                button->enabled = _opts.undoEnabled;
                button->center = true;
                button->drawBorder = true;
                button->insetX = 1;
                button->frame = undoFrame;
            }
            
            {
                auto button = _buttons.emplace_back(std::make_shared<UI::Button>(_opts.colors));
                button->label = "Redo";
                button->enabled = _opts.redoEnabled;
                button->center = true;
                button->drawBorder = true;
                button->insetX = 1;
                button->frame = redoFrame;
            }
            
            {
                auto button = _buttons.emplace_back(std::make_shared<UI::Button>(_opts.colors));
                button->label = "Snapshots…";
                button->enabled = _opts.snapshotsEnabled;
                button->center = true;
                button->drawBorder = true;
                button->insetX = 1;
                button->frame = snapshotsFrame;
            }
        }
    }
    
    void draw(const Window& win) {
        // Don't draw anything if our column is truncated
        if (_truncated) return;
        
        // Draw branch name
        {
            UI::Window::Attr bold = win.attr(A_BOLD);
            const Point p = _opts.offset + Size{(_opts.width-(int)UTF8::Strlen(_name))/2, _TitleInsetY};
            win.drawText(p, "%s", _name.c_str());
        }
        
        if (!_opts.rev.isMutable()) {
            UI::Window::Attr color = win.attr(_opts.colors.error);
            const char immutableText[] = "read-only";
            const Point p = _opts.offset + Size{std::max(0, (_opts.width-(int)(std::size(immutableText)-1))/2), _ReadonlyInsetY};
            win.drawText(p, "%s", immutableText);
        }
        
        for (UI::CommitPanel p : _panels) {
            p->drawIfNeeded();
        }
        
        for (UI::ButtonPtr button : _buttons) {
            button->draw(win);
        }
    }
    
    RevColumnHitTestResult updateMouse(const UI::Point& p) {
        for (UI::ButtonPtr button : _buttons) {
            button->highlight = false;
        }
        
        for (UI::CommitPanel panel : _panels) {
            if (panel->hitTest(p)) return RevColumnHitTestResult{ .panel = panel };
        }
        
        int bnum = (int)RevColumnButton::None+1;
        for (UI::ButtonPtr& button : _buttons) {
            if (button->hitTest(p)) {
                button->highlight = true;
                return RevColumnHitTestResult{
                    .button = (RevColumnButton)bnum,
                    .buttonEnabled = button->enabled,
                };
            }
            bnum++;
        }
        
        return {};
    }
    
    Git::Rev rev() { return _opts.rev; }
    UI::CommitPanelVec& panels() { return _panels; }
    const RevColumnOptions& opts() { return _opts; }
    
private:
    static constexpr int _TitleInsetY    = 0;
    static constexpr int _ReadonlyInsetY = 2;
    static constexpr int _ButtonsInsetY  = 1;
    static constexpr int _CommitsInsetY  = 5;
    static constexpr int _ButtonWidth    = 8;
    static constexpr int _CommitSpacing  = 1;
    RevColumnOptions _opts;
    std::string _name;
    bool _truncated = false;
    UI::CommitPanelVec _panels;
    std::vector<UI::ButtonPtr> _buttons;
};

using RevColumn = std::shared_ptr<_RevColumn>;

} // namespace UI
