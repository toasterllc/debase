#pragma once
#include <string>
#include "git/Conflict.h"

namespace UI {

class ConflictPanel : public Panel {
public:
    enum class Layout {
        LeftOurs,
        RightOurs,
    };
    
    enum class Result {
        ChooseOurs,
        ChooseTheirs,
        OpenInEditor,
        Cancel,
    };
    
    ConflictPanel(
        Layout layout, std::string_view title,
        std::string_view refNameOurs, std::string_view refNameTheirs,
        const Git::FileConflict& fc, size_t hunkIdx) :
        _layout(layout), _fileConflict(fc), _hunkIdx(hunkIdx) {
        
        borderColor(colors().error);
        
        _title->inhibitErase(true); // Title overlaps border, so don't erase
        _title->textAttr(colors().error|WA_BOLD);
        _title->prefix(" ");
        _title->suffix(": ");
        _title->text(title);
        
        _titleFilePath->inhibitErase(true); // Title overlaps border, so don't erase
        _titleFilePath->textAttr(colors().error);
        _titleFilePath->suffix(" ");
        _titleFilePath->truncate(Truncate::Head);
        _titleFilePath->text(fc.path.string());
        
        _refNameLeft->align(Align::Center);
        _refNameLeft->textAttr(colors().error|WA_BOLD);
        _refNameLeft->text(_layout==Layout::LeftOurs ? refNameOurs : refNameTheirs);
        
        _refNameRight->align(Align::Center);
        _refNameRight->textAttr(colors().error|WA_BOLD);
        _refNameRight->text(_layout==Layout::LeftOurs ? refNameTheirs : refNameOurs);
        
        _chooseButtonLeft->bordered(true);
        _chooseButtonLeft->highlightColor(colors().error);
        _chooseButtonLeft->label()->text("Choose");
        _chooseButtonLeft->action( [&] (UI::Button& b) { _done(b); } );
//        _chooseButtonLeft->label()->align(UI::Align::Left);
//        _chooseButtonLeft->key()->text("◀");
        
        _chooseButtonRight->bordered(true);
        _chooseButtonRight->highlightColor(colors().error);
        _chooseButtonRight->label()->text("Choose");
        _chooseButtonRight->action( [&] (UI::Button& b) { _done(b); } );
//        _chooseButtonRight->label()->align(UI::Align::Left);
//        _chooseButtonRight->key()->text("▶");
        
        _openInEditorButton->bordered(true);
        _openInEditorButton->highlightColor(colors().error);
        _openInEditorButton->label()->text("Open in Editor");
        _openInEditorButton->action( [&] (UI::Button& b) { _done(b); } );
        
        _cancelButton->bordered(true);
        _cancelButton->highlightColor(colors().error);
        _cancelButton->label()->text("Cancel");
        _cancelButton->action( [&] (UI::Button& b) { _done(b); } );
    }
    
