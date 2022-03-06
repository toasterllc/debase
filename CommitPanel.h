#pragma once
#include "Git.h"

class CommitPanel : public Panel {
public:
    CommitPanel(Commit commit, size_t idx, int width) {
        _commit = commit;
        _idx = idx;
        _oid = StrFromGitOid(*git_commit_id(*_commit));
        _time = StrFromGitTime(git_commit_time(*_commit));
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
//            // Replace the last 4 characters with an ellipses
//            line.replace(it, it+strlen(ellipses), ellipses);
        }
        
        setSize({width, 3 + (int)_message.size()});
//        setSize({width, 5 + (int)_message.size()});
        _drawNeeded = true;
    }
    
    bool selected() const { return _selected; }
    
    void setSelected(bool x) {
        if (_selected == x) return;
        _selected = x;
        _drawNeeded = true;
    }
    
    void draw() {
        erase();
        
        int i = 0;
        for (const std::string& line : _message) {
            drawText({2, 2+i}, "%s", line.c_str());
            i++;
        }
        
        {
            Window::Attr attr;
            if (_selected) attr = setAttr(COLOR_PAIR(1));
            drawBorder();
            drawText({2, 0}, " %s ", _oid.c_str());
        }
        drawText({12, 0}, " %s ", _time.c_str());
        
        {
            auto attr = setAttr(COLOR_PAIR(2));
            drawText({2, 1}, "%s", _author.c_str());
        }
        
        _drawNeeded = false;
    }
    
//    CommitPanel copy() {
//        CommitPanel p;
//        p._s = _s;
//        return p;
//    }
    
    void drawIfNeeded() {
        if (_drawNeeded) {
            draw();
        }
    }
    
    const Commit& commit() const { return _commit; }
    const size_t idx() const { return _idx; }
    
private:
//    CommitPanel() {}
    
    Commit _commit;
    size_t _idx = 0;
    std::string _oid;
    std::string _time;
    std::string _author;
    std::vector<std::string> _message;
    bool _selected = false;
    bool _drawNeeded = false;
};
