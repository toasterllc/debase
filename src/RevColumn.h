#pragma once
#include "Git.h"
#include "Panel.h"
#include "CommitPanel.h"
#include "MakeShared.h"
#include "Color.h"
#include "UTF8.h"

namespace UI {

// RevColumn: a column in the UI containing commits (of type CommitPanel)
// for a particular `Git::Rev` (commit/branch/tag)
class RevColumn {
public:
    enum class Button : int {
        None,
        Undo,
        Redo,
        Snapshots,
    };

    struct HitTestResult {
        CommitPanelPtr panel;
        Button button = Button::None;
        bool buttonEnabled = false;
        
        operator bool() const {
            return panel || button!=Button::None;
        }
        
        bool operator==(const HitTestResult& x) const {
            if (panel != x.panel) return false;
            if (button != x.button) return false;
            if (buttonEnabled != x.buttonEnabled) return false;
            return true;
        }
        
        bool operator!=(const HitTestResult& x) const {
            return !(*this==x);
        }
    };
    
    RevColumn(const ColorPalette& colors) : colors(colors) {}
    
    void layout() {
        // Short-circuit if layout isn't needed
        if (!_layoutNeeded) return;
        _layoutNeeded = false;
        
        // Set our column name
        {
            _name = rev.displayName();
            
            if (head && _name!="HEAD") {
                _name = _name + " (HEAD)";
            }
            
            // Truncate the name to our width
            _name.erase(std::min(UTF8::Strlen(_name), (size_t)width));
        }
        
        // Create our CommitPanels
        UI::Rect nameFrame = {offset, {width, 1}};
        _truncated = Intersection(containerBounds, nameFrame) != nameFrame;
        if (!_truncated) {
            // Create panels for each commit
            int offY = _CommitsInsetY;
            Git::Commit commit = rev.commit;
            size_t skip = rev.skip;
            while (commit) {
                if (!skip) {
                    Point p = offset + Size{0,offY};
                    UI::CommitPanelPtr panel = MakeShared<UI::CommitPanelPtr>(colors, false, width, commit);
                    UI::Rect frame = {p, panel->frame().size};
                    // Check if any part of the window would be offscreen
                    if (Intersection(containerBounds, frame) != frame) break;
                    panel->setPosition(p);
                    panels.push_back(panel);
                    offY += panel->frame().size.y + _CommitSpacing;
                
                } else {
                    skip--;
                }
                
                commit = commit.parent();
            }
        }
        
        // Create our undo/redo buttons
        if (rev.isMutable()) {
            constexpr int UndoWidth      = 8;
            constexpr int RedoWidth      = 8;
            constexpr int SnapshotsWidth = 16;
            
            Rect undoFrame = {offset+Size{0, _ButtonsInsetY}, {UndoWidth,3}};
            Rect redoFrame = {offset+Size{UndoWidth, _ButtonsInsetY}, {RedoWidth,3}};
            Rect snapshotsFrame = {offset+Size{(width-SnapshotsWidth), _ButtonsInsetY}, {SnapshotsWidth,3}};
            
            {
                auto button = _buttons.emplace_back(std::make_shared<UI::Button>(colors));
                button->label = "Undo";
                button->enabled = undoEnabled;
                button->center = true;
                button->drawBorder = true;
                button->insetX = 1;
                button->frame = undoFrame;
            }
            
            {
                auto button = _buttons.emplace_back(std::make_shared<UI::Button>(colors));
                button->label = "Redo";
                button->enabled = redoEnabled;
                button->center = true;
                button->drawBorder = true;
                button->insetX = 1;
                button->frame = redoFrame;
            }
            
            {
                auto button = _buttons.emplace_back(std::make_shared<UI::Button>(colors));
                button->label = "Snapshots…";
                button->enabled = snapshotsEnabled;
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
            const Point p = offset + Size{(width-(int)UTF8::Strlen(_name))/2, _TitleInsetY};
            win.drawText(p, "%s", _name.c_str());
        }
        
        if (!rev.isMutable()) {
            UI::Window::Attr color = win.attr(colors.error);
            const char immutableText[] = "read-only";
            const Point p = offset + Size{std::max(0, (width-(int)(std::size(immutableText)-1))/2), _ReadonlyInsetY};
            win.drawText(p, "%s", immutableText);
        }
        
        for (UI::CommitPanelPtr p : panels) {
            p->draw();
        }
        
        for (UI::ButtonPtr button : _buttons) {
            button->draw(win);
        }
    }
    
    HitTestResult updateMouse(const UI::Point& p) {
        for (UI::ButtonPtr button : _buttons) {
            button->highlight = false;
        }
        
        for (UI::CommitPanelPtr panel : panels) {
            if (panel->hitTest(p)) return HitTestResult{ .panel = panel };
        }
        
        int bnum = (int)Button::None+1;
        for (UI::ButtonPtr& button : _buttons) {
            if (button->hitTest(p)) {
                button->highlight = true;
                return HitTestResult{
                    .button = (Button)bnum,
                    .buttonEnabled = button->enabled,
                };
            }
            bnum++;
        }
        
        return {};
    }
    
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
    
    UI::CommitPanelVec panels;
    
private:
    static constexpr int _TitleInsetY    = 0;
    static constexpr int _ReadonlyInsetY = 2;
    static constexpr int _ButtonsInsetY  = 1;
    static constexpr int _CommitsInsetY  = 5;
    static constexpr int _ButtonWidth    = 8;
    static constexpr int _CommitSpacing  = 1;
    
    bool _layoutNeeded = true;
    std::string _name;
    bool _truncated = false;
    std::vector<UI::ButtonPtr> _buttons;
};

using RevColumnPtr = std::shared_ptr<RevColumn>;

} // namespace UI
