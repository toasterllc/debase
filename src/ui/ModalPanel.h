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
    
    enum class TextAlign {
        Left,
        Center,
        CenterSingleLine
    };
    
    static constexpr int BorderSize() { return 2; }
    static constexpr int MessageInsetX() { return BorderSize()+3; }
    
    Size sizeIntrinsic() override {
        std::vector<std::string> messageLines = _createMessageLines();
        return {
            .x = _width,
            .y = 2*BorderSize() + _messageInsetY + (int)messageLines.size(),
        };
    }
    
    bool layout() override {
        if (!Panel::layout()) return false;
        _messageLines = _createMessageLines();
        return true;
    }
    
    bool draw() override {
        if (!Panel::draw()) return false;
        
        if (erased()) {
            int offY = BorderSize()-1; // -1 because the title overwrites the border
            {
                Window::Attr style = attr(_color);
                drawRect(Inset(bounds(), {2,1}));
                drawRect(bounds());
            }
            
            // Draw title
            if (!_title.empty()) {
                Window::Attr style = attr(_color|A_BOLD);
                drawText({MessageInsetX(), offY}, " %s ", _title.c_str());
                offY++;
            }
            offY += _messageInsetY;
            
            // Draw message
            for (const std::string& line : _messageLines) {
                int offX = 0;
                if (_textAlign==TextAlign::Center || (_textAlign==TextAlign::CenterSingleLine && _messageLines.size()==1)) {
                    offX = (bounds().size.x-(int)UTF8::Strlen(line)-2*MessageInsetX())/2;
                }
                drawText({MessageInsetX()+offX, offY}, line.c_str());
                offY++;
            }
        }
        
        return true;
    }
    
    bool handleEvent(const Event& ev) override {
        // Dismiss upon mouse-up
        if (ev.mouseUp(Event::MouseButtons::Left|Event::MouseButtons::Right)) return false;
        // Dismiss upon escape key
        if (ev.type == Event::Type::KeyEscape) return false;
        // Eat all other events
        return true;
    }
    
    const ColorPalette& colors;
    
    auto color() const { return _color; }
    template <typename T> void color(const T& x) { _set(_color, x); }
    
    auto width() const { return _width; }
    template <typename T> void width(const T& x) { _set(_width, x); }
    
    auto messageInsetY() const { return _messageInsetY; }
    template <typename T> void messageInsetY(const T& x) { _set(_messageInsetY, x); }
    
    auto textAlign() const { return _textAlign; }
    template <typename T> void textAlign(const T& x) { _set(_textAlign, x); }
    
    auto title() const { return _title; }
    template <typename T> void title(const T& x) { _set(_title, x); }
    
    auto message() const { return _message; }
    template <typename T> void message(const T& x) { _set(_message, x); }
    
private:
    std::vector<std::string> _createMessageLines() const {
        return LineWrap::Wrap(SIZE_MAX, _width-2*MessageInsetX(), _message);
    }
    
    Color _color;
    int _width = 0;
    int _messageInsetY = 0;
    TextAlign _textAlign = TextAlign::Left;
    std::string _title;
    std::string _message;
    std::vector<std::string> _messageLines;
};

using ModalPanelPtr = std::shared_ptr<ModalPanel>;

} // namespace UI
