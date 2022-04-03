#pragma once
#include "Git.h"
#include "Panel.h"
#include "CommitPanel.h"
#include "Color.h"
#include "UTF8.h"
#include "Button.h"
#include "Control.h"

#include <os/log.h>

namespace UI {

// RevColumn: a column in the UI containing commits (of type CommitPanel)
// for a particular `Git::Rev` (commit/branch/tag)
class RevColumn : public Control {
public:
    RevColumn(const ColorPalette& colors) :
    Control(colors), undoButton(colors), redoButton(colors), snapshotsButton(colors) {
    
        undoButton.label = "Undo";
        undoButton.center = true;
        undoButton.drawBorder = true;
        undoButton.insetX = 1;
        
        redoButton.label = "Redo";
        redoButton.center = true;
        redoButton.drawBorder = true;
        redoButton.insetX = 1;
        
        snapshotsButton.label = "Snapshots…";
        snapshotsButton.center = true;
        snapshotsButton.drawBorder = true;
        snapshotsButton.insetX = 1;
        snapshotsButton.actionTrigger = Button::ActionTrigger::MouseDown;
    }
    
    ~RevColumn() {
        os_log(OS_LOG_DEFAULT, "~RevColumn");
    }
    
    bool layout(const Window& win) override {
        if (!Control::layout(win)) return false;
        const Point pos = position();
        const int width = size().x;
        
        // Set our column name
        {
            _name = rev.displayName();
            
            if (head && _name!="HEAD") {
                _name = _name + " (HEAD)";
            }
            
            // Truncate the name to our width
            _name.erase(std::min(UTF8::Strlen(_name), (size_t)width));
        }
        
        // Create our CommitPanels for each commit
        panels.clear();
        int offY = _CommitsInsetY;
        Git::Commit commit = rev.commit;
        size_t skip = rev.skip;
        while (commit) {
            if (!skip) {
                CommitPanelPtr panel = std::make_shared<CommitPanel>(colors, false, width, commit);
                const Rect f = {pos + Size{0,offY}, panel->size()};
                if (f.ymax() > frame().ymax()) break;
                panel->position(f.point);
                panels.push_back(panel);
                offY += panel->size().y + _CommitSpacing;
            
            } else {
                skip--;
            }
            
            commit = commit.parent();
        }
        
        constexpr int UndoWidth      = 8;
        constexpr int RedoWidth      = 8;
        constexpr int SnapshotsWidth = 16;
        
        Rect undoFrame = {pos+Size{0, _ButtonsInsetY}, {UndoWidth,3}};
        Rect redoFrame = {pos+Size{UndoWidth, _ButtonsInsetY}, {RedoWidth,3}};
        Rect snapshotsFrame = {pos+Size{(width-SnapshotsWidth), _ButtonsInsetY}, {SnapshotsWidth,3}};
        
        undoButton.enabled = (bool)undoButton.action;
        redoButton.enabled = (bool)redoButton.action;
        snapshotsButton.enabled = (bool)snapshotsButton.action;
        
        undoButton.frame(undoFrame);
        redoButton.frame(redoFrame);
        snapshotsButton.frame(snapshotsFrame);
        
        return true;
    }
    
    bool drawNeeded() const override {
        if (Control::drawNeeded()) return true;
        if (_showButtons()) {
            if (undoButton.drawNeeded()) return true;
            if (redoButton.drawNeeded()) return true;
            if (snapshotsButton.drawNeeded()) return true;
        }
        return false;
    }
    
    bool draw(const Window& win) override {
        if (!Control::draw(win)) return false;
        
        // Don't draw anything if we don't have any panels
        if (panels.empty()) return true;
        
        const Point pos = position();
        const int width = size().x;
        
        // Draw branch name
        if (win.erased()) {
            Window::Attr bold = win.attr(A_BOLD);
            const Point p = pos + Size{(width-(int)UTF8::Strlen(_name))/2, _TitleInsetY};
            win.drawText(p, "%s", _name.c_str());
        }
        
        if (!rev.isMutable()) {
            Window::Attr color = win.attr(colors.error);
            const char immutableText[] = "read-only";
            const Point p = pos + Size{std::max(0, (width-(int)(std::size(immutableText)-1))/2), _ReadonlyInsetY};
            win.drawText(p, "%s", immutableText);
        }
        
        for (CommitPanelPtr p : panels) {
            p->draw();
        }
        
        if (_showButtons()) {
            undoButton.draw(win);
            redoButton.draw(win);
            snapshotsButton.draw(win);
        }
        
        return true;
    }
    
    bool handleEvent(const Window& win, const Event& ev) override {
        if (_showButtons()) {
            bool handled = false;
            if (!handled) handled = undoButton.handleEvent(win, ev);
            if (!handled) handled = redoButton.handleEvent(win, ev);
            if (!handled) handled = snapshotsButton.handleEvent(win, ev);
            return handled;
        }
        return false;
    }
    
    CommitPanelPtr hitTest(const Point& p) {
        for (CommitPanelPtr panel : panels) {
            if (panel->hitTest(p)) return panel;
        }
        return nullptr;
    }
    
    Git::Repo repo;
    Git::Rev rev;
    bool head = false;
    Button undoButton;
    Button redoButton;
    Button snapshotsButton;
    CommitPanelVec panels;
    
private:
    static constexpr int _TitleInsetY    = 0;
    static constexpr int _ReadonlyInsetY = 2;
    static constexpr int _ButtonsInsetY  = 1;
    static constexpr int _CommitsInsetY  = 5;
    static constexpr int _ButtonWidth    = 8;
    static constexpr int _CommitSpacing  = 1;
    
    bool _showButtons() const {
        return rev.isMutable();
    }
    
    std::string _name;
};

using RevColumnPtr = std::shared_ptr<RevColumn>;

} // namespace UI
