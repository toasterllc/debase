#pragma once
#include <string>
#include "git/Conflict.h"



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
    
    ConflictPanel(const std::filesystem::path& path, Layout layout,
        const Git::FileConflict::Hunk& hunk) : _layout(layout), _hunk(hunk) {
        
        borderColor(colors().error);
        
        _title->inhibitErase(true); // Title overlaps border, so don't erase
        _title->textAttr(colors().error|WA_BOLD);
        _title->prefix(" ");
        _title->suffix(" ");
        _title->text("Conflict: " + path.string());
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
    }
    
private:
    static constexpr int _TitleInset = 2;
    
    const Layout _layout = Layout::LeftOurs;
    const Git::FileConflict::Hunk& _hunk;
    
    LabelPtr _title = subviewCreate<Label>();
    ButtonPtr _chooseLeftButton = subviewCreate<Button>();
    ButtonPtr _chooseRightButton = subviewCreate<Button>();
    ButtonPtr _openInEditorButton = subviewCreate<Button>();
    ButtonPtr _cancelButton = subviewCreate<Button>();
};

using ConflictPanelPtr = std::shared_ptr<ConflictPanel>;

} // namespace UI
