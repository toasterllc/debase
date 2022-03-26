#pragma once
#include <optional>
#include "Git.h"
#include "Panel.h"
#include "Color.h"
#include "Attr.h"
#include "LineWrap.h"
#include "UTF8.h"
#include "State.h"

namespace UI {

class _SnapshotButton : public _Button {
public:
    _SnapshotButton(const ButtonOptions& opts, Git::Repo repo, const State::Snapshot& snap) : _Button(opts) {
        Git::Commit commit = State::Convert(repo, snap.history.get().head);
    }
    
    using _Button::_Button;
    
    void draw(Window win) const override {
    }
    
private:
    std::string _commitId;
    std::string _time;
    std::string _commitAuthor;
    std::string _commitMsg;
    
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
