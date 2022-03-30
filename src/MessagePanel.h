#pragma once
#include <optional>
#include "Panel.h"
#include "Color.h"
#include "Attr.h"

namespace UI {

struct MessagePanelOptions {
    Color color;
    int width = 0;
    int extraHeight = 0;
    bool center = false;
    std::string title;
    std::string message;
};

class _MessagePanel : public _Panel, public std::enable_shared_from_this<_MessagePanel> {
public:
    _MessagePanel(const MessagePanelOptions& opts) : _opts(opts) {
        _message = LineWrap::Wrap(SIZE_MAX, _opts.width-2*_MessageInsetX, _opts.message);
        int height = 2*_MessageInsetY + (int)_message.size() + _opts.extraHeight;
        setSize({_opts.width, height});
        _drawNeeded = true;
    }
    
    virtual void draw() {
        {
            UI::Attr attr(shared_from_this(), _opts.color);
            drawRect(Inset(bounds(), {2,1}));
            drawRect(bounds());
        }
        
        {
            UI::Attr attr(shared_from_this(), _opts.color|A_BOLD);
            drawText({_MessageInsetX, _MessageInsetY-1}, " %s ", _opts.title.c_str());
        }
        
        int i = 0;
        for (const std::string& line : _message) {
            int offX = 0;
            if (_opts.center) {
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
    
    const MessagePanelOptions& options() { return _opts; }
    
protected:
    static constexpr int _MessageInsetX = 4;
    static constexpr int _MessageInsetY = 2;
    
private:
    MessagePanelOptions _opts;
    std::vector<std::string> _message;
    bool _drawNeeded = false;
};

using MessagePanel = std::shared_ptr<_MessagePanel>;

} // namespace UI
