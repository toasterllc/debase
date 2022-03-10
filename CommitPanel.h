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
class CommitPanel : public Panel {
public:
    CommitPanel(Git::Commit commit, size_t idx, int width) {
        _s = std::make_shared<_State>();
        _s->commit = commit;
        _s->idx = idx;
        _s->oid = Git::Str(*git_commit_id(*_s->commit));
        _s->time = Git::Str(git_commit_time(*_s->commit));
        _s->author = git_commit_author(*_s->commit)->name;
        
        const std::string message = git_commit_message(*_s->commit);
        std::deque<std::string> words;
        
        {
            std::istringstream stream(message);
            std::string word;
            while (stream >> word) words.push_back(word);
        }
        
        const int MaxLineCount = 2;
        const int MaxLineLen = width-4;
        for (;;) {
            std::string& line = _s->message.emplace_back();
            for (;;) {
                if (words.empty()) break;
                const std::string& word = words.front();
                const std::string add = (line.empty() ? "" : " ") + word;
                if (line.size()+add.size() > MaxLineLen) break; // Line filled, next line
                line += add;
                words.pop_front();
            }
            
            if (words.empty()) break; // No more words -> done
            if (_s->message.size() >= MaxLineCount) break; // Hit max number of lines -> done
            if (words.front().size() > MaxLineLen) break; // Current word is too large for line -> done
        }
        
        // Add as many letters from the remaining word as will fit on the last line
        if (!words.empty()) {
            const std::string& word = words.front();
            std::string& line = _s->message.back();
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
        
        setSize({width, 3 + (int)_s->message.size()});
        _s->drawNeeded = true;
    }
    
    void setBorderColor(std::optional<Color> x) {
        if (_s->borderColor == x) return;
        _s->borderColor = x;
        _s->drawNeeded = true;
    }
    
    void draw() {
        erase();
        
        int i = 0;
        for (const std::string& line : _s->message) {
            drawText({2, 2+i}, "%s", line.c_str());
            i++;
        }
        
        {
            UI::Attr attr;
            if (_s->borderColor) attr = Attr(*this, COLOR_PAIR(*_s->borderColor));
            drawBorder();
            drawText({2, 0}, " %s ", _s->oid.c_str());
        }
        drawText({12, 0}, " %s ", _s->time.c_str());
        
        {
            auto attr = Attr(*this, COLOR_PAIR(Colors::SubtitleText));
            drawText({2, 1}, "%s", _s->author.c_str());
        }
        
        _s->drawNeeded = false;
    }
    
    void drawIfNeeded() {
        if (_s->drawNeeded) {
            draw();
        }
    }
    
    const Git::Commit& commit() const { return _s->commit; }
    const size_t idx() const { return _s->idx; }
    
    bool operator<(const CommitPanel& x) {
        if (_s->idx != x.idx()) return _s->idx < x.idx();
        // Indices are equal; perform pointer comparison
        return _s.get() < x._s.get();
    }
    
private:
    struct _State {
        Git::Commit commit;
        size_t idx = 0;
        std::string oid;
        std::string time;
        std::string author;
        std::vector<std::string> message;
        std::optional<Color> borderColor;
        bool drawNeeded = false;
    };
    
    std::shared_ptr<_State> _s;
};

static bool Contains(const std::set<CommitPanel>& s, CommitPanel p) {
    return s.find(p) != s.end();
}

using CommitPanelVec = std::vector<CommitPanel>;
using CommitPanelVecIter = CommitPanelVec::iterator;

} // namespace UI
