#pragma once
#include <algorithm>
#include "Panel.h"
#include "Button.h"
#include "UTF8.h"

namespace UI {

class SnapshotMenu : public Menu {
public:
    using Menu::Menu;
    
    void draw() {
        const MenuOptions& opts = options();
        Menu::draw();
        const int width = bounds().size.x;
        
        // Draw separator
        if (opts.buttons.size() > 1) {
            UI::ButtonPtr button0 = opts.buttons[0];
            UI::Window::Attr color = attr(opts.colors.menu);
            Point p = {1, button0->frame.ymax()+1};
            int len = width-2;
            cchar_t c = { .chars = L"╍" };
            mvwhline_set(*this, p.y, p.x, &c, len);
        }
    }
};

using SnapshotMenuPtr = std::shared_ptr<SnapshotMenu>;

} // namespace UI
