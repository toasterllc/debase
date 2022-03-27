#pragma once
#include "Git.h"
#include "Color.h"
#include "Attr.h"
#include "LineWrap.h"
#include "UTF8.h"
#include "State.h"
#include "Time.h"

namespace UI {

class _SnapshotButton : public _Button {
public:
    _SnapshotButton(Git::Repo repo, const State::Snapshot& snapshot, bool sessionStart, int width) :
    _sessionStart(sessionStart), _snapshot(snapshot) {
        
        Git::Commit commit = State::Convert(repo, _snapshot.head);
        Git::Signature sig = commit.author();
        
        _time = Time::RelativeTimeDisplayString(_snapshot.creationTime);
        _commit.id = Git::DisplayStringForId(commit.id());
        _commit.author = sig.name();
        _commit.message = LineWrap::Wrap(1, width, commit.message());
        
        options().frame.size = {width, 3};
        
//        if (isdigit(_time[0])) _time += " ago";
    }
    
    void draw(Window win) const override {
        const ButtonOptions& opts = options();
        const ColorPalette& colors = opts.colors;
        const int width = opts.frame.size.x;
        const Point off = opts.frame.point;
        
        {
            UI::Attr attr;
            if (opts.highlight) attr = UI::Attr(win, colors.menu|A_BOLD);
            win->drawText(off + Size{0, 0}, "%s", _commit.id.c_str());
        }
        
        {
            UI::Attr attr(win, opts.colors.subtitleText);
            int offX = width - (int)UTF8::Strlen(_time);
            win->drawText(off + Size{offX, 0}, "%s", _time.c_str());
        }
        
        {
            UI::Attr attr;
            if (opts.highlight) attr = UI::Attr(win, colors.menu|A_BOLD);
            else                attr = UI::Attr(win, opts.colors.subtitleText);
            win->drawText(off + Size{0, 1}, "%s", _commit.author.c_str());
        }
        
        {
            UI::Attr attr;
            if (opts.highlight) attr = UI::Attr(win, colors.menu|A_BOLD);
            
            int i = 0;
            for (const std::string& line : _commit.message) {
                win->drawText(off + Size{0, 2+i}, "%s", line.c_str());
                i++;
            }
        }
        
//        {
//            UI::Attr attr;
//            if (_borderColor) attr = Attr(shared_from_this(), *_borderColor);
//            drawBorder();
//            
//            if (_commit.isMerge()) {
//                mvwprintw(*this, 1, 0, "𝝠");
//            }
//        }
        
//        {
//            constexpr int width = 16;
//            int offX = width - (int)UTF8::Strlen(_time);
//            win->drawText(off + Size{12 + (_header ? 1 : 0) + offX, 0}, " %s ", _time.c_str());
//        }
        
//        if (_header) {
//            UI::Attr attr;
////            if (_borderColor) attr = Attr(shared_from_this(), *_borderColor);
//            win->drawText({3, 0}, " %s ", _headerLabel.c_str());
//        }
    }
    
    const State::Snapshot& snapshot() { return _snapshot; }
    
private:
    static constexpr size_t _LineCountMax = 2;
    static constexpr size_t _LineLenInset = 2;
    
    const State::Snapshot _snapshot;
    
    bool _sessionStart = false;
    std::string _time;
    struct {
        std::string id;
        std::string author;
        std::vector<std::string> message;
    } _commit;
    
//    static ButtonOptions _Opts() {
//        return ButtonOptions{
//            .colors ,
//            .label,
//            .key,
//            .enabled = false,
//            .center = false,
//            .drawBorder = false,
//            .insetX = 0,
//            .hitTestExpand = 0,
//            .highlight = false,
//            .frame,
//        };
//    }
};

using SnapshotButton = std::shared_ptr<_SnapshotButton>;

} // namespace UI
