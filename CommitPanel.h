#pragma once
#include <optional>
#include "Git.h"
#include "Panel.h"
#include "Color.h"
#include "Attr.h"
#include "LineWrap.h"

namespace UI {

// CommitPanel: a Panel representing a particular git commit
// CommitPanel contains an index indicating the index of the panel/commit within
// its containing branch, where the top/first CommitPanel is index 0
class _CommitPanel : public _Panel, public std::enable_shared_from_this<_CommitPanel> {
public:
    _CommitPanel(const ColorPalette& colors, bool header, int width, Git::Commit commit) :
    _colors(colors) {
        _commit = commit;
//        _idx = idx;
        _header = header;
        _id = Git::StringForId(*git_commit_id(*_commit));
        _time = Git::ShortStringForTime(git_commit_author(*_commit)->when.time);
        _author = git_commit_author(*_commit)->name;
        
        const std::string message = git_commit_message(*_commit);
        _message = LineWrap::Wrap(_LineCountMax, width-2*_LineLenInset, message);
        
        setSize({width, (_header ? 1 : 0) + 3 + (int)_message.size()});
        _drawNeeded = true;
    }
    
//    void setVisible(bool x) {
//        _Panel::setVisible(x);
//        _drawNeeded = true;
//    }
    
    void setBorderColor(std::optional<Color> x) {
        if (_borderColor == x) return;
        _borderColor = x;
        _drawNeeded = true;
    }
    
    void setHeaderLabel(std::string_view x) {
        assert(_header);
        if (_headerLabel == x) return;
        _headerLabel = x;
        _drawNeeded = true;
    }
    
    void draw() {
//        erase();
        
        const int offY = (_header ? 1 : 0);
        
        int i = 0;
        for (const std::string& line : _message) {
            drawText({2, offY+2+i}, "%s", line.c_str());
            i++;
        }
        
        {
            UI::Attr attr;
            if (_borderColor) attr = Attr(shared_from_this(), *_borderColor);
            drawBorder();
        }
        
        {
            UI::Attr attr;
            if (!_header && _borderColor) attr = Attr(shared_from_this(), *_borderColor);
            drawText({2 + (_header ? -1 : 0), offY+0}, " %s ", _id.c_str());
        }
        
        drawText({12 + (_header ?  1 : 0), offY+0}, " %s ", _time.c_str());
        
        if (_header) {
            UI::Attr attr;
            if (_borderColor) attr = Attr(shared_from_this(), *_borderColor);
            drawText({3, 0}, " %s ", _headerLabel.c_str());
        }
        
        {
            UI::Attr attr(shared_from_this(), _colors.subtitleText);
            drawText({2, offY+1}, "%s", _author.c_str());
        }
        
        _drawNeeded = false;
    }
    
    void drawIfNeeded() {
        if (_drawNeeded) {
            draw();
        }
    }
    
    const Git::Commit commit() const { return _commit; }
//    const size_t idx() const { return _idx; }
    
//    bool operator<(const CommitPanel& x) {
//        if (_idx != x.idx()) return _idx < x.idx();
//        // Indices are equal; perform pointer comparison
//        return _s.get() < x._s.get();
//    }
    
private:
    static constexpr size_t _LineCountMax = 2;
    static constexpr size_t _LineLenInset = 2;
    
    const ColorPalette& _colors;
    Git::Commit _commit;
//    size_t _idx = 0;
    bool _header = false;
    std::string _headerLabel;
    std::string _id;
    std::string _time;
    std::string _author;
    std::vector<std::string> _message;
    std::optional<Color> _borderColor;
    bool _drawNeeded = false;
};

using CommitPanel = std::shared_ptr<_CommitPanel>;
using CommitPanelVec = std::vector<CommitPanel>;
using CommitPanelVecIter = CommitPanelVec::iterator;

} // namespace UI
