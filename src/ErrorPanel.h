#pragma once
#include <optional>
#include "Panel.h"
#include "Color.h"
#include "Attr.h"
#include "Color.h"

namespace UI {

class _ErrorPanel : public _Panel, public std::enable_shared_from_this<_ErrorPanel> {
public:
    _ErrorPanel(const ColorPalette& colors, int width, std::string_view title, std::string_view message) :
    _colors(colors), _title(title) {
        _message = LineWrap::Wrap(SIZE_MAX, width-2*_MessageInsetX, message);
        setSize({width, (int)_message.size()+2*_MessageInsetY});
        _drawNeeded = true;
    }
    
    void draw() {
        {
            UI::Attr attr(shared_from_this(), _colors.error);
            drawRect(Inset(bounds(), {2,1}));
            drawRect(bounds());
        }
        
        {
            UI::Attr attr(shared_from_this(), _colors.error);
            drawText({_MessageInsetX, _MessageInsetY-1}, " %s ", _title.c_str());
        }
        
        int i = 0;
        for (const std::string& line : _message) {
            int offX = 0;
            if (_message.size() == 1) {
                offX = (bounds().size.x-(int)UTF8::Strlen(line)-2*_MessageInsetX)/2;
            }
            drawText({_MessageInsetX+offX, _MessageInsetY+i}, "%s", line.c_str());
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
    
    const ColorPalette& _colors;
    std::string _title;
    std::vector<std::string> _message;
    bool _drawNeeded = false;
};

using ErrorPanel = std::shared_ptr<_ErrorPanel>;

} // namespace UI
