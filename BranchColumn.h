#pragma once
#include "Git.h"
#include "Panel.h"
#include "CommitPanel.h"

// BranchColumn: a column in the UI containing commits (of type CommitPanel) for a particular git branch
class BranchColumn {
public:
    BranchColumn(Window& win, Repo repo, std::string_view name, int offsetX, int width) :
    _win(win), _name(name), _offsetX(offsetX), _width(width) {
        // Create panels for each commit
        RevWalk walk = RevWalkCreate(repo);
        
        std::string revrangeStr = (_name+"~10.."+_name);
        
        int ir = git_revwalk_push_range(*walk, revrangeStr.c_str());
        if (ir) throw RuntimeError("git_revwalk_push_range failed: %s", git_error_last()->message);
        
        constexpr int InsetY = 2;
        size_t idx = 0;
        int offY = InsetY;
        git_oid oid;
        while (!git_revwalk_next(&oid, *walk)) {
            Commit commit = CommitCreate(repo, oid);
            CommitPanel& p = _panels.emplace_back(commit, idx, width);
            p.setPosition({_offsetX, offY});
            offY += p.rect().size.y + 1;
            idx++;
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
    
    CommitPanelSet selection() {
        CommitPanelSet s;
        for (CommitPanel& p : _panels) {
            if (p.selected()) s.insert(&p);
        }
        return s;
    }
    
    CommitPanelVector& panels() { return _panels; }
    
private:
    Window& _win;
    std::string _name;
    int _offsetX = 0;
    int _width = 0;
    CommitPanelVector _panels;
};
