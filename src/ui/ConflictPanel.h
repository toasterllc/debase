#pragma once
#include <string>
#include "git/Conflict.h"

// TODO: don't center conflict text vertically if there isn't enough content to fill the top.
//       shift both the left/right up by the same amount, so that their centers are aligned,
//       and text starts at the top (at least on one side)
// TODO: draw '---- empty ----' placeholder when conflict region is empty
// TODO: make filename title non-bold
// TODO: truncate the beginning of the filename, not the end
// TODO: handle indentation -- if the conflicted block is indented a lot, unindent the text

// √ TODO: improve conflict window sizing -- have some padding when large, fill window when small
// √ TODO: highlight conflict region text
// √ TODO: make conflict window fill the window size

namespace UI {

class ConflictPanel : public Panel {
public:
    enum class Layout {
        LeftOurs,
        RightOurs,
    };
    
    ConflictPanel(Layout layout, std::string_view refNameOurs, std::string_view refNameTheirs,
    const Git::FileConflict& fc, size_t hunkIdx) :
    _layout(layout), _fileConflict(fc), _hunkIdx(hunkIdx) {
        
        borderColor(colors().error);
        
        _title->inhibitErase(true); // Title overlaps border, so don't erase
        _title->textAttr(colors().error|WA_BOLD);
        _title->prefix(" ");
        _title->suffix(" ");
        _title->text("Conflict: " + fc.path.string());
        
        _refNameLeft->align(Align::Center);
        _refNameLeft->textAttr(colors().error|WA_BOLD);
        _refNameLeft->text(_layout==Layout::LeftOurs ? refNameOurs : refNameTheirs);
        
        _refNameRight->align(Align::Center);
        _refNameRight->textAttr(colors().error|WA_BOLD);
        _refNameRight->text(_layout==Layout::LeftOurs ? refNameTheirs : refNameOurs);
        
        _chooseButtonLeft->bordered(true);
        _chooseButtonLeft->highlightColor(colors().error);
        _chooseButtonLeft->label()->text("Choose");
        _chooseButtonLeft->label()->align(UI::Align::Left);
//        _chooseButtonLeft->key()->text("◀");
        
        _chooseButtonRight->bordered(true);
        _chooseButtonRight->highlightColor(colors().error);
        _chooseButtonRight->label()->text("Choose");
        _chooseButtonRight->label()->align(UI::Align::Left);
//        _chooseButtonRight->key()->text("▶");
        
        _openInEditorButton->bordered(true);
        _openInEditorButton->highlightColor(colors().error);
        _openInEditorButton->label()->text("Open in Editor");
        
        _cancelButton->bordered(true);
        _cancelButton->highlightColor(colors().error);
        _cancelButton->label()->text("Cancel");
    }
    
    Size sizeIntrinsic(Size constraint) override {
        Size s = constraint;
        const int paddingX = std::min(10, constraint.x/10);
        const int paddingY = std::min(6, constraint.y/10);
        s.x -= paddingX;
        s.y -= paddingY;
        
//        if (s.x >= 75) {
//            s.x -= 10;
//        }
//        if (s.y >= 25) {
//            s.y -= 5;
//        }
        return s;
//        return {std::min(75,constraint.x), std::min(25,constraint.y)};
    }
    
    using View::layout;
    void layout() override {
        View::layout();
        
        const Rect b = bounds();
        const Point titleOrigin = {_TitleInsetX,0};
        _title->frame({titleOrigin, {b.w()-2*_TitleInsetX, 1}});
        
        const Rect contentRectLeft = _ContentRectLeft(b);
        const Rect contentRectRight = _ContentRectRight(b);
        _refNameLeft->frame({contentRectLeft.tl()-Size{0,1}, {contentRectLeft.w(), 1}});
        _refNameRight->frame({contentRectRight.tl()-Size{0,1}, {contentRectRight.w(), 1}});
        
        _chooseButtonLeft->sizeToFit(ConstraintNoneSize);
        _chooseButtonLeft->origin({
            contentRectLeft.l() + (contentRectLeft.w()-_chooseButtonLeft->frame().w())/2,
            contentRectLeft.b()+1,
        });
        
        _chooseButtonRight->sizeToFit(ConstraintNoneSize);
        _chooseButtonRight->origin({
            contentRectRight.l() + (contentRectRight.w()-_chooseButtonRight->frame().w())/2,
            contentRectRight.b()+1,
        });
        
        _openInEditorButton->sizeToFit(ConstraintNoneSize);
        _openInEditorButton->origin({b.r()-_openInEditorButton->size().x-2, _chooseButtonRight->frame().b()+1});
        
        _cancelButton->sizeToFit(ConstraintNoneSize);
        _cancelButton->origin({_openInEditorButton->frame().l()-_cancelButton->size().x-1, _openInEditorButton->frame().t()});
        
//        _cancelButton->sizeToFit(ConstraintNoneSize);
    }
    
