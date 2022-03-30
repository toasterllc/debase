#pragma once
#include <optional>
#include "Panel.h"
#include "Color.h"
#include "Attr.h"
#include "MessagePanel.h"

namespace UI {

class _RegisterPanel : public _MessagePanel {
public:
    _RegisterPanel(const MessagePanelOptions& opts) : _MessagePanel(_AdjustedOptions(opts)) {}
    
    void draw() override {
        const MessagePanelOptions& opts = options();
        int offY = size().y-_FieldsExtraHeight-1;
        _MessagePanel::draw();
        
        {
            UI::Attr attr(shared_from_this(), opts.color|A_BOLD);
            drawText({_MessageInsetX, offY}, "%s", "Email: ");
            offY += 2;
        }
        
        {
            UI::Attr attr(shared_from_this(), opts.color|A_BOLD);
            drawText({_MessageInsetX, offY}, "%s", "Code: ");
            offY += 2;
        }
        
//        char buf[128];
//        wgetnstr(*this, buf, sizeof(buf));
//        mvwgetnstr(*this, offY-2, _MessageInsetX, buf, sizeof(buf));
    }
    
private:
    static constexpr int _FieldsExtraHeight = 4;
    
    static MessagePanelOptions _AdjustedOptions(const MessagePanelOptions& x) {
        MessagePanelOptions opts = x;
        opts.extraHeight = _FieldsExtraHeight;
        return opts;
    }
};

using RegisterPanel = std::shared_ptr<_RegisterPanel>;

} // namespace UI