    Size sizeIntrinsic(Size constraint) override {
        Size s = constraint;
        const int paddingX = std::min(10, constraint.x/5);
        const int paddingY = std::min(6, constraint.y/5);
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
        _title->sizeToFit(ConstraintNoneSize);
        _title->origin({_TitleInsetX,0});
        _titleFilePath->frame({_title->frame().tr(), {b.w()-_title->frame().r()-_TitleInsetX, 1}});
        
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
    
//    auto& refNameOurs() { return (_layout==Layout::LeftOurs ? refNameOurs : refNameTheirs); }
//    auto& refNameTheirs() { return (_layout==Layout::LeftOurs ? refNameTheirs : refNameOurs); }
    
    auto& doneAction() { return _doneAction; }
    
    bool handleEvent(const Event& ev) override {
        // Whether window is enabled
        if (enabled()) {
            if (ev.type == Event::Type::KeyEscape) {
                _doneAction(Result::Cancel);
            }
        }
        return true;
    }
    
//    const auto& doneAction() const { return _doneAction; }
//    template <typename T> bool doneAction(const T& x) { return _setForce(_doneAction, x); }
    
private:
    static constexpr int _TitleInsetX = 2;
    static constexpr Size _Inset = {3,1};
    static constexpr int _ContentInsetTop = 1;
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
    
    const std::vector<std::string>& _hunkLinesGet(const Git::FileConflict::Hunk& hunk, bool left) const {
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
    
    int _maxLinesAboveConflict(bool left) const {
        const int h = bounds().h(); // No need to return values over our height
        const auto& hunks = _fileConflict.hunks;
        auto hunkRend = std::rend(hunks);
        int count = 0;
        for (auto hunkIter=std::make_reverse_iterator(std::begin(hunks)+_hunkIdx); hunkIter!=hunkRend && count<h; hunkIter++) {
            const std::vector<std::string>& lines = _hunkLinesGet(*hunkIter, left);
            for (auto it=lines.rbegin(); it!=lines.rend() && count<h; it++) {
                count++;
            }
        }
        return count;
    }
    
    // _ContentTextFilter(): filters non-printable ascii characters, and replaces tabs with spaces
    // so we can easily limit the horizontal width of the printed text
    static std::string _ContentTextFilter(size_t len, std::string_view str) {
        constexpr size_t TabSize = 8;
        
        std::string buf;
        buf.reserve(len); // Guesstimated size; will be larger if we have multi-byte UTF8 codepoints
        
        size_t i = 0;
        for (auto it=str.begin(); it!=str.end() && i<len;) {
            auto itNext = UTF8::Next(it, str.end());
            
            if (isascii(*it)) {
                // Handle printable ascii chars
                if (isprint(*it)) {
                    buf += *it;
                    i++;
                
                // Handle tab characters
                // Replace tabs with spaces, where the current column (i) determines the
                // number of spaces (emulating standard tab behavior)
                } else if (*it == '\t') {
                    const size_t spaceCount = TabSize - (i % TabSize);
                    // Limit the number of spaces so that we don't exceed `len` characters
                    const size_t spaceCountLimited = std::min(len-i, spaceCount);
                    buf += std::string(spaceCountLimited, ' ');
                    i += spaceCountLimited;
                }
            
            // Handle non-ascii UTF8 codepoints
            } else {
                buf.insert(buf.end(), it, itNext);
                i++;
            }
            
            it = itNext;
        }
        
        return buf;
    }
    
    void _contentTextDraw(const Point& origin, int width, std::string_view str) {
        const std::string strFiltered = _ContentTextFilter((size_t)width, str);
        drawText(origin, strFiltered.c_str());
        
//        std::vector<uint8_t> buf;
//        buf.reserve(width);
//        
//        size_t i = 0;
//        for (auto it=str.begin(); it!=str.end() && i<width;) {
//            auto itNext = UTF8::Next(it);
//            
//            if (isascii(*it)) {
//                if (isprint(*it)) {
//                    buf.push_back(*it);
//                    i++;
//                
//                // Specially handle tab characters
//                } else if (*it == '\t') {
//                    
//                }
//            } else {
//                buf.insert(buf.end(), it, itNext);
//                i++;
//            }
//            
//            it = itNext;
//        }
//        
//        auto it = str.begin();
//        auto itPrev = str.begin();
//        size_t i = 0;
//        while (it!=str.end() && i<width) {
//            if (isascii(*it)) {
//                if (isprint(*it)) {
//                    buf.push_back(*it);
//                    i++;
//                }
//            } else {
//                buf.insert(buf.end(), itPrev, it);
//                i++;
//            }
//            
//            itPrev = it;
//            it = UTF8::Next(it);
//        }
//        
//        for (; it!=str.end() && ; it=UTF8::Next(it)) {
//            if (isascii(*it)) {
//                if (isprint(*it)) {
//                    buf.push_back(*it);
//                }
//            } else {
//                buf.insert(buf.end(), itPrev, it);
//            }
//            itPrev = it;
//        }
//        
//        for (uint8_t ch : str) {
////            extern NCURSES_EXPORT(int) mvadd_wch (int, int, const cchar_t *);	/* generated:WIDEC */
////            mvwaddch
//        }
    }
    
    void _contentTextDraw(const Rect& rect, bool left) {
        const auto& hunks = _fileConflict.hunks;
        auto hunkEnd = std::end(hunks);
        auto hunkRend = std::rend(hunks);
        
        auto& mainConflictLines = _hunkLinesGet(hunks[_hunkIdx], left);
        const int maxLinesAboveConflict = std::min(_maxLinesAboveConflict(true), _maxLinesAboveConflict(false));
        const int mainConflictStartY = rect.t() + std::min(maxLinesAboveConflict, std::max(0, (rect.h()-(int)mainConflictLines.size())/2));
//        const int mainConflictStartY = Inset.y + std::max(0, (h-(int)mainConflictLines.size())/2);
        
        // Draw main conflict section, and lines beneath it
        {
            bool main = true;
            int offY = mainConflictStartY;
            for (auto hunkIter=std::begin(hunks)+_hunkIdx; hunkIter!=hunkEnd && offY<rect.b(); hunkIter++) {
                const std::vector<std::string>& lines = _hunkLinesGet(*hunkIter, left);
                const Attr color = attr(main ? colors().conflictTextMain : colors().conflictTextDim);
                if (!main || !lines.empty()) {
                    for (auto it=lines.begin(); it!=lines.end() && offY<rect.b(); it++) {
                        const std::string& line = *it;
                        // Draw highlighted-text background
                        if (main) drawLineHoriz({rect.l()-1, offY}, rect.w()+2, ' ');
                        _contentTextDraw({rect.l(), offY}, rect.w(), line.c_str());
                        offY++;
                    }
                
                } else {
                    // This is the main conflict region, and it's empty.
                    // Draw placeholder text
                    drawLineHoriz({rect.l(), offY}, rect.w());
                    constexpr const char NoFile[]   = " no file ";
                    constexpr const char Empty[]    = " empty ";
                    
                    // The file is considered deleted if the total number of lines==0. (Therefore
                    // an empty but existing file would have 1 line containing an empty string.)
                    // So if our FileConflict has one hunk and that hunk has zero lines, then it's
                    // a non-existent file.
                    const char* text = nullptr;
                    size_t textLen = 0;
                    
                    if (_fileConflict.noFileOurs() || _fileConflict.noFileTheirs()) {
                        text = NoFile;
                        textLen = std::size(NoFile)-1;
                    } else {
                        text = Empty;
                        textLen = std::size(Empty)-1;
                    }
                    
                    drawText({rect.l()+(rect.w()-((int)textLen))/2, offY}, text);
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
                const std::vector<std::string>& lines = _hunkLinesGet(*hunkIter, left);
                for (auto it=lines.rbegin(); it!=lines.rend() && offY>=rect.t(); it++) {
                    const std::string& line = *it;
                    _contentTextDraw({rect.l(), offY}, rect.w(), line.c_str());
                    offY--;
                }
            }
        }
    }
    
    void _done(UI::Button& b) {
        if (&b == &*_chooseButtonLeft) {
            _doneAction(_layout==Layout::LeftOurs ? Result::ChooseOurs : Result::ChooseTheirs);
        
        } else if (&b == &*_chooseButtonRight) {
            _doneAction(_layout==Layout::LeftOurs ? Result::ChooseTheirs : Result::ChooseOurs);
        
        } else if (&b == &*_openInEditorButton) {
            _doneAction(Result::OpenInEditor);
        
        } else if (&b == &*_cancelButton) {
            _doneAction(Result::Cancel);
        
        } else abort(); // UnknownButton
    }
    
    const Layout _layout = Layout::LeftOurs;
    const Git::FileConflict& _fileConflict;
    const size_t _hunkIdx = 0;
    
    LabelPtr _title = subviewCreate<Label>();
    LabelPtr _titleFilePath = subviewCreate<Label>();
    LabelPtr _refNameLeft = subviewCreate<Label>();
    LabelPtr _refNameRight = subviewCreate<Label>();
    ButtonPtr _chooseButtonLeft = subviewCreate<Button>();
    ButtonPtr _chooseButtonRight = subviewCreate<Button>();
    ButtonPtr _openInEditorButton = subviewCreate<Button>();
    ButtonPtr _cancelButton = subviewCreate<Button>();
    
    std::function<void(Result)> _doneAction;
};

using ConflictPanelPtr = std::shared_ptr<ConflictPanel>;

} // namespace UI
