#pragma once
#include <optional>
#include "Panel.h"
#include "Color.h"

namespace UI {

class ModalPanel : public Panel {
public:
    ModalPanel(const ColorPalette& colors) : colors(colors) {}
    
//    bool layoutNeeded() {
//        if (_layoutNeeded) return true;
//        
//        if (width <= 0) return false;
//        Size sizeCur = size();
//        Size sizeNew = _calcSize();
//        if (sizeNew==_sizePrev && sizeNew==sizeCur) return false;
//        
//        _layoutNeeded = true;
//        return true;
//    }
    
    Size sizeIntrinsic() override {
        std::vector<std::string> messageLines = _createMessageLines();
        return {
            .x = width,
            .y = 2*_MessageInsetY + messageInsetY + (int)messageLines.size() + extraHeight,
        };
    }
    
    void layout() override {
        if (!layoutNeeded) return;
        Panel::layout();
        
        _messageLines = _createMessageLines();
    }
    
    void draw() override {
        if (!drawNeeded) return;
        Panel::draw();
        
        erase();
        
        int offY = _MessageInsetY-1; // -1 because the title overwrites the border
        {
            Window::Attr style = attr(color);
            drawRect(Inset(bounds(), {2,1}));
            drawRect(bounds());
        }
        
        {
            Window::Attr style = attr(color|A_BOLD);
            drawText({_MessageInsetX, offY}, " %s ", title.c_str());
            offY++;
        }
        
        offY += messageInsetY;
        
        for (const std::string& line : _messageLines) {
            int offX = 0;
            if (center) {
                offX = (bounds().size.x-(int)UTF8::Strlen(line)-2*_MessageInsetX)/2;
            }
            drawText({_MessageInsetX+offX, offY}, "%s", line.c_str());
            offY++;
        }
    }
    
    bool handleEvent(const Event& ev) override {
        // Dismiss upon mouse-up
        if (ev.mouseUp()) return false;
        // Dismiss upon escape key
        if (ev.type == Event::Type::KeyEscape) return false;
        // Eat all other events
        return true;
    }
    
    const ColorPalette& colors;
    Color color;
    int width = 0;
    int messageInsetY = 0;
    int extraHeight = 0;
    bool center = false;
    std::string title;
    std::string message;
    
protected:
    static constexpr int _MessageInsetX = 5;
    static constexpr int _MessageInsetY = 2;
    
private:
//    Size _calcSize() {
//        std::vector<std::string> messageLines = _createMessageLines();
//        return {
//            .x = width,
//            .y = 2*_MessageInsetY + messageInsetY + (int)messageLines.size() + extraHeight,
//        };
//    }
    
    std::vector<std::string> _createMessageLines() const {
        return LineWrap::Wrap(SIZE_MAX, width-2*_MessageInsetX, message);;
    }
    
//    bool _layoutNeeded = false;
//    Size _sizePrev;
    std::vector<std::string> _messageLines;
};

using ModalPanelPtr = std::shared_ptr<ModalPanel>;

} // namespace UI
