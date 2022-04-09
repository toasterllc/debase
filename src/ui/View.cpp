#include "Window.h"

namespace UI {

bool View::_winErased(const Window& win) const {
    return win.erased();
}

WINDOW* View::_drawWin() const {
    assert(_treeState);
    return *_treeState.win();
}

Point View::_drawOrigin() const {
    assert(_treeState);
    return _treeState.origin();
}

} // namespace UI
