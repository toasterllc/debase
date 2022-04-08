#include "Window.h"

namespace UI {

bool View::_winErased(const Window& win) {
    return win.erased();
}

void View::drawBackground(const Window& win) {
    if (_borderColor) {
        Window::Attr color = win.attr(*_borderColor);
        win.drawRect(frame());
    }
}

} // namespace UI
