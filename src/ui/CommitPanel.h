#pragma once
#include <optional>
#include "git/Git.h"
#include "Panel.h"
#include "Color.h"
#include "LineWrap.h"
#include "UTF8.h"
#include <os/log.h>

namespace UI {

// CommitPanel: a Panel representing a particular git commit
// CommitPanel contains an index indicating the index of the panel/commit within
// its containing branch, where the top/first CommitPanel is index 0
class CommitPanel : public Panel {
public:
    CommitPanel(bool header, int width, Git::Commit commit) {
        _commit = commit;
        _header = header;
        _id = Git::DisplayStringForId(_commit.id());
        
        Git::Signature sig = _commit.author();
        _time = Git::ShortStringForTime(Git::TimeForGitTime(sig.time()));
        _author = sig.name();
        _message = LineWrap::Wrap(_LineCountMax, width-2*_LineLenInset, _commit.message());
        
        size({width, (_header ? 1 : 0) + 3 + (int)_message.size()});
    }
    
    ~CommitPanel() {
        os_log(OS_LOG_DEFAULT, "~CommitPanel()");
    }
    
    void borderColor(const Color& x) {
        if (_borderColor == x) return;
        _borderColor = x;
        drawNeeded(true);
    }
    
    void headerLabel(std::string_view x) {
        assert(_header);
        if (_headerLabel == x) return;
        _headerLabel = x;
        drawNeeded(true);
    }
    
    void draw(const Window& win) override {
        const int offY = (_header ? 1 : 0);
        int i = 0;
        for (const std::string& line : _message) {
            drawText({2, offY+2+i}, line.c_str());
            i++;
        }
        
        {
            Window::Attr color = attr(_borderColor);
            drawBorder();
            
            if (_commit.isMerge()) {
                drawText({0, 1}, "𝝠");
            }
        }
        
        {
            Window::Attr bold = attr(A_BOLD);
            Window::Attr color;
            if (!_header) color = attr(_borderColor);
            drawText({2 + (_header ? -1 : 0), offY+0}, " %s ", _id.c_str());
        }
        
        {
            constexpr int width = 16;
            int off = width - (int)UTF8::Strlen(_time);
            drawText({12 + (_header ? 1 : 0) + off, offY+0}, " %s ", _time.c_str());
        }
        
        if (_header) {
            Window::Attr color = attr(_borderColor);
            drawText({3, 0}, " %s ", _headerLabel.c_str());
        }
        
        {
            Window::Attr color = attr(Colors().dimmed);
            drawText({2, offY+1}, _author.c_str());
        }
    }
    
    const Git::Commit commit() const { return _commit; }
    
private:
    static constexpr size_t _LineCountMax = 2;
    static constexpr size_t _LineLenInset = 2;
    
    Git::Commit _commit;
    bool _header = false;
    std::string _headerLabel;
    std::string _id;
    std::string _time;
    std::string _author;
    std::vector<std::string> _message;
    Color _borderColor = Colors().normal;
};

using CommitPanelPtr = std::shared_ptr<CommitPanel>;
using CommitPanelVec = std::vector<CommitPanelPtr>;
using CommitPanelIter = CommitPanelVec::const_iterator;

} // namespace UI
