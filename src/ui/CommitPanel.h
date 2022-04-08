#pragma once
#include <optional>
#include "git/Git.h"
#include "Panel.h"
#include "Color.h"
#include "LineWrap.h"
#include "UTF8.h"
#include "Label.h"
#include <os/log.h>

namespace UI {

// CommitPanel: a Panel representing a particular git commit
// CommitPanel contains an index indicating the index of the panel/commit within
// its containing branch, where the top/first CommitPanel is index 0
class CommitPanel : public Panel {
public:
    CommitPanel() {
        borderColor(Colors().normal);
        
        _header->prefix(" ");
        _header->suffix(" ");
        
        _author->attr(Colors().dimmed);
        
//        _message->visible(false);
        _message->wrap(true);
        
        _mergeSymbol->text("𝝠");
        
        _time->align(Align::Right);
    }
    
    Size messageSize(int width) const {
        Size s = _message->sizeIntrinsic({width-2*_TextInset, 0});
        s.y = std::min(_MessageLineCountMax, s.y);
        return s;
    }
    
    Size sizeIntrinsic(Size constraint) override {
        const Size msgSize = messageSize(constraint.x);
        const bool header = !_header->text().empty();
        const int height = (header ? 1 : 0) + 3 + msgSize.y;
        return {constraint.x, height};
    }
    
    void layout(const Window& win) override {
        const Rect f = frame();
        const bool header = !_header->text().empty();
        const int offY = (header ? 1 : 0);
        
        _header->frame({{_TextInset+1,0}, {f.size.x-(_TextInset+1), 1}});
        _id->frame({{_TextInset, offY}, {f.size.x-2*_TextInset, 1}});
        _time->frame({{0, offY}, {f.size.x-_TextInset, 1}});
        _author->frame({{_TextInset, offY+1}, {f.size.x-2*_TextInset, 1}});
        
        Size ms = messageSize(size().x);
        ms.y = std::min(_MessageLineCountMax, ms.y);
        _message->frame({{_TextInset, offY+2}, ms});
        
        _mergeSymbol->frame({{0,1}, {1,1}});
    }
    
    void draw(const Window& win) override {
        assert(borderColor());
        const Color color = *borderColor();
        const bool header = !_header->text().empty();
        
        _header->attr(color);
        
        _id->attr(color|A_BOLD);
        _id->prefix(!header ? " " : "");
        _id->suffix(!header ? " " : "");
        
        _time->prefix(!header ? " " : "");
        _time->suffix(!header ? " " : "");
        
        _mergeSymbol->attr(color);
        
        // Always redraw _time/_id/_mergeSymbol because our border may have clobbered them
        _time->drawNeeded(true);
        _id->drawNeeded(true);
        _mergeSymbol->drawNeeded(true);
    }
    
    const auto& commit() const { return _commit; }
    template <typename T> void commit(const T& x) {
        _set(_commit, x);
        
        const Git::Signature sig = _commit.author();
        _id->text(Git::DisplayStringForId(_commit.id()));
        _time->text(Git::ShortStringForTime(Git::TimeForGitTime(sig.time())));
        _author->text(sig.name());
        _message->text(_commit.message());
        
        _mergeSymbol->visible(_commit.isMerge());
    }
    
    const auto& header() const { return _header; }
    template <typename T> void header(const T& x) { _set(_header, x); }
    
private:
    static constexpr int _MessageLineCountMax = 2;
    static constexpr int _TextInset = 2;
    
    Git::Commit _commit;
    LabelPtr _header        = createSubview<Label>();
    LabelPtr _id            = createSubview<Label>();
    LabelPtr _time          = createSubview<Label>();
    LabelPtr _author        = createSubview<Label>();
    LabelPtr _message       = createSubview<Label>();
    LabelPtr _mergeSymbol   = createSubview<Label>();
};

using CommitPanelPtr = std::shared_ptr<CommitPanel>;
using CommitPanelVec = std::vector<CommitPanelPtr>;
using CommitPanelIter = CommitPanelVec::const_iterator;

} // namespace UI
