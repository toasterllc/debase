#pragma once
#include "Git.h"
#include "Panel.h"
#include "CommitPanel.h"
#include "MakeShared.h"
#include "Color.h"

namespace UI {

struct RevColumnOptions {
    UI::Window win;
    ColorPalette colors;
    Git::Repo repo;
    Git::Rev rev;
    bool head = false;
    int offsetX = 0;
    int width = 0;
};

// RevColumn: a column in the UI containing commits (of type CommitPanel)
// for a particular `Git::Rev` (commit/branch/tag)
class _RevColumn {
public:
    _RevColumn(const RevColumnOptions& opts) : _opts(opts) {
        // Set our column name
        {
            _name = _opts.rev.displayName();
            
            if (_opts.head && _name!="HEAD") {
                _name = _name + " (HEAD)";
            }
            
            // Truncate the name to our width
            _name.erase(std::min(_name.size(), (size_t)_opts.width));
        }
        
        // Create our CommitPanels
        UI::Rect nameFrame = {{_opts.offsetX,0}, {_opts.width, 1}};
        _truncated = Intersection(_opts.win->bounds(), nameFrame) != nameFrame;
        if (!_truncated) {
            // Create panels for each commit
            int offY = CommitsInsetY;
            Git::Commit commit = _opts.rev.commit;
            size_t skip = _opts.rev.skip;
            while (commit) {
                if (!skip) {
                    Point p = {_opts.offsetX, offY};
                    UI::CommitPanel panel = MakeShared<UI::CommitPanel>(_opts.colors, false, _opts.width, commit);
                    UI::Rect frame = {p, panel->frame().size};
                    // Check if any part of the window would be offscreen
                    if (Intersection(_opts.win->bounds(), frame) != frame) break;
                    panel->setPosition(p);
                    _panels.push_back(panel);
                    offY += panel->frame().size.y + 1;
                
                } else {
                    skip--;
                }
                
                commit = commit.parent();
            }
        }
        
        // Create our undo/redo buttons
        {
            int buttonWidth = 8;
            int w2 = _opts.width/2;
            Rect leftFrame = Inset({{_opts.offsetX, ButtonsInsetY}, {w2,3}}, {4, 0});
            Rect rightFrame = Inset({{_opts.offsetX+w2, ButtonsInsetY}, {w2,3}}, {4, 0});
            UI::ButtonOptions undoOpts = {
                .colors = _opts.colors,
                .label = "Undo",
//                .key = "z",
                .enabled = true,
                .center = true,
                .borderColor = _opts.colors.subtitleText,
                .insetX = 1,
                .frame = leftFrame,
            };
            _undoButton = UI::Button(undoOpts);
            
            UI::ButtonOptions redoOpts = {
                .colors = _opts.colors,
                .label = "Redo",
//                .key = "Z",
                .enabled = true,
                .center = true,
                .borderColor = _opts.colors.subtitleText,
                .insetX = 1,
                .frame = rightFrame,
            };
            _redoButton = UI::Button(redoOpts);
        }
    }
    
    void draw() {
        // Don't draw anything if our column is truncated
        if (_truncated) return;
        
//        _opts.win->drawLineVert({_opts.offsetX+_opts.width/2, 0}, _opts.win->bounds().size.y);
        
        // Draw branch name
        {
            UI::Attr attr(_opts.win, A_UNDERLINE);
            const int offX = _opts.offsetX + (_opts.width-(int)_name.size())/2;
            _opts.win->drawText({offX, TitleInsetY}, "%s", _name.c_str());
        }
        
//        if (_opts.showMutability) {
//            UI::Attr attr(_opts.win, _opts.colors.error);
//            const char immutableText[] = "read-only";
//            const int offX = _opts.offsetX + std::max(0, (_opts.width-(int)(std::size(immutableText)-1))/2);
//            if (!_opts.rev.isMutable()) {
//                _opts.win->drawText({offX, 2}, "%s", immutableText);
//            }
//        }
        
        for (UI::CommitPanel p : _panels) {
            p->drawIfNeeded();
        }
        
        _undoButton.draw(_opts.win);
        _redoButton.draw(_opts.win);
    }
    
    UI::CommitPanel hitTest(const UI::Point& p) {
        for (UI::CommitPanel panel : _panels) {
            if (panel->hitTest(p)) return panel;
        }
        return {};
    }
    
    Git::Rev rev() { return _opts.rev; }
    UI::CommitPanelVec& panels() { return _panels; }
    
private:
    static constexpr int ButtonsInsetY = 0;
    static constexpr int TitleInsetY   = 3;
    static constexpr int CommitsInsetY = 5;
    RevColumnOptions _opts;
    std::string _name;
    bool _truncated = false;
    UI::CommitPanelVec _panels;
    UI::Button _undoButton;
    UI::Button _redoButton;
};

using RevColumn = std::shared_ptr<_RevColumn>;

} // namespace UI
