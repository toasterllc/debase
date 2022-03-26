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
    _SnapshotButton(Git::Repo repo, const State::Snapshot& snap) {
        Git::Commit commit = State::Convert(repo, snap.history.get().head);
        Git::Signature sig = commit.author();
        
        _commitId = Git::DisplayStringForId(commit.id());
        _time = Time::RelativeTimeDisplayString(snap.creationTime);
        _commitAuthor = sig.name();
        _commitMsg = commit.message();
        
//        _commitMsg = LineWrap::Wrap(1, width-2*_LineLenInset, _commit.message());
        
//        if (isdigit(_time[0])) _time += " ago";
    }
    
    void draw(Window win) const override {
    }
    
private:
    std::string _commitId;
    std::string _time;
    std::string _commitAuthor;
    std::string _commitMsg;
    std::string _commitMsgLineWrapped;
    
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
