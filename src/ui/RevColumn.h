#pragma once
#include "git/Git.h"
#include "Panel.h"
#include "CommitPanel.h"
#include "Color.h"
#include "UTF8.h"
#include "Button.h"
#include "View.h"

#include <os/log.h>

namespace UI {

// RevColumn: a column in the UI containing commits (of type CommitPanel)
// for a particular `Git::Rev` (commit/branch/tag)
class RevColumn : public View {
public:
    RevColumn(const ColorPalette& colors) :
    View(colors), undoButton(colors), redoButton(colors), snapshotsButton(colors) {
        
        undoButton.label().text("Undo");
        undoButton.drawBorder(true);
        
        redoButton.label().text("Redo");
        redoButton.drawBorder(true);
        
        snapshotsButton.label().text("Snapshots…");
        snapshotsButton.drawBorder(true);
        snapshotsButton.actionTrigger(Button::ActionTrigger::MouseDown);
    }
    
    bool layout(const Window& win) override {
        if (!View::layout(win)) return false;
        const Point pos = origin();
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
        int offY = _CommitsInsetY;
        Git::Commit commit = rev.commit;
        size_t skip = rev.skip;
        size_t i = 0;
        while (commit) {
            if (!skip) {
                CommitPanelPtr panel = (i<panels.size() ? panels[i] : nullptr);
                
                // Create the panel if it doesn't already exist, or if it does but contains the wrong commit
                if (!panel || panel->commit()!=commit) {
                    panel = std::make_shared<CommitPanel>(colors(), false, width, commit);
                    panels.insert(panels.begin()+i, panel);
                }
                
                const Rect f = {pos + Size{0,offY}, panel->size()};
                if (f.ymax() > frame().ymax()) break;
                panel->origin(f.origin);
                
                offY += panel->size().y + _CommitSpacing;
                i++;
            
            } else {
                skip--;
            }
            
            commit = commit.parent();
        }
        
        // Erase excess panels (ones that extend beyond visible region)
        panels.erase(panels.begin()+i, panels.end());
        
        constexpr int UndoWidth      = 8;
        constexpr int RedoWidth      = 8;
        constexpr int SnapshotsWidth = 16;
        
        Rect undoFrame = {pos+Size{0, _ButtonsInsetY}, {UndoWidth,3}};
        Rect redoFrame = {pos+Size{UndoWidth, _ButtonsInsetY}, {RedoWidth,3}};
        Rect snapshotsFrame = {pos+Size{(width-SnapshotsWidth), _ButtonsInsetY}, {SnapshotsWidth,3}};
        
        undoButton.frame(undoFrame);
        redoButton.frame(redoFrame);
        snapshotsButton.frame(snapshotsFrame);
        
        undoButton.layout(win);
        redoButton.layout(win);
        snapshotsButton.layout(win);
        
        return true;
    }
    
    bool drawNeeded() const override {
        if (View::drawNeeded()) return true;
        if (_showButtons()) {
            if (undoButton.drawNeeded()) return true;
            if (redoButton.drawNeeded()) return true;
            if (snapshotsButton.drawNeeded()) return true;
        }
        
        for (CommitPanelPtr p : panels) {
            if (p->drawNeeded()) return true;
        }
        
        return false;
    }
    
    bool draw(const Window& win) override {
        if (!View::draw(win)) return false;
        const Point pos = origin();
        const int width = size().x;
        
        // Draw branch name
        if (win.erased()) {
            Window::Attr color = win.attr(colors().menu);
            Window::Attr bold = win.attr(A_BOLD);
            const Point p = pos + Size{(width-(int)UTF8::Strlen(_name))/2, _TitleInsetY};
            win.drawText(p, _name.c_str());
        }
        
        if (!rev.isMutable()) {
            Window::Attr color = win.attr(colors().error);
            const char immutableText[] = "read-only";
            const Point p = pos + Size{std::max(0, (width-(int)(std::size(immutableText)-1))/2), _ReadonlyInsetY};
            win.drawText(p, immutableText);
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
