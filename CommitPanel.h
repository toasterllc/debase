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
        _header = header;
        _id = Git::StringForId(*git_commit_id(*_commit));
        _time = Git::ShortStringForTime(git_commit_author(*_commit)->when.time);
        _author = git_commit_author(*_commit)->name;
        
        const std::string message = git_commit_message(*_commit);
        _message = LineWrap::Wrap(_LineCountMax, width-2*_LineLenInset, message);
        
        setSize({width, (_header ? 1 : 0) + 3 + (int)_message.size()});
        _drawNeeded = true;
    }
    
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
            
            if (_commit.isMerge()) {
                UI::Rect rect = bounds();
//                UI::Attr attr(shared_from_this(), _colors.subtitleText);
                mvwprintw(*this, rect.ymax(), rect.xmax()-1, "𝝠");
            }
            
////            UI::Rect rect = bounds();
//            
////            mvwprintw(*this, rect.ymax(), rect.xmax(), "m");
////            mvwprintw(*this, rect.ymax(), rect.xmax()-2, "m");
//            
//            
////            UI::Rect rect = bounds();
////            mvwaddch(*this, rect.size.y-1, rect.size.x/2, ACS_TTEE);
//            
//            
//            
//            {
//                UI::Rect rect = bounds();
//                const int x1 = rect.point.x;
//                const int y1 = rect.point.y;
//                const int x2 = rect.point.x+rect.size.x-1;
//                const int y2 = rect.point.y+rect.size.y-1;
//                mvwhline(*this, y1, x1, 0, rect.size.x);
//                mvwhline(*this, y2, x1, 0, rect.size.x);
//                mvwvline(*this, y1, x1, 0, rect.size.y);
//                mvwvline(*this, y1, x2, 0, rect.size.y);
//                mvwaddch(*this, y1, x1, ACS_ULCORNER);
//                mvwaddch(*this, y2, x1, ACS_LLCORNER);
//                mvwaddch(*this, y1, x2, ACS_URCORNER);
//                mvwaddch(*this, y2, x2, ACS_LRCORNER);
//                
////                mvwaddch(*this, y2, x2-6, ACS_URCORNER);
////                mvwprintw(*this, y2, x2-5, "Merge");
//                
////                mvwaddch(*this, y2, x2-2, ACS_URCORNER);
//                {
//                    UI::Attr attr(shared_from_this(), _colors.subtitleText);
//                    mvwprintw(*this, y2, x2-1, "𝝠");
//                }
////                mvwprintw(*this, y2-1, x2, "𝝠");
//                
////                cchar_t c = {
////                    .chars = {L"╲M"}
////                };
////                
////                
////                mvwadd_wch(*this, y2, x2-1, &c);
//            }
//            
//            
//            
//            
////            {
////                UI::Rect rect = bounds();
////                const int x1 = rect.point.x;
////                const int y1 = rect.point.y;
////                const int x2 = rect.point.x+rect.size.x-1;
////                const int y2 = rect.point.y+rect.size.y-1;
////                mvwhline(*this, y1, x1, 0, rect.size.x);
////                mvwhline(*this, y2, x1, 0, rect.size.x-2);
////                mvwvline(*this, y1, x1, 0, rect.size.y);
////                mvwvline(*this, y1, x2, 0, rect.size.y-1);
////                mvwaddch(*this, y1, x1, ACS_ULCORNER);
////                mvwaddch(*this, y2, x1, ACS_LLCORNER);
////                mvwaddch(*this, y1, x2, ACS_URCORNER);
////                mvwaddch(*this, y2, x2, ACS_LRCORNER);
////                
////                mvwaddch(*this, y2, x2-2, ACS_URCORNER);
////                mvwprintw(*this, y2, x2-2, "𝙈");
////            }
//
////            {
////                UI::Rect rect = bounds();
////                const int x1 = rect.point.x;
////                const int y1 = rect.point.y;
////                const int x2 = rect.point.x+rect.size.x-1;
////                const int y2 = rect.point.y+rect.size.y-1;
////                mvwhline(*this, y1, x1, 0, rect.size.x);
////                mvwhline(*this, y2, x1, 0, rect.size.x-2);
////                mvwvline(*this, y1, x1, 0, rect.size.y);
////                mvwvline(*this, y1, x2, 0, rect.size.y-1);
////                mvwaddch(*this, y1, x1, ACS_ULCORNER);
////                mvwaddch(*this, y2, x1, ACS_LLCORNER);
////                mvwaddch(*this, y1, x2, ACS_URCORNER);
////                mvwaddch(*this, y2, x2, ACS_LRCORNER);
////                
//////                mvwaddch(*this, y2, x2-2, ACS_URCORNER);
////                {
////                    UI::Attr attr(shared_from_this(), _colors.subtitleText);
////                    mvwprintw(*this, y2, x2-1, "𝑚");
////                }
////            }
//            
//            
////            {
////                UI::Rect rect = bounds();
////                const int x1 = rect.point.x;
////                const int y1 = rect.point.y;
////                const int x2 = rect.point.x+rect.size.x-1;
////                const int y2 = rect.point.y+rect.size.y-1;
////                mvwhline(*this, y1, x1, 0, rect.size.x);
////                mvwhline(*this, y2, x1, 0, rect.size.x-2);
////                mvwvline(*this, y1, x1, 0, rect.size.y);
////                mvwvline(*this, y1, x2, 0, rect.size.y);
////                mvwaddch(*this, y1, x1, ACS_ULCORNER);
////                mvwaddch(*this, y2, x1, ACS_LLCORNER);
////                mvwaddch(*this, y1, x2, ACS_URCORNER);
//////                mvwaddch(*this, y2, x2, ACS_LRCORNER);
////                
////                mvwaddch(*this, y2, x2-2, ACS_URCORNER);
////                {
////                    UI::Attr attr(shared_from_this(), _colors.subtitleText);
////                    mvwprintw(*this, y2, x2-1, "𝗺");
////                }
////            }
//            
//            
////            {
////                UI::Rect rect = bounds();
////                const int x1 = rect.point.x;
////                const int y1 = rect.point.y;
////                const int x2 = rect.point.x+rect.size.x-1;
////                const int y2 = rect.point.y+rect.size.y-1;
////                mvwhline(*this, y1, x1, 0, rect.size.x);
////                mvwhline(*this, y2, x1, 0, rect.size.x);
////                mvwvline(*this, y1, x1, 0, rect.size.y);
////                mvwvline(*this, y1, x2, 0, rect.size.y);
////                mvwaddch(*this, y1, x1, ACS_ULCORNER);
////                mvwaddch(*this, y2, x1, ACS_LLCORNER);
////                mvwaddch(*this, y1, x2, ACS_URCORNER);
////                mvwaddch(*this, y2, x2, ACS_LRCORNER);
////                
//////                mvwaddch(*this, y2-1, x2, ACS_ULCORNER);
//////                mvwaddch(*this, y2-2, x2, ACS_LLCORNER);
////                mvwprintw(*this, y2-1, x2, "ⅿ");
////                
//////                mvwaddch(*this, y2, x2-2, ACS_URCORNER);
//////                {
//////                    UI::Attr attr(shared_from_this(), _colors.subtitleText);
//////                    mvwprintw(*this, y2-1, x2, "𝗺");
//////                }
////            }
//            
//            
////            {
////                UI::Rect rect = bounds();
////                const int x1 = rect.point.x;
////                const int y1 = rect.point.y;
////                const int x2 = rect.point.x+rect.size.x-1;
////                const int y2 = rect.point.y+rect.size.y-1;
////                mvwvline(*this, y1, x1, 0, rect.size.y);
////                mvwvline(*this, y1, x2, 0, rect.size.y);
////                mvwhline(*this, y1, x1, 0, rect.size.x);
////                mvwhline(*this, y2, x1, 0, rect.size.x);
////                mvwaddch(*this, y1, x1, ACS_ULCORNER);
////                mvwaddch(*this, y2, x1, ACS_LLCORNER);
////                mvwaddch(*this, y1, x2, ACS_URCORNER);
////                mvwaddch(*this, y2, x2, ACS_LRCORNER);
////                
//////                mvwaddch(*this, y2-1, x2, ACS_ULCORNER);
//////                mvwaddch(*this, y2-2, x2, ACS_LLCORNER);
//////                mvwprintw(*this, y2-1, x2, "𝗺");
////                
//////                mvwaddch(*this, y2, x2-2, ACS_URCORNER);
////                {
//////                    UI::Attr attr(shared_from_this(), _colors.menu);
////                    mvwprintw(*this, y2-1, x2, "m");
////                }
////            }
//            
//            
//            
//            
//            
//            
//            
////            {
////                UI::Rect rect = bounds();
////                const int x1 = rect.point.x;
////                const int y1 = rect.point.y;
////                const int x2 = rect.point.x+rect.size.x-1;
////                const int y2 = rect.point.y+rect.size.y-1;
////                mvwhline(*this, y1, x1, 0, rect.size.x);
////                mvwhline(*this, y2, x1, 0, rect.size.x-2);
////                mvwvline(*this, y1, x1, 0, rect.size.y);
////                mvwvline(*this, y1, x2, 0, rect.size.y-1);
////                mvwaddch(*this, y1, x1, ACS_ULCORNER);
////                mvwaddch(*this, y2, x1, ACS_LLCORNER);
////                mvwaddch(*this, y1, x2, ACS_URCORNER);
//////                mvwaddch(*this, y2, x2, ACS_LRCORNER);
////                
//////                mvwaddch(*this, y2, x2-2, ACS_URCORNER);
////                mvwprintw(*this, y2, x2-1, "𝙈");
////            }
//            
//            
////            {
////                UI::Rect rect = bounds();
////                const int x1 = rect.point.x;
////                const int y1 = rect.point.y;
////                const int x2 = rect.point.x+rect.size.x-1;
////                const int y2 = rect.point.y+rect.size.y-1;
////                mvwhline(*this, y1, x1, 0, rect.size.x);
////                mvwhline(*this, y2, x1, 0, rect.size.x-2);
////                mvwvline(*this, y1, x1, 0, rect.size.y);
////                mvwvline(*this, y1, x2, 0, rect.size.y-1);
////                mvwaddch(*this, y1, x1, ACS_ULCORNER);
////                mvwaddch(*this, y2, x1, ACS_LLCORNER);
////                mvwaddch(*this, y1, x2, ACS_URCORNER);
//////                mvwaddch(*this, y2, x2, ACS_LRCORNER);
////                
//////                mvwaddch(*this, y2, x2-2, ACS_URCORNER);
////                mvwprintw(*this, y2, x2-1, "𝘔");
////            }
            
            
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
    
private:
    static constexpr size_t _LineCountMax = 2;
    static constexpr size_t _LineLenInset = 2;
    
    const ColorPalette& _colors;
    Git::Commit _commit;
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
