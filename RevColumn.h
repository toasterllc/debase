#pragma once
#include "Git.h"
#include "Panel.h"
#include "CommitPanel.h"
#include "MakeShared.h"

namespace UI {

// RevColumn: a column in the UI containing commits (of type CommitPanel)
// for a particular `Git::Rev` (commit/branch/tag)
class _RevColumn {
public:
    _RevColumn(UI::Window win, Git::Repo repo, Git::Rev rev, int offsetX, int width) :
    _win(win), _rev(rev), _offsetX(offsetX), _width(width) {
        
        if (_rev.ref) {
            _name = _rev.ref.name();
        } else {
            _name = Git::StringForId(_rev.commit.id());
        }
        
        bool isHead = repo.head().commit() == rev.commit;
        if (_name!="HEAD" && isHead) {
            _name = _name + " (HEAD)";
        }
        
        // Create panels for each commit
        constexpr int InsetY = 2;
        int offY = InsetY;
        Git::Commit commit = rev.commit;
        for (int i=0; commit && i<10; i++) {
            UI::CommitPanel p = MakeShared<UI::CommitPanel>(commit, i, false, width);
            p->setPosition({_offsetX, offY});
            offY += p->rect().size.y + 1;
            _panels.push_back(p);
            commit = commit.parent();
        }
    }
    
    void draw() {
        // Draw branch name
        {
            UI::Attr attr(_win, A_UNDERLINE);
            const int offX = _offsetX + std::max(0, (_width-(int)_name.size())/2);
            _win->drawText({offX, 0}, "%s", _name.c_str());
        }
        
        for (UI::CommitPanel p : _panels) {
            p->drawIfNeeded();
        }
    }
    
    UI::CommitPanel hitTest(const UI::Point& p) {
        for (UI::CommitPanel panel : _panels) {
            if (panel->hitTest(p)) return panel;
        }
        return {};
    }
    
    Git::Rev rev() { return _rev; }
    UI::CommitPanelVec& panels() { return _panels; }
    
private:
    UI::Window _win;
    Git::Rev _rev;
    std::string _name;
    int _offsetX = 0;
    int _width = 0;
    UI::CommitPanelVec _panels;
};

using RevColumn = std::shared_ptr<_RevColumn>;

} // namespace UI
