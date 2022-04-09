#include "Window.h"

namespace UI {

bool View::_winErased(const Window& win) const {
    return win.erased();
}

WINDOW* View::_drawWin() const {
    assert(_TState);
    return *_TState.win();
}

} // namespace UI
