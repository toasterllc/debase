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
    RevColumn() {
        
        _undoButton->label()->text("Undo");
        _undoButton->drawBorder(true);
        
        _redoButton->label()->text("Redo");
        _redoButton->drawBorder(true);
        
        _snapshotsButton->label()->text("Snapshots…");
        _snapshotsButton->drawBorder(true);
        _snapshotsButton->actionTrigger(Button::ActionTrigger::MouseDown);
    }
    
    void layout(const Window& win) override {
        const Point pos = origin();
        const int width = size().x;
        
        // Set our column name
        {
            _name = _rev.displayName();
            
            if (_head && _name!="HEAD") {
                _name = _name + " (HEAD)";
            }
            
            // Truncate the name to our width
            _name.erase(std::min(UTF8::Strlen(_name), (size_t)width));
        }
        
        // Create our CommitPanels for each commit
        int offY = _CommitsInsetY;
        Git::Commit commit = _rev.commit;
        size_t skip = _rev.skip;
        size_t i = 0;
        while (commit) {
            if (!skip) {
                CommitPanelPtr panel = (i<_panels.size() ? _panels[i] : nullptr);
                
                // Create the panel if it doesn't already exist, or if it does but contains the wrong commit
                if (!panel || panel->commit()!=commit) {
                    panel = std::make_shared<CommitPanel>(false, width, commit);
                    _panels.insert(_panels.begin()+i, panel);
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
        _panels.erase(_panels.begin()+i, _panels.end());
        
        constexpr int UndoWidth      = 8;
        constexpr int RedoWidth      = 8;
        constexpr int SnapshotsWidth = 16;
        
        Rect undoFrame = {pos+Size{0, _ButtonsInsetY}, {UndoWidth,3}};
        Rect redoFrame = {pos+Size{UndoWidth, _ButtonsInsetY}, {RedoWidth,3}};
        Rect snapshotsFrame = {pos+Size{(width-SnapshotsWidth), _ButtonsInsetY}, {SnapshotsWidth,3}};
        
        _undoButton->frame(undoFrame);
        _redoButton->frame(redoFrame);
        _snapshotsButton->frame(snapshotsFrame);
        
        _undoButton->visible(_rev.isMutable());
        _redoButton->visible(_rev.isMutable());
        _snapshotsButton->visible(_rev.isMutable());
        
//        undoButton->layout(win);
//        redoButton->layout(win);
//        snapshotsButton->layout(win);
    }
    
    bool drawNeeded() const override {
        if (View::drawNeeded()) return true;
        
        for (CommitPanelPtr p : _panels) {
            if (p->drawNeeded()) return true;
        }
        
        return false;
    }
    
    void draw(const Window& win) override {
        const Point pos = origin();
        const int width = size().x;
        // Draw branch name
        if (win.erased()) {
            Window::Attr color = win.attr(Colors().menu);
            Window::Attr bold = win.attr(A_BOLD);
            const Point p = pos + Size{(width-(int)UTF8::Strlen(_name))/2, _TitleInsetY};
            win.drawText(p, _name.c_str());
        }
        
        if (!_rev.isMutable()) {
            Window::Attr color = win.attr(Colors().error);
            const char immutableText[] = "read-only";
            const Point p = pos + Size{std::max(0, (width-(int)(std::size(immutableText)-1))/2), _ReadonlyInsetY};
            win.drawText(p, immutableText);
        }
        
        for (CommitPanelPtr p : _panels) {
            p->drawTree(win);
        }
    }
    
    CommitPanelPtr hitTest(const Point& p) {
        for (CommitPanelPtr panel : _panels) {
            if (panel->hitTest(p)) return panel;
        }
        return nullptr;
    }
    
    const auto& repo() const { return _repo; }
    template <typename T> void repo(const T& x) { _set(_repo, x); }
    
    const auto& rev() const { return _rev; }
    template <typename T> void rev(const T& x) { _set(_rev, x); }
    
    const auto& head() const { return _head; }
    template <typename T> void head(const T& x) { _set(_head, x); }
    
    const auto& panels() const { return _panels; }
    template <typename T> void panels(const T& x) { _set(_panels, x); }
    
    const auto& undoButton() const { return _undoButton; }
    template <typename T> void undoButton(const T& x) { _set(_undoButton, x); }
    
    const auto& redoButton() const { return _redoButton; }
    template <typename T> void redoButton(const T& x) { _set(_redoButton, x); }
    
    const auto& snapshotsButton() const { return _snapshotsButton; }
    template <typename T> void snapshotsButton(const T& x) { _set(_snapshotsButton, x); }
    
private:
    static constexpr int _TitleInsetY    = 0;
    static constexpr int _ReadonlyInsetY = 2;
    static constexpr int _ButtonsInsetY  = 1;
    static constexpr int _CommitsInsetY  = 5;
    static constexpr int _CommitSpacing  = 1;
    
    Git::Repo _repo;
    Git::Rev _rev;
    bool _head = false;
    CommitPanelVec _panels;
    
    ButtonPtr _undoButton       = createSubview<Button>();
    ButtonPtr _redoButton       = createSubview<Button>();
    ButtonPtr _snapshotsButton  = createSubview<Button>();
    
    std::string _name;
};

using RevColumnPtr = std::shared_ptr<RevColumn>;

} // namespace UI
