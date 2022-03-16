#pragma once
#include "Git.h"
#include "Panel.h"
#include "CommitPanel.h"
#include "MakeShared.h"
#include "Color.h"

namespace UI {

// RevColumn: a column in the UI containing commits (of type CommitPanel)
// for a particular `Git::Rev` (commit/branch/tag)
class _RevColumn {
public:
    _RevColumn(const ColorPalette& colors, UI::Window win, Git::Repo repo, Git::Rev rev, int offsetX, int width, bool showMutability=false) :
    _colors(colors), _win(win), _rev(rev), _offsetX(offsetX), _width(width), _showMutability(showMutability) {
        
        if (_rev.ref) {
            _name = _rev.ref.name();
        } else {
            _name = Git::StringForId(_rev.commit.id());
        }
        
        bool isHead = repo.head().commit() == _rev.commit;
        if (_name!="HEAD" && isHead) {
            _name = _name + " (HEAD)";
        }
        
        // Create panels for each commit
        const int InsetY = (!_showMutability ? 2 : 3);
        int offY = InsetY;
        Git::Commit commit = _rev.commit;
        while (commit) {
            Point p = {_offsetX, offY};
            UI::CommitPanel panel = MakeShared<UI::CommitPanel>(_colors, false, width, commit);
            if (!panel->validPosition(p)) break;
            panel->setPosition(p);
            _panels.push_back(panel);
            offY += panel->frame().size.y + 1;
            commit = commit.parent();
        }
//        for (int i=0; commit && i<_PanelCount; commit=commit.parent(), i++) {
//            UI::CommitPanel p = MakeShared<UI::CommitPanel>(_colors, i, false, width, commit);
//            _panels.push_back(p);
//        }
    }
    
    void recreate() {
        const int InsetY = (!_showMutability ? 2 : 3);
        int offY = InsetY;
        bool visible = true;
        for (UI::CommitPanel panel : _panels) {
            Point p = {_offsetX, offY};
            visible &= panel->validPosition(p);
            panel->setVisible(visible);
//            panel->setVisible(false);
//            panel->setVisible(true);
            if (visible) {
                panel->setPosition(p);
            }
//            if (visible) {
//                panel->setPosition(p);
//            }
            offY += panel->frame().size.y + 1;
        }
    }
    
    void draw() {
        // Draw branch name
        {
            UI::Attr attr(_win, A_UNDERLINE);
            const int offX = _offsetX + std::max(0, (_width-(int)_name.size())/2);
            _win->drawText({offX, 0}, "%s", _name.c_str());
        }
        
        if (_showMutability) {
            UI::Attr attr(_win, _colors.error);
            const char immutableText[] = "read-only";
            const int offX = _offsetX + std::max(0, (_width-(int)(std::size(immutableText)-1))/2);
            if (!_rev.isMutable()) {
                _win->drawText({offX, 1}, "%s", immutableText);
            }
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
    static constexpr size_t _PanelCount = 30;
    const ColorPalette& _colors;
    UI::Window _win;
    Git::Rev _rev;
    std::string _name;
    int _offsetX = 0;
    int _width = 0;
    bool _showMutability = false;
    UI::CommitPanelVec _panels;
};

using RevColumn = std::shared_ptr<_RevColumn>;

} // namespace UI
