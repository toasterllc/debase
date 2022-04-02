#pragma once
#include "State.h"

namespace State {

Theme ThemeRead() {
    State state(StateDir());
    Theme theme = state.theme();
    if (theme != Theme::None) return theme;
    
    bool write = false;
    // If a theme isn't set, ask the terminal for its background color,
    // and we'll choose the theme based on that
    Terminal::Background bg = Terminal::Background::Dark;
    try {
        bg = Terminal::BackgroundGet();
    } catch (...) {
        // We failed to get the terminal background color, so write the theme
        // for the default background color to disk, so we don't try to get
        // the background color again in the future. (This avoids a timeout
        // delay in Terminal::BackgroundGet() that occurs if the terminal
        // doesn't support querying the background color.)
        write = true;
    }
    
    switch (bg) {
    case Terminal::Background::Dark:    theme = Theme::Dark; break;
    case Terminal::Background::Light:   theme = Theme::Light; break;
    }
    
    if (write) {
        state.theme(theme);
        state.write();
    }
    return theme;
}

static void ThemeWrite(Theme theme) {
    State state(StateDir());
    state.theme(theme);
    state.write();
}

}
