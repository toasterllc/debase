#pragma once
#include "Git.h"
#include "Color.h"
#include "Attr.h"
#include "LineWrap.h"
#include "UTF8.h"
#include "State.h"
#include "Time.h"

namespace UI {

struct SnapshotButtonOptions {
    // Required (by constructor)
    Git::Repo repo;
    State::Snapshot snapshot;
    int width = 0;
    
    // Optional
    bool activeSnapshot = false;
};

class _SnapshotButton : public _Button {
public:
    _SnapshotButton(const SnapshotButtonOptions& opts) : _opts(opts) {
        Git::Commit commit = State::Convert(_opts.repo, _opts.snapshot.head);
        Git::Signature sig = commit.author();
        
        _time = Time::RelativeTimeDisplayString(_opts.snapshot.creationTime);
        _commit.id = Git::DisplayStringForId(commit.id());
        _commit.author = sig.name();
        _commit.message = LineWrap::Wrap(1, _opts.width-_TextInsetX, commit.message());
        
        options().frame.size = {_opts.width, 3};
    }
    
    void draw(Window win) const override {
        const ButtonOptions& opts = options();
        const ColorPalette& colors = opts.colors;
        const int width = opts.frame.size.x;
        const Point off = opts.frame.point;
        const Size offTextX = Size{_TextInsetX, 0};
        const Size offTextY = Size{0, 0};
        const Size offText = off+offTextX+offTextY;
        
        // Draw time
        {
            UI::Attr attr(win, opts.colors.dimmed);
            int offX = width - (int)UTF8::Strlen(_time);
            win->drawText(off + offTextY + Size{offX, 0}, "%s", _time.c_str());
        }
        
        // Draw commit id
        {
            UI::Attr bold(win, A_BOLD);
            UI::Attr color;
            if (opts.highlight || (_opts.activeSnapshot && !opts.mouseActive)) {
                color = UI::Attr(win, colors.menu);
            }
            win->drawText(offText, "%s", _commit.id.c_str());
        }
        
        // Draw author name
        {
            UI::Attr attr(win, opts.colors.dimmed);
            win->drawText(offText + Size{0, 1}, "%s", _commit.author.c_str());
        }
        
        // Draw commit message
        {
            int i = 0;
            for (const std::string& line : _commit.message) {
                win->drawText(offText + Size{0, 2+i}, "%s", line.c_str());
                i++;
            }
        }
        
        // Draw highlight
        {
            if (opts.highlight) {
                UI::Attr attr(win, colors.menu|A_BOLD);
                win->drawText(off + offTextY, "%s", "●");
            
            } else if (_opts.activeSnapshot) {
                if (opts.mouseActive) {
                    win->drawText(off + offTextY, "%s", "○");
                } else {
                    UI::Attr attr(win, colors.menu|A_BOLD);
                    win->drawText(off + offTextY, "%s", "●");
                }
            }
        }
    }
    
    const SnapshotButtonOptions& snapshotOptions() { return _opts; }
    
private:
    static constexpr int _TextInsetX = 2;
    
    SnapshotButtonOptions _opts;
    std::string _time;
    struct {
        std::string id;
        std::string author;
        std::vector<std::string> message;
    } _commit;
};

using SnapshotButton = std::shared_ptr<_SnapshotButton>;

} // namespace UI
