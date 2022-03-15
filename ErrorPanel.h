#pragma once
#include <optional>
#include "Panel.h"
#include "Color.h"
#include "Attr.h"
#include "Color.h"

namespace UI {

class _ErrorPanel : public _Panel, public std::enable_shared_from_this<_ErrorPanel> {
public:
    _ErrorPanel(int width, std::string_view title, std::string_view message) : _title(title) {
        _message = LineWrap::Wrap(SIZE_MAX, width-2*_MessageInsetX, message);
//        setSize({10, 10});
        setSize({width, (int)_message.size()+2*_MessageInsetY});
        _drawNeeded = true;
    }
    
    void draw() {
//        drawRect(<#const Rect &rect#>)
        
        {
            UI::Attr attr(shared_from_this(), Colors::Error);
            drawRect(Inset(bounds(), {2,1}));
            drawRect(bounds());
//            drawBorder();
//            drawRect(Inset(rect(), {1,1}));
//            drawBorder();
        }
        
        {
            UI::Attr attr(shared_from_this(), Colors::Error);
            drawText({_MessageInsetX, _MessageInsetY-1}, " %s ", _title.c_str());
        }
        
        int i = 0;
        for (const std::string& line : _message) {
            drawText({_MessageInsetX, _MessageInsetY+i}, "%s", line.c_str());
            i++;
        }
        
        _drawNeeded = false;
    }
    
    void drawIfNeeded() {
        if (_drawNeeded) {
            draw();
        }
    }
    
private:
    static constexpr int _MessageInsetX = 4;
    static constexpr int _MessageInsetY = 2;
    
    std::string _title;
    std::vector<std::string> _message;
    bool _drawNeeded = false;
};

using ErrorPanel = std::shared_ptr<_ErrorPanel>;

} // namespace UI
