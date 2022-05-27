#pragma once
#include <string>
#include "git/Conflict.h"

// TODO: draw veritcal lines to the left of the conflict region
// TODO: draw '---- empty ----' placeholder when conflict region is empty
// TODO: make filename title non-bold
// TODO: truncate the beginning of the filename, not the end

//struct FileConflict {
//    struct Hunk {
//        enum class Type {
//            Normal,
//            Conflict,
//        };
//        
//        Type type = Type::Normal;
//        
//        struct {
//            std::vector<std::string> lines;
//        } normal;
//        
//        struct {
//            std::vector<std::string> linesOurs;
//            std::vector<std::string> linesTheirs;
//        } conflict;
//        
//        bool empty() const {
//            switch (type) {
//            case Type::Normal:      return normal.lines.empty();
//            case Type::Conflict:    return conflict.linesOurs.empty() && conflict.linesTheirs.empty();
//            default: abort();
//            }
//        }
//    };
//    
//    std::filesystem::path path;
//    std::vector<Hunk> hunks;
//};





namespace UI {

class ConflictPanel : public Panel {
public:
    enum class Layout {
        LeftOurs,
        RightOurs,
    };
    
    ConflictPanel(Layout layout, const Git::FileConflict& fc, size_t hunkIdx) :
    _layout(layout), _fileConflict(fc), _hunkIdx(hunkIdx) {
        
        borderColor(colors().error);
        
        _title->inhibitErase(true); // Title overlaps border, so don't erase
        _title->textAttr(colors().error|WA_BOLD);
        _title->prefix(" ");
        _title->suffix(" ");
        _title->text("Conflict: " + fc.path.string());
    }
    
    Size sizeIntrinsic(Size constraint) override {
        return {std::min(75,constraint.x), std::min(25,constraint.y)};
    }
    
    using View::layout;
    void layout() override {
        View::layout();
        
        const Rect b = bounds();
        const Point titleOrigin = {_TitleInset,0};
        _title->frame({titleOrigin, {b.w()-2*_TitleInset, 1}});
    }
    
    using View::draw;
    void draw() override {
        View::draw();
        
        // Always redraw _title because our border may have clobbered it
        _title->drawNeeded(true);
        
        constexpr Size Inset = {2,1};
        const Rect b = bounds();
        const int h = b.h()-2*Inset.y;
        const int separatorX = b.w()/2;
        
        const Rect leftRect = {
            .origin = Inset,
            .size = {separatorX-Inset.x-1, h},
        };
        
        const Rect rightRect = {
            .origin = {separatorX+2, Inset.y},
            .size = {b.w()-(separatorX+2)-Inset.x, h},
        };
        
        {
            Attr color = attr(colors().error);
            drawLineVert({separatorX, 1}, b.h()-2);
        }
        
        _drawText(leftRect, true);
        _drawText(rightRect, false);
        
//        drawRect(leftRect);
//        drawRect(rightRect);
        
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
    static constexpr int _TitleInset = 2;
    
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
    
    void _drawText(const Rect& rect, bool left) {
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
    ButtonPtr _chooseLeftButton = subviewCreate<Button>();
    ButtonPtr _chooseRightButton = subviewCreate<Button>();
    ButtonPtr _openInEditorButton = subviewCreate<Button>();
    ButtonPtr _cancelButton = subviewCreate<Button>();
};

using ConflictPanelPtr = std::shared_ptr<ConflictPanel>;

} // namespace UI
