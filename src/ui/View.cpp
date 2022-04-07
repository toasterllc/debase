#include "Window.h"

namespace UI {

bool View::_winErased(const Window& win) {
    return win.erased();
}

Event View::_winNextEvent(const Window& win) {
    return win.nextEvent();
}

} // namespace UI
