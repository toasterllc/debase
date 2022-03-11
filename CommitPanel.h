#pragma once
#include <optional>
#include "Git.h"
#include "Panel.h"
#include "Color.h"
#include "Attr.h"

namespace UI {

// CommitPanel: a Panel representing a particular git commit
// CommitPanel contains an index indicating the index of the panel/commit within
// its containing branch, where the top/first CommitPanel is index 0
class _CommitPanel : public _Panel, public std::enable_shared_from_this<_CommitPanel> {
public:
    _CommitPanel(Git::Commit commit, size_t idx, bool header, int width) {
        _commit = commit;
        _idx = idx;
        _header = header;
        _id = Git::Str(*git_commit_id(*_commit));
        _time = Git::Str(git_commit_time(*_commit));
        _author = git_commit_author(*_commit)->name;
        
        const std::string message = git_commit_message(*_commit);
        std::deque<std::string> words;
        
        {
            std::istringstream stream(message);
            std::string word;
            while (stream >> word) words.push_back(word);
        }
        
        const int MaxLineCount = 2;
        const int MaxLineLen = width-4;
        for (;;) {
            std::string& line = _message.emplace_back();
            for (;;) {
                if (words.empty()) break;
                const std::string& word = words.front();
                const std::string add = (line.empty() ? "" : " ") + word;
                if (line.size()+add.size() > MaxLineLen) break; // Line filled, next line
                line += add;
                words.pop_front();
            }
            
            if (words.empty()) break; // No more words -> done
            if (_message.size() >= MaxLineCount) break; // Hit max number of lines -> done
            if (words.front().size() > MaxLineLen) break; // Current word is too large for line -> done
        }
        
        // Add as many letters from the remaining word as will fit on the last line
        if (!words.empty()) {
            const std::string& word = words.front();
            std::string& line = _message.back();
            line += (line.empty() ? "" : " ") + word;
            // Our logic guarantees that if the word would have fit, it would've been included in the last line.
            // So since the word isn't included, the length of the line (with the word included) must be larger
            // than `MaxLineLen`. So verify that assumption.
            assert(line.size() > MaxLineLen);
            
//            const char*const ellipses = "...";
//            // Find the first non-space character, at least strlen(ellipses) characters before the end
//            auto it =line.begin()+MaxLineLen-strlen(ellipses);
//            for (; it!=line.begin() && std::isspace(*it); it--);
//            
//            line.erase(it, line.end());
//            
//            // Replace the line's final characters with an ellipses
//            line.replace(it, it+strlen(ellipses), ellipses);
        }
        
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
        erase();
        
        const int offY = (_header ? 1 : 0);
        
        int i = 0;
        for (const std::string& line : _message) {
            drawText({2, offY+2+i}, "%s", line.c_str());
            i++;
        }
        
        {
            UI::Attr attr;
            if (_borderColor) attr = Attr(shared_from_this(), COLOR_PAIR(*_borderColor));
            drawBorder();
        }
        
        {
            UI::Attr attr;
            if (!_header && _borderColor) attr = Attr(shared_from_this(), COLOR_PAIR(*_borderColor));
            drawText({2  + (_header ? -1 : 0), offY+0}, " %s ", _id.c_str());
        }
        
        drawText({12 + (_header ?  1 : 0), offY+0}, " %s ", _time.c_str());
        
        if (_header) {
            UI::Attr attr;
            if (_borderColor) attr = Attr(shared_from_this(), COLOR_PAIR(*_borderColor));
            drawText({3, 0}, " %s ", _headerLabel.c_str());
        }
        
        {
            UI::Attr attr(shared_from_this(), COLOR_PAIR(Colors::SubtitleText));
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
    const size_t idx() const { return _idx; }
    
//    bool operator<(const CommitPanel& x) {
//        if (_idx != x.idx()) return _idx < x.idx();
//        // Indices are equal; perform pointer comparison
//        return _s.get() < x._s.get();
//    }
    
private:
    Git::Commit _commit;
    size_t _idx = 0;
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
