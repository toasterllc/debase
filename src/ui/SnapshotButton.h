#pragma once
#include "git/Git.h"
#include "state/State.h"
#include "Color.h"
#include "LineWrap.h"
#include "UTF8.h"
#include "Time.h"

namespace UI {

class SnapshotButton : public Button {
public:
    SnapshotButton(Git::Repo repo, const State::Snapshot& snapshot, int width) :
    repo(repo), snapshot(snapshot) {
        
        Git::Commit commit = State::Convert(repo, snapshot.head);
        Git::Signature sig = commit.author();
        
        _time = Time::RelativeTimeDisplayString(snapshot.creationTime);
        _commit.id = Git::DisplayStringForId(commit.id(), _CommitIdWidth);
        _commit.author = sig.name();
        _commit.message = LineWrap::Wrap(1, width-_TextInsetX, commit.message());
        
        size({width, 3});
    }
    
    void layout() override {
        // Don't call super::layout(), because we don't want Button to layout its
        // label (because it'll overlap our content and erase it)
    }
    
    void draw() override {
        const int width = size().x;
        const Size offTextX = Size{_TextInsetX, 0};
        const Size offTextY = Size{0, 0};
        const Size offText = offTextX+offTextY;
        
        // Draw time
        {
            Attr color = attr(Colors().dimmed);
            int offX = width - (int)UTF8::Len(_time);
            drawText(offTextY + Size{offX, 0}, _time.c_str());
        }
        
        // Draw commit id
        {
            Attr bold = attr(A_BOLD);
            Attr color;
            if (highlighted() || (activeSnapshot && !mouseActive())) {
                color = attr(Colors().menu);
            }
            drawText(offText, _commit.id.c_str());
        }
        
        // Draw author name
        {
            Attr color = attr(Colors().dimmed);
            drawText(offText + Size{0, 1}, _commit.author.c_str());
        }
        
        // Draw commit message
        {
            int i = 0;
            for (const std::string& line : _commit.message) {
                drawText(offText + Size{0, 2+i}, line.c_str());
                i++;
            }
        }
        
        // Draw highlight
        {
            if (highlighted()) {
                Attr color = attr(Colors().menu|A_BOLD);
                drawText(offTextY, "●");
            
            } else if (activeSnapshot) {
                if (mouseActive()) {
                    drawText(offTextY, "○");
                } else {
                    Attr color = attr(Colors().menu|A_BOLD);
                    drawText(offTextY, "●");
                }
            } else {
                // Draw a space to erase the previous highlight
                // Otherwise we'd need to erase the panel to get rid of the
                // previously-drawn ●/○, and this is easier
                drawText(offTextY, " ");
            }
        }
    }
    
    Git::Repo repo;
    State::Snapshot snapshot;
    bool activeSnapshot = false;
    
private:
    static constexpr int _CommitIdWidth = 7;
    static constexpr int _TextInsetX    = 2;
    
    std::string _time;
    struct {
        std::string id;
        std::string author;
        std::vector<std::string> message;
    } _commit;
};

using SnapshotButtonPtr = std::shared_ptr<SnapshotButton>;

} // namespace UI
