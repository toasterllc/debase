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
    _RevColumn(const ColorPalette& colors, UI::Window win, Git::Repo repo, bool head, Git::Rev rev, int offsetX, int width, bool showMutability=false) :
    _colors(colors), _win(win), _rev(rev), _offsetX(offsetX), _width(width), _showMutability(showMutability) {
        // Set our column name
        {
            _name = _rev.displayName();
            
            if (head) {
                _name = _name + " (HEAD)";
            }
            
            // Truncate the name to our width
            _name.erase(std::min(_name.size(), (size_t)_width));
        }
        
        UI::Rect nameFrame = {{_offsetX,0}, {_width, 1}};
        _truncated = Intersection(_win->bounds(), nameFrame) != nameFrame;
        if (!_truncated) {
            // Create panels for each commit
            const int InsetY = (!_showMutability ? 2 : 3);
            int offY = InsetY;
            Git::Commit commit = _rev.commit;
            size_t skip = _rev.skip;
            while (commit) {
                if (!skip) {
                    Point p = {_offsetX, offY};
                    UI::CommitPanel panel = MakeShared<UI::CommitPanel>(_colors, false, _width, commit);
                    UI::Rect frame = {p, panel->frame().size};
                    // Check if any part of the window would be offscreen
                    if (Intersection(_win->bounds(), frame) != frame) break;
                    panel->setPosition(p);
                    _panels.push_back(panel);
                    offY += panel->frame().size.y + 1;
                
                } else {
                    skip--;
                }
                
                commit = commit.parent();
            }
        }
    }
    
    void draw() {
        // Don't draw anything if our column is truncated
        if (_truncated) return;
        
//        _win->drawLineVert({_offsetX+_width/2, 0}, _win->bounds().size.y);
        
        // Draw branch name
        {
            UI::Attr attr(_win, A_UNDERLINE);
            const int offX = _offsetX + (_width-(int)_name.size())/2;
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
    const ColorPalette& _colors;
    UI::Window _win;
    Git::Rev _rev;
    std::string _name;
    int _offsetX = 0;
    int _width = 0;
    bool _showMutability = false;
    bool _truncated = false;
    UI::CommitPanelVec _panels;
};

using RevColumn = std::shared_ptr<_RevColumn>;

} // namespace UI