    using View::draw;
    void draw() override {
        View::draw();
        
        // Always redraw _title because our border may have clobbered it
        _title->drawNeeded(true);
        
        const Rect b = bounds();
        const int separatorX = _SeparatorX(b);
        const Rect contentRectLeft = _ContentRectLeft(b);
        const Rect contentRectRight = _ContentRectRight(b);
        const int conflictBottomY = _chooseButtonLeft->frame().b();
        
        _contentTextDraw(contentRectLeft, true);
        _contentTextDraw(contentRectRight, false);
        
        {
            Attr color = attr(colors().error);
//            Attr color = attr(colors().conflictTextDim);
            
            
//            constexpr cchar_t Chars = { 0, L"│" };
//            int len = conflictBottomY-_Inset.y;
//            mvwvline_set(*this, _Inset.y, separatorX, &Chars, len);
            
            
            drawLineVert({separatorX, _Inset.y}, conflictBottomY-_Inset.y);
        }
        
        {
            Attr color = attr(colors().conflictTextDim);
            drawLineHoriz({1, conflictBottomY}, b.w()-2);
        }
//        
//        drawRect(contentRectLeft);
//        drawRect(contentRectRight);
        
//        auto& hunk = _fileConflict.hunks[_hunkIdx];
//        assert(hunk.type == Git::FileConflict::Hunk::Type::Conflict);
//        
//        auto& lines = (_layout==Layout::LeftOurs ? hunk.conflict.linesOurs : hunk.conflict.linesTheirs);
//        int offY = Inset.y+std::max(0, (h-(int)lines.size())/2);
//        for (size_t i=0; i<lines.size(); i++) {
//            const std::string& line = lines[i];
//            drawText({2, offY+(int)i}, line.c_str());
//        }
        

    }
    
private:
    static constexpr int _TitleInsetX = 2;
    static constexpr Size _Inset = {3,1};
    static constexpr int _ContentInsetTop = 2;
    static constexpr int _ContentInsetBottom = 8;
    
    static int _SeparatorX(const Rect& bounds) {
        return bounds.w()/2;
    }
    
    static Rect _ContentRectLeft(const Rect& bounds) {
        const int w = _SeparatorX(bounds)-_Inset.x-2;
        const int h = bounds.h()-2*_Inset.y-_ContentInsetTop-_ContentInsetBottom;
        return {
            .origin = _Inset+Size{0,_ContentInsetTop},
            .size = {w, h},
        };
    }
    
    static Rect _ContentRectRight(const Rect& bounds) {
        const int w = bounds.w()-(_SeparatorX(bounds)+3)-_Inset.x;
        const int h = bounds.h()-2*_Inset.y-_ContentInsetTop-_ContentInsetBottom;
        return {
            .origin = {_SeparatorX(bounds)+3, _Inset.y+_ContentInsetTop},
            .size = {w, h},
        };
    }
    
    const std::vector<std::string>& _hunkLinesGet(const Git::FileConflict::Hunk& hunk, bool left, bool main) {
        switch (hunk.type) {
        case Git::FileConflict::Hunk::Type::Normal:
            return hunk.normal.lines;
        case Git::FileConflict::Hunk::Type::Conflict:
            switch (_layout) {
            case Layout::LeftOurs:
                return (left ? hunk.conflict.linesOurs : hunk.conflict.linesTheirs);
            case Layout::RightOurs:
                return (left ? hunk.conflict.linesTheirs : hunk.conflict.linesOurs);
            default: abort();
            }
        default: abort();
        }
    }
    
    void _contentTextDraw(const Rect& rect, bool left) {
        const auto& hunks = _fileConflict.hunks;
        auto hunkBegin = std::begin(hunks);
        auto hunkEnd = std::end(hunks);
        auto hunkRend = std::rend(hunks);
        
        auto& mainConflictLines = _hunkLinesGet(hunks[_hunkIdx], left, true);
        const int mainConflictStartY = rect.t() + std::max(0, (rect.h()-(int)mainConflictLines.size())/2);
//        const int mainConflictStartY = Inset.y + std::max(0, (h-(int)mainConflictLines.size())/2);
        
        // Draw main conflict section, and lines beneath it
        {
            bool main = true;
            int offY = mainConflictStartY;
            for (auto hunkIter=std::begin(hunks)+_hunkIdx; hunkIter!=hunkEnd && offY<rect.b(); hunkIter++) {
                const std::vector<std::string>& lines = _hunkLinesGet(*hunkIter, left, main);
                const Attr color = attr(main ? colors().conflictTextMain : colors().conflictTextDim);
                for (auto it=lines.begin(); it!=lines.end() && offY<rect.b(); it++) {
                    const std::string& line = *it;
                    // Draw highlighted-text background
                    if (main) drawLineHoriz({rect.l()-1, offY}, rect.w()+2, ' ');
                    drawText({rect.l(), offY}, rect.w(), line.c_str());
                    offY++;
                }
                main = false;
            }
        }
        
        // Draw lines above the main conflict section
        {
            const Attr color = attr(colors().conflictTextDim);
            int offY = mainConflictStartY-1;
            for (auto hunkIter=std::make_reverse_iterator(std::begin(hunks)+_hunkIdx); hunkIter!=hunkRend && offY>=rect.t(); hunkIter++) {
                const std::vector<std::string>& lines = _hunkLinesGet(*hunkIter, left, false);
                for (auto it=lines.rbegin(); it!=lines.rend() && offY>=rect.t(); it++) {
                    const std::string& line = *it;
                    drawText({rect.l(), offY}, rect.w(), line.c_str());
                    offY--;
                }
            }
        }
    }
    
    const Layout _layout = Layout::LeftOurs;
    const Git::FileConflict& _fileConflict;
    const size_t _hunkIdx = 0;
    
    LabelPtr _title = subviewCreate<Label>();
    LabelPtr _refNameLeft = subviewCreate<Label>();
    LabelPtr _refNameRight = subviewCreate<Label>();
    ButtonPtr _chooseButtonLeft = subviewCreate<Button>();
    ButtonPtr _chooseButtonRight = subviewCreate<Button>();
    ButtonPtr _openInEditorButton = subviewCreate<Button>();
    ButtonPtr _cancelButton = subviewCreate<Button>();
};

using ConflictPanelPtr = std::shared_ptr<ConflictPanel>;

} // namespace UI
