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
    _SnapshotButton(Git::Repo repo, const State::Snapshot& snap, int width) {
        Git::Commit commit = State::Convert(repo, snap.history.get().head);
        Git::Signature sig = commit.author();
        
        _commitId = Git::DisplayStringForId(commit.id());
        _time = Time::RelativeTimeDisplayString(snap.creationTime);
        _commitAuthor = sig.name();
        _commitMessage = LineWrap::Wrap(1, width-2*_LineLenInset, commit.message());
        
        options().frame.size = {width, 3};
        
//        if (isdigit(_time[0])) _time += " ago";
    }
    
    void draw(Window win) const override {
    }
    
private:
    static constexpr size_t _LineCountMax = 2;
    static constexpr size_t _LineLenInset = 2;
    
    std::string _commitId;
    std::string _time;
    std::string _commitAuthor;
    std::vector<std::string> _commitMessage;
    
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
