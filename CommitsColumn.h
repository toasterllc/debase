#pragma once
#include "Git.h"
#include "Panel.h"
#include "CommitPanel.h"

// CommitsColumn: a column in the UI containing commits (of type CommitPanel)
// for a particular git branch or ref
class CommitsColumn {
public:
    CommitsColumn(Window& win, Git::Repo repo, Git::Rev rev, int offsetX, int width) :
    _win(win), _rev(rev), _name(_rev.name()), _offsetX(offsetX), _width(width) {
        
        if (_ref.isHead()) {
            _name = _name + " (HEAD)";
        }
        
        // Create panels for each commit
        constexpr int InsetY = 2;
        int offY = InsetY;
        Git::Commit commit = _ref.commit();
        for (int i=0; commit && i<8; i++) {
            CommitPanel& p = _panels.emplace_back(commit, i, width);
            p.setPosition({_offsetX, offY});
            offY += p.rect().size.y + 1;
            commit = commit.parent();
        }
    }
    
    void draw() {
        // Draw branch name
        {
            Window::Attr attr = _win.setAttr(A_UNDERLINE);
            const int offX = _offsetX + std::max(0, (_width-(int)_name.size())/2);
            _win.drawText({offX, 0}, _name.c_str());
        }
        
        for (CommitPanel& p : _panels) {
            p.drawIfNeeded();
        }
    }
    
    CommitPanel* hitTest(const Point& p) {
        for (CommitPanel& panel : _panels) {
            if (panel.hitTest(p)) return &panel;
        }
        return nullptr;
    }
    
    Git::Rev rev() { return _rev; }
    CommitPanelVec& panels() { return _panels; }
    
private:
    Window& _win;
    Git::Rev _rev;
    std::string _name;
    int _offsetX = 0;
    int _width = 0;
    CommitPanelVec _panels;
};
