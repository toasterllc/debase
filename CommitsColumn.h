#pragma once
#include "Git.h"
#include "Panel.h"
#include "CommitPanel.h"

// CommitsColumn: a column in the UI containing commits (of type CommitPanel)
// for a particular git branch or ref
class CommitsColumn {
public:
    CommitsColumn(Window& win, Git::Repo repo, Git::Reference ref, int offsetX, int width) :
    _win(win), _ref(ref), _name(_ref.name()), _displayName(_ref.name()), _offsetX(offsetX), _width(width) {
        if (_ref.isHead()) {
            _displayName = _displayName + " (HEAD)";
        }
        
        // Create panels for each commit
        constexpr int InsetY = 2;
        size_t idx = 0;
        int offY = InsetY;
        Git::RevWalk walk = Git::RevWalk::Create(repo, _name+"~10.."+_name);
        while (Git::Commit commit = walk.next()) {
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
            const int offX = _offsetX + std::max(0, (_width-(int)_displayName.size())/2);
            _win.drawText({offX, 0}, _displayName.c_str());
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
    
    Git::Reference ref() { return _ref; }
    CommitPanelVec& panels() { return _panels; }
    
private:
    Window& _win;
    Git::Reference _ref;
    std::string _name;
    std::string _displayName;
    int _offsetX = 0;
    int _width = 0;
    CommitPanelVec _panels;
};
