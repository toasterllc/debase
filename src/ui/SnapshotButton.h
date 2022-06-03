#pragma once
#include "git/Git.h"
#include "state/State.h"
#include "Color.h"
#include "LineWrap.h"
#include "UTF8.h"
#include "Time.h"
#include "Label.h"

namespace UI {

class SnapshotButton : public Button {
public:
    SnapshotButton(Git::Repo repo, const State::Snapshot& snapshot, int width) :
    _repo(repo), _snapshot(snapshot), _width(width) {
        
        Git::Commit commit = State::Convert(repo, snapshot.refState.head);
        Git::Signature sig = commit.author();
        
        _time->text(Time::RelativeTimeString({.pastSuffix="ago", .abbreviate=true}, snapshot.creationTime));
        _time->align(Align::Right);
        _time->textAttr(colors().dimmed);
        
        _id->text(Git::DisplayStringForId(commit.id(), _CommitIdWidth));
        
        _author->text(sig.name());
        _author->textAttr(colors().dimmed);
        
        _message->text(commit.message());
        _message->wrap(true);
        
        sizeToFit(ConstraintNoneSize);
    }
    
    Size sizeIntrinsic(Size constraint) override { return {_width, 3}; }
    
    void layout() override {
        // Don't call super::layout(), because we don't want Button to layout its
        // label, because it overlaps our content and erases it
        
        const int width = size().x;
        const Size offTextX = Size{_TextInsetX, 0};
        const Size offTextY = Size{0, 0};
        const Size offText = offTextX+offTextY;
        
        _time->frame            ({offText,              {width-offText.x, 1}  });
        _id->frame              ({offText,              {width-offText.x, 1}  });
        _author->frame          ({offText + Size{0,1},  {width-offText.x, 1}  });
        _message->frame         ({offText + Size{0,2},  {width-offText.x, 1}  });
        _highlightSymbol->frame ({offTextY,             {1,1}       });
    }
    
    void draw() override {
        const bool menuColor = (highlighted() || (_activeSnapshot && !mouseActive()));
        const Color idColor = (menuColor ? colors().menu : colors().normal);
        _id->textAttr(idColor|WA_BOLD);
        
        // Draw highlight
        if (highlighted() || (_activeSnapshot && !mouseActive())) {
            _highlightSymbol->textAttr(colors().menu|WA_BOLD);
            _highlightSymbol->text("●");
        
        } else if (_activeSnapshot && mouseActive()) {
            _highlightSymbol->textAttr(colors().normal);
            _highlightSymbol->text("○");
        
        } else {
            _highlightSymbol->textAttr(colors().normal);
            _highlightSymbol->text(" ");
        }
    }
    
    const auto& repo() const { return _repo; }
    const auto& snapshot() const { return _snapshot; }
    const auto& width() const { return _width; }
    
    const auto& activeSnapshot() const { return _activeSnapshot; }
    template <typename T> bool activeSnapshot(const T& x) { return _set(_activeSnapshot, x); }
    
private:
    static constexpr int _CommitIdWidth = 7;
    static constexpr int _TextInsetX    = 2;
    
    Git::Repo _repo;
    State::Snapshot _snapshot;
    int _width = 0;
    
    bool _activeSnapshot = false;
    
    LabelPtr _time              = subviewCreate<Label>();
    LabelPtr _id                = subviewCreate<Label>();
    LabelPtr _author            = subviewCreate<Label>();
    LabelPtr _message           = subviewCreate<Label>();
    LabelPtr _highlightSymbol   = subviewCreate<Label>();
};

using SnapshotButtonPtr = std::shared_ptr<SnapshotButton>;

} // namespace UI
