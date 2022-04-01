#pragma once
#include "Git.h"
#include "Color.h"
#include "LineWrap.h"
#include "UTF8.h"
#include "State.h"
#include "Time.h"

namespace UI {

class SnapshotButton : public Button {
public:
//    template <typename T, typename ...Args>
//    static auto Make(const ColorPalette& colors, Git::Repo repo, State::Snapshot snapshot, int width) {
//        return std::make_shared<SnapshotButton>(colors, opts);
//    }
    
    SnapshotButton(const ColorPalette& colors, Git::Repo repo, const State::Snapshot& snapshot, int width) :
    Button(colors), repo(repo), snapshot(snapshot) {
        
        Git::Commit commit = State::Convert(repo, snapshot.head);
        Git::Signature sig = commit.author();
        
        _time = Time::RelativeTimeDisplayString(snapshot.creationTime);
        _commit.id = Git::DisplayStringForId(commit.id());
        _commit.author = sig.name();
        _commit.message = LineWrap::Wrap(1, width-_TextInsetX, commit.message());
        
        frame.size = {width, 3};
    }
    
    void draw(const Window& win) override {
        if (!drawNeeded) return;
        Button::draw(win);
        
        const int width = frame.size.x;
        const Point off = frame.point;
        const Size offTextX = Size{_TextInsetX, 0};
        const Size offTextY = Size{0, 0};
        const Size offText = off+offTextX+offTextY;
        
        // Draw time
        {
            Window::Attr color = win.attr(colors.dimmed);
            int offX = width - (int)UTF8::Strlen(_time);
            win.drawText(off + offTextY + Size{offX, 0}, "%s", _time.c_str());
        }
        
        // Draw commit id
        {
            Window::Attr bold = win.attr(A_BOLD);
            Window::Attr color;
            if (highlighted() || (activeSnapshot && !mouseActive())) {
                color = win.attr(colors.menu);
            }
            win.drawText(offText, "%s", _commit.id.c_str());
        }
        
        // Draw author name
        {
            Window::Attr color = win.attr(colors.dimmed);
            win.drawText(offText + Size{0, 1}, "%s", _commit.author.c_str());
        }
        
        // Draw commit message
        {
            int i = 0;
            for (const std::string& line : _commit.message) {
                win.drawText(offText + Size{0, 2+i}, "%s", line.c_str());
                i++;
            }
        }
        
        // Draw highlight
        {
            if (highlighted()) {
                Window::Attr color = win.attr(colors.menu|A_BOLD);
                win.drawText(off + offTextY, "%s", "●");
            
            } else if (activeSnapshot) {
                if (mouseActive()) {
                    win.drawText(off + offTextY, "%s", "○");
                } else {
                    Window::Attr color = win.attr(colors.menu|A_BOLD);
                    win.drawText(off + offTextY, "%s", "●");
                }
            }
        }
    }
    
    Git::Repo repo;
    State::Snapshot snapshot;
    bool activeSnapshot = false;
    
private:
    static constexpr int _TextInsetX = 2;
    
    std::string _time;
    struct {
        std::string id;
        std::string author;
        std::vector<std::string> message;
    } _commit;
};

using SnapshotButtonPtr = std::shared_ptr<SnapshotButton>;

} // namespace UI
