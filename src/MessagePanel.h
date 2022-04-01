#pragma once
#include <optional>
//#include <os/log.h>
#include "Panel.h"
#include "Color.h"

namespace UI {

struct MessagePanelOptions {
    const ColorPalette& colors;
    Color color;
    int width = 0;
    int messageInsetY = 0;
    int extraHeight = 0;
    bool center = false;
    std::string title;
    std::string message;
};

class _MessagePanel : public _Panel, public std::enable_shared_from_this<_MessagePanel> {
public:
    _MessagePanel(const MessagePanelOptions& opts) : _opts(opts) {}
    
    virtual bool layout() {
        if (_opts.width <= 0) return false;
        
        std::vector<std::string> message = LineWrap::Wrap(SIZE_MAX, _opts.width-2*_MessageInsetX, _opts.message);
        Size sizeCur = size();
        Size sizeNew = {
            .x = _opts.width,
            .y = 2*_MessageInsetY + _opts.messageInsetY + (int)message.size() + _opts.extraHeight,
        };
        
        if (sizeNew==_sizePrev && sizeNew==sizeCur) {
//            os_log(OS_LOG_DEFAULT, "sizeCur={%d,%d} sizeNew={%d,%d} DENY", sizeCur.x, sizeCur.y, sizeNew.x, sizeNew.y);
            return false;
        }
        
//        os_log(OS_LOG_DEFAULT, "sizeCur={%d,%d} sizeNew={%d,%d} ALLOW", sizeCur.x, sizeCur.y, sizeNew.x, sizeNew.y);
        
        _message = message;
        setSize(sizeNew);
        _sizePrev = sizeNew;
        _drawNeeded = true;
        return true;
    }
    
    virtual void draw() {
        erase();
        
        int offY = _MessageInsetY-1; // -1 because the title overwrites the border
        {
            UI::_Window::Attr color = attr(_opts.color);
            drawRect(Inset(bounds(), {2,1}));
            drawRect(bounds());
        }
        
        {
            UI::_Window::Attr color = attr(_opts.color|A_BOLD);
            drawText({_MessageInsetX, offY}, " %s ", _opts.title.c_str());
            offY++;
        }
        
        offY += _opts.messageInsetY;
        
        for (const std::string& line : _message) {
            int offX = 0;
            if (_opts.center) {
                offX = (bounds().size.x-(int)UTF8::Strlen(line)-2*_MessageInsetX)/2;
            }
            drawText({_MessageInsetX+offX, offY}, "%s", line.c_str());
            offY++;
        }
        
        _drawNeeded = false;
    }
    
    void drawIfNeeded() {
//        if (_drawNeeded) {
            draw();
//        }
    }
    
    bool drawNeeded() const { return _drawNeeded; }
    void drawNeeded(bool x) { _drawNeeded = x; }
    
    virtual UI::Event handleEvent(const UI::Event& ev) {
        // Let caller handle mouse-up's
        if (ev.mouseUp()) return ev;
        // Let caller handle escape key
        if (ev.type == UI::Event::Type::KeyEscape) return ev;
        // Let caller handle escape key
        if (ev.type == UI::Event::Type::WindowResize) return ev;
        // Let caller handle Ctrl-C/D
        if (ev.type == UI::Event::Type::KeyCtrlC) return ev;
        if (ev.type == UI::Event::Type::KeyCtrlD) return ev;
        // Eat all other events
        return {};
    }
    
    MessagePanelOptions& options() { return _opts; }
    
protected:
    static constexpr int _MessageInsetX = 5;
    static constexpr int _MessageInsetY = 2;
    
private:
    MessagePanelOptions _opts;
    Size _sizePrev;
    std::vector<std::string> _message;
    bool _drawNeeded = false;
};

using MessagePanel = std::shared_ptr<_MessagePanel>;

} // namespace UI
