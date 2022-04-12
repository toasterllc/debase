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
        
        Git::Commit commit = State::Convert(repo, snapshot.head);
        Git::Signature sig = commit.author();
        
        _time->text(Time::RelativeTimeDisplayString(snapshot.creationTime));
        _time->align(Align::Right);
        _time->textAttr(Colors().dimmed);
        
        _id->text(Git::DisplayStringForId(commit.id(), _CommitIdWidth));
        
        _author->text(sig.name());
        _author->textAttr(Colors().dimmed);
        
        _message->text(commit.message());
        _message->wrap(true);
        
        sizeToFit();
    }
    
    Size sizeIntrinsic(Size constraint) override { return {_width, 3}; }
    
    void layout() override {
        // Don't call super::layout(), because we don't want Button to layout its
        // label, because it overlaps our content and erases it
        
        const int width = size().x;
        const Size offTextX = Size{_TextInsetX, 0};
        const Size offTextY = Size{0, 0};
        const Size offText = offTextX+offTextY;
        
        _time->frame            ({offTextY,             {width, 1}  });
        _id->frame              ({offText,              {width, 1}  });
        _author->frame          ({offText + Size{0,1},  {width, 1}  });
        _message->frame         ({offText + Size{0,2},  {width, 1}  });
        _highlightSymbol->frame ({offTextY,             {1,1}       });
    }
    
    void draw() override {
        const int width = size().x;
        const Size offTextX = Size{_TextInsetX, 0};
        const Size offTextY = Size{0, 0};
        const Size offText = offTextX+offTextY;
        
//        // Draw time
//        {
//            Attr color = attr(Colors().dimmed);
//            int offX = width - (int)UTF8::Len(_time);
//            drawText(offTextY + Size{offX, 0}, _time.c_str());
//        }
        
        const Color idColor = ((highlighted() || (_activeSnapshot && !mouseActive())) ? Colors().menu : Colors().normal);
        _id->textAttr(idColor|A_BOLD);
        
//        // Draw commit id
//        {
//            Attr bold = attr(A_BOLD);
//            Attr color;
//            if (highlighted() || (activeSnapshot && !mouseActive())) {
//                color = attr(Colors().menu);
//            }
//            drawText(offText, _commit.id.c_str());
//        }
        
//        // Draw author name
//        {
//            Attr color = attr(Colors().dimmed);
//            drawText(offText + Size{0, 1}, _commit.author.c_str());
//        }
        
//        // Draw commit message
//        {
//            int i = 0;
//            for (const std::string& line : _commit.message) {
//                drawText(offText + Size{0, 2+i}, line.c_str());
//                i++;
//            }
//        }
        
//        // Draw highlight
//        if (highlighted()) {
////            _highlightSymbol->textAttr(Colors().menu|A_BOLD);
//            _highlightSymbol->text("A");
//            os_log(OS_LOG_DEFAULT, "AAA");
////            _highlightSymbol->drawNeeded(true);
////            _highlightSymbol->eraseNeeded(false);
//        } else {
////            _highlightSymbol->textAttr(Colors().normal);
//            _highlightSymbol->text("B");
//            os_log(OS_LOG_DEFAULT, "BBB");
////            _highlightSymbol->drawNeeded(true);
////            _highlightSymbol->eraseNeeded(false);
//        }
        
        // Draw highlight
        if (highlighted() || (_activeSnapshot && !mouseActive())) {
            _highlightSymbol->textAttr(Colors().menu|A_BOLD);
            _highlightSymbol->text("●");
        
        } else if (_activeSnapshot && mouseActive()) {
            _highlightSymbol->textAttr(Colors().normal);
            _highlightSymbol->text("○");
        
        } else {
            _highlightSymbol->textAttr(Colors().normal);
            _highlightSymbol->text(" ");
        }
    }
    
    const auto& repo() const { return _repo; }
    const auto& snapshot() const { return _snapshot; }
    const auto& width() const { return _width; }
    
    const auto& activeSnapshot() const { return _activeSnapshot; }
    template <typename T> void activeSnapshot(const T& x) { _set(_activeSnapshot, x); }
    
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
