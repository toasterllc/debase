#include "Window.h"

namespace UI {

void View::drawBackground(const Window& win) {
    if (_borderColor) {
        Window::Attr color = win.attr(*_borderColor);
        win.drawRect(frame());
    }
}

bool View::_winErased(const Window& win) const {
    return win.erased();
}

WINDOW* View::_drawWin() const {
    return Window::DrawWindow();
}

} // namespace UI
