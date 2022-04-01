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
        Rect nameFrame = {offset, {width, 1}};
        _truncated = Intersection(containerBounds, nameFrame) != nameFrame;
        if (!_truncated) {
            // Create panels for each commit
            int offY = _CommitsInsetY;
            Git::Commit commit = rev.commit;
            size_t skip = rev.skip;
            while (commit) {
                if (!skip) {
                    Point p = offset + Size{0,offY};
                    CommitPanelPtr panel = MakeShared<CommitPanelPtr>(colors, false, width, commit);
                    Rect frame = {p, panel->frame().size};
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
                button->enabled = (bool)undoAction;
                button->center = true;
                button->drawBorder = true;
                button->insetX = 1;
                button->frame = undoFrame;
                if (undoAction) {
                    button->action = [&] (UI::Button&) { undoAction(*this); };
                }
            }
            
            {
                auto button = _buttons.emplace_back(std::make_shared<UI::Button>(colors));
                button->label = "Redo";
                button->enabled = (bool)redoAction;
                button->center = true;
                button->drawBorder = true;
                button->insetX = 1;
                button->frame = redoFrame;
                if (redoAction) {
                    button->action = [&] (UI::Button&) { redoAction(*this); };
                }
            }
            
            {
                auto button = _buttons.emplace_back(std::make_shared<UI::Button>(colors));
                button->label = "Snapshots…";
                button->enabled = (bool)snapshotsAction;
                button->center = true;
                button->drawBorder = true;
                button->insetX = 1;
                button->frame = snapshotsFrame;
                if (snapshotsAction) {
                    button->action = [&] (UI::Button&) { snapshotsAction(*this); };
                }
            }
        }
    }
    
    void draw(const Window& win) {
        // Don't draw anything if our column is truncated
        if (_truncated) return;
        
        // Draw branch name
        {
            Window::Attr bold = win.attr(A_BOLD);
            const Point p = offset + Size{(width-(int)UTF8::Strlen(_name))/2, _TitleInsetY};
            win.drawText(p, "%s", _name.c_str());
        }
        
        if (!rev.isMutable()) {
            Window::Attr color = win.attr(colors.error);
            const char immutableText[] = "read-only";
            const Point p = offset + Size{std::max(0, (width-(int)(std::size(immutableText)-1))/2), _ReadonlyInsetY};
            win.drawText(p, "%s", immutableText);
        }
        
        for (CommitPanelPtr p : panels) {
            p->draw();
        }
        
        for (ButtonPtr button : _buttons) {
            button->draw(win);
        }
    }
    
    Event handleEvent(const Window& win, const Event& ev) {
        Event e = ev;
        for (ButtonPtr button : _buttons) {
            e = button->handleEvent(win, ev);
            if (!e) return {};
        }
        return e;
    }
    
    CommitPanelPtr hitTest(const Point& p) {
        for (CommitPanelPtr panel : panels) {
            if (panel->hitTest(p)) return panel;
        }
        return nullptr;
    }
    
    const ColorPalette& colors;
    Rect containerBounds;
    Git::Repo repo;
    Git::Rev rev;
    bool head = false;
    Point offset;
    int width = 0;
    std::function<void(RevColumn&)> undoAction;
    std::function<void(RevColumn&)> redoAction;
    std::function<void(RevColumn&)> snapshotsAction;
    CommitPanelVec panels;
    
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
    std::vector<ButtonPtr> _buttons;
};

using RevColumnPtr = std::shared_ptr<RevColumn>;

} // namespace UI
