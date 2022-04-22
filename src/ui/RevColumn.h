#pragma once
#include "git/Git.h"
#include "Panel.h"
#include "CommitPanel.h"
#include "Color.h"
#include "UTF8.h"
#include "Button.h"
#include "View.h"

namespace UI {

// RevColumn: a column in the UI containing commits (of type CommitPanel)
// for a particular `Git::Rev` (commit/branch/tag)
class RevColumn : public View {
public:
    RevColumn() {
        
        _undoButton->label()->text("Undo");
        _undoButton->bordered(true);
        
        _redoButton->label()->text("Redo");
        _redoButton->bordered(true);
        
        _snapshotsButton->label()->text("Snapshots…");
        _snapshotsButton->bordered(true);
        _snapshotsButton->actionTrigger(Button::ActionTrigger::MouseDown);
        
        _name->textAttr(colors().menu | WA_BOLD);
        _name->align(Align::Center);
//        _name->visible(false);
        
        _readOnly->text("read-only");
        _readOnly->align(Align::Center);
        _readOnly->textAttr(colors().error);
        
//        borderColor(colors().normal);
    }
    
    void reload(Size size) {
        // Set our column name
        _name->text(_rev.displayName());
        
        if (_head && _rev.displayName()!="HEAD") {
            _name->suffix(" (HEAD)");
        } else {
            _name->suffix("");
        }
        
        _readOnly->visible(!_rev.isMutable());
        
        _undoButton->visible(_rev.isMutable());
        _redoButton->visible(_rev.isMutable());
        _snapshotsButton->visible(_rev.isMutable());
        
//        _redoButton->visible(false);
//        _snapshotsButton->visible(false);
        
        // Create our CommitPanels for each commit
        Git::Commit commit = _rev.commit;
        size_t skip = _rev.skip;
        size_t i = 0;
        int offY = _CommitsInsetY;
        while (commit) {
            if (!skip) {
                CommitPanelPtr panel = (i<_panels.size() ? _panels[i] : nullptr);
                
                // Create the panel if it doesn't already exist, or if it does but contains the wrong commit
                if (!panel || panel->commit()!=commit) {
                    panel = subviewCreate<CommitPanel>();
                    panel->commit(commit);
                    _panels.insert(_panels.begin()+i, panel);
                }
                
                const Size panelSize = panel->sizeIntrinsic({size.x, ConstraintNone});
                const int rem = size.y-offY;
                if (panelSize.y > rem) break;
                
                offY += panelSize.y + _CommitSpacing;
                i++;
            
            } else {
                skip--;
            }
            
            commit = commit.parent();
        }
        
        // Erase excess panels (ones that extend beyond visible region)
        _panels.erase(_panels.begin()+i, _panels.end());
        
        layoutNeeded(true);
    }
    
    void layout() override {
        constexpr int UndoWidth      = 8;
        constexpr int RedoWidth      = 8;
        constexpr int SnapshotsWidth = 16;
            
        const Size s = size();
        
        _name->frame({{0,_NameInsetY}, {s.x, 1}});
        _readOnly->frame({{0,_ReadonlyInsetY}, {s.x, 1}});
        
        _undoButton->frame({{0, _ButtonsInsetY}, {UndoWidth,3}});
        _redoButton->frame({{UndoWidth, _ButtonsInsetY}, {RedoWidth,3}});
        _snapshotsButton->frame({{(s.x-SnapshotsWidth), _ButtonsInsetY}, {SnapshotsWidth,3}});
        
        int offY = _CommitsInsetY;
        for (CommitPanelPtr panel : _panels) {
            const Size ps = panel->sizeIntrinsic({s.x, ConstraintNone});
            const Rect pf = {{0,offY}, ps};
            panel->frame(pf);
            offY += ps.y + _CommitSpacing;
        }
    }
    
//    bool drawNeeded() const override {
//        if (View::drawNeeded()) return true;
//        
//        for (CommitPanelPtr p : _panels) {
//            if (p->drawNeeded()) return true;
//        }
//        
//        return false;
//    }
//    
//    void draw() override {
//        const Point pos = origin();
//        const int width = size().x;
//        // Draw branch name
//        if (win.erased()) {
//            Attr color = attr(colors().menu);
//            Attr bold = attr(WA_BOLD);
//            const Point p = pos + Size{(width-(int)UTF8::Len(_name))/2, _TitleInsetY};
//            drawText(p, _name.c_str());
//        }
//        
//        if (!_rev.isMutable()) {
//            Attr color = attr(colors().error);
//            const char immutableText[] = "read-only";
//            const Point p = pos + Size{std::max(0, (width-(int)(std::size(immutableText)-1))/2), _ReadonlyInsetY};
//            drawText(p, immutableText);
//        }
//        
//        for (CommitPanelPtr p : _panels) {
//            p->draw(win);
//        }
//    }
    
//    void draw() override {
//        drawRect();
//    }
    
    CommitPanelPtr hitTestCommit(const Point& p) {
        for (CommitPanelPtr panel : _panels) {
            if (HitTest(panel->frame(), p)) return panel;
        }
        return nullptr;
    }
    
    const auto& repo() const { return _repo; }
    template <typename T> bool repo(const T& x) { return _set(_repo, x); }
    
    const auto& rev() const { return _rev; }
    template <typename T> bool rev(const T& x) { return _set(_rev, x); }
    
    const auto& head() const { return _head; }
    template <typename T> bool head(const T& x) { return _set(_head, x); }
    
    const auto& panels() const { return _panels; }
    template <typename T> bool panels(const T& x) { return _set(_panels, x); }
    
    const auto& undoButton() const { return _undoButton; }
    template <typename T> bool undoButton(const T& x) { return _set(_undoButton, x); }
    
    const auto& redoButton() const { return _redoButton; }
    template <typename T> bool redoButton(const T& x) { return _set(_redoButton, x); }
    
    const auto& snapshotsButton() const { return _snapshotsButton; }
    template <typename T> bool snapshotsButton(const T& x) { return _set(_snapshotsButton, x); }
    
private:
    static constexpr int _NameInsetY    = 0;
    static constexpr int _ReadonlyInsetY = 2;
    static constexpr int _ButtonsInsetY  = 1;
    static constexpr int _CommitsInsetY  = 5;
    static constexpr int _CommitSpacing  = 1;
    
    Git::Repo _repo;
    Git::Rev _rev;
    bool _head = false;
    CommitPanelVec _panels;
    
    LabelPtr _name              = subviewCreate<Label>();
    LabelPtr _readOnly          = subviewCreate<Label>();
    
    ButtonPtr _undoButton       = subviewCreate<Button>();
    ButtonPtr _redoButton       = subviewCreate<Button>();
    ButtonPtr _snapshotsButton  = subviewCreate<Button>();
};

using RevColumnPtr = std::shared_ptr<RevColumn>;

} // namespace UI
