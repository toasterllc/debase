#pragma once

inline bool XDGDetected() noexcept {
    return  getenv("XDG_DATA_DIRS")     ||
            getenv("XDG_RUNTIME_DIR")   ||
            getenv("XDG_SESSION_CLASS") ||
            getenv("XDG_SESSION_ID")    ||
            getenv("XDG_SESSION_TYPE")  ;
}
