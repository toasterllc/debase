#pragma once
#include <algorithm>
#include "Panel.h"
#include "Button.h"
#include "UTF8.h"

namespace UI {

class SnapshotMenu : public Menu {
public:
    using Menu::Menu;
    
    void draw() override {
        Menu::draw();
        
        // Draw separator
        const int width = size().x;
        if (erased()) { // Performance optimization: only draw if the window was erased
            if (visibleButtonCount() > 1) {
                ButtonPtr button0 = buttons()[0];
                Attr color = attr(colors().menu);
                Point p = {1, button0->frame().b()};
                int len = width-2;
                
                constexpr cchar_t Chars = { 0, L"╍" };
//                cchar_t c = {};
//                const wchar_t chars[] = L"╍";
//                static_assert(sizeof(chars) <= sizeof(c.chars));
//                // GCC has a weird bug where we can't initialize `c` with designated-initializer
//                // syntax, without using extra braces that Clang can't compile. So just memcpy it.
//                memcpy(c.chars, chars, sizeof(chars));
                
                mvwhline_set(*this, p.y, p.x, &Chars, len);
            }
        }
    }
    
    size_t visibleButtonCount() {
        size_t count = 0;
        for (UI::ButtonPtr button : buttons()) {
            if (!button->visible()) break;
            count++;
        }
        return count;
    }
};

using SnapshotMenuPtr = std::shared_ptr<SnapshotMenu>;

} // namespace UI
