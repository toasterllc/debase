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
    bool sessionStart = false;
    
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
        _commit.message = LineWrap::Wrap(1, _opts.width, commit.message());
        
        options().frame.size = {_opts.width, 3};
        
//        if (isdigit(_time[0])) _time += " ago";
    }
    
    void draw(Window win) const override {
        constexpr int InsetX = 2;
        
        const ButtonOptions& opts = options();
        const ColorPalette& colors = opts.colors;
        const int width = opts.frame.size.x;
        const Point off = opts.frame.point;
        const Size offTextX = Size{InsetX, 0};
        const Size offTextY = Size{0, 0};
        const Size offText = off+offTextX+offTextY;
        
//        const Point offText = off + Size{InsetX, (_sessionStart ? 1 : 0)};
//        const int offY = (_sessionStart ? 1 : 0);
        
        if (_opts.sessionStart) {
//            const char title[] = "       Session Start      ";
//            int offX = 0;
//            UI::Attr attr(win, A_BOLD|A_UNDERLINE);
//            win->drawText(off + Size{offX, 0}, title);
            
//            const char title[] = "Session Start";
//            int offX = (width - (std::size(title)-1))/2;
//            UI::Attr attr(win, A_BOLD|A_UNDERLINE);
//            win->drawText(off + Size{offX, 0}, title);
            
//            const char title[] = "Session Start";
//            int offX = 0;
//            UI::Attr attr(win, A_BOLD);
//            win->drawText(off + Size{offX, 0}, title);
            
            
//            const char title[] = " Session Start ";
//            int offX = (width - (std::size(title)-1))/2;
//            win->drawLineHoriz(off, width);
//            UI::Attr attr(win, A_BOLD);
//            win->drawText(off + Size{offX, 0}, title);
            
            
//            win->drawLineHoriz(off, width);
//            UI::Attr attr(win, A_BOLD);
////            UI::Attr attr(win, A_UNDERLINE);
//            const char title[] = "Session Start";
//            int offX = 0;
////            int offX = (width - std::size(title)-1)/2;
//            win->drawText(off + Size{offX, 0}, "Session Start ");
        }
        
        // Draw time
        {
            UI::Attr attr(win, opts.colors.disabledText);
            int offX = width - (int)UTF8::Strlen(_time);
            win->drawText(off + offTextY + Size{offX, 0}, "%s", _time.c_str());
        }
        
        // Draw commit id
        {
            UI::Attr attr;
            if (opts.highlight) attr = UI::Attr(win, colors.menu|A_BOLD);
            win->drawText(offText, "%s", _commit.id.c_str());
        }
        
        // Draw author name
        {
            UI::Attr attr(win, opts.colors.disabledText);
//            if (opts.highlight) attr = UI::Attr(win, colors.menu|A_BOLD);
//            else                attr = UI::Attr(win, opts.colors.disabledText);
            win->drawText(offText + Size{0, 1}, "%s", _commit.author.c_str());
        }
        
        // Draw commit message
        {
//            UI::Attr attr;
//            if (opts.highlight) attr = UI::Attr(win, colors.menu|A_BOLD);
            
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
                    win->drawText(off + offTextY, "%s", "●");
                }
            }
            
//            if (opts.highlight) {
//                UI::Attr attr(win, colors.menu|A_BOLD);
////                win->drawText(off + Size{0,0}, "●");
//                win->drawText(off + Size{0,0}, "○");
//            }
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
    
    const SnapshotButtonOptions& snapshotOptions() { return _opts; }
    
private:
//    static constexpr int _SessionStartHeaderHeight = 1;
    
    SnapshotButtonOptions _opts;
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
