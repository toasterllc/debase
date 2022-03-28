#pragma once
#include <algorithm>
#include "Panel.h"
#include "Button.h"
#include "UTF8.h"

namespace UI {

struct SnapshotMenuOptions {
    bool sessionStart = false;
};

class _SnapshotMenu : public _Menu {
public:
    _SnapshotMenu(const SnapshotMenuOptions& snapOpts, const MenuOptions& menuOpts) :
    _Menu(menuOpts), _opts(snapOpts) {}
    
    void draw() {
        const MenuOptions& opts = options();
        _Menu::draw();
        const int width = bounds().size.x;
        
        // Draw separator
        if (_opts.sessionStart && opts.buttons.size()>1) {
            UI::Button button0 = opts.buttons[0];
            UI::Attr attr(shared_from_this(), opts.colors.menu);
//            drawLineHoriz({0, button0->options().frame.ymax()+1}, width);
            Point p = {1, button0->options().frame.ymax()+1};
            int len = width-2;
//            mvwhline(*this, p.y, p.x+1, '-', len);
            cchar_t c = { .chars = L"╌" };
            mvwhline_set(*this, p.y, p.x, &c, len);
        }
    }
    
private:
    SnapshotMenuOptions _opts;
};

using SnapshotMenu = std::shared_ptr<_SnapshotMenu>;

} // namespace UI
