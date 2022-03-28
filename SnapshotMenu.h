#pragma once
#include <algorithm>
#include "Panel.h"
#include "Button.h"
#include "UTF8.h"

namespace UI {

class _SnapshotMenu : public _Menu {
public:
    using _Menu::_Menu;
    
    void draw() {
        const MenuOptions& opts = options();
        _Menu::draw();
        const int width = bounds().size.x;
        
        // Draw separator
        if (opts.buttons.size() > 1) {
            UI::Button button0 = opts.buttons[0];
            UI::Attr attr(shared_from_this(), opts.colors.menu);
//            drawLineHoriz({0, button0->options().frame.ymax()+1}, width);
            Point p = {1, button0->options().frame.ymax()+1};
            int len = width-2;
//            mvwhline(*this, p.y, p.x+1, '-', len);
            cchar_t c = { .chars = L"╍" };
            mvwhline_set(*this, p.y, p.x, &c, len);
        }
    }
};

using SnapshotMenu = std::shared_ptr<_SnapshotMenu>;

} // namespace UI
