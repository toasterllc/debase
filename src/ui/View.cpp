#include "Window.h"

namespace UI {

bool View::_windowErased(const Window& window) const {
    return window.erased();
}

WINDOW* View::_gstateWindow() const {
    assert(_GState.window);
    return *_GState.window;
}

} // namespace UI
