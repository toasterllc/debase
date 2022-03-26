#pragma once
#include <algorithm>
#include "Window.h"

namespace UTF8 {

inline size_t Strlen(std::string_view str) {
    size_t r = 0;
    for (uint8_t b : str) {
        if ((b & 0xC0) != 0x80) {
            r++;
        }
    }
    return r;
}

} // namespace UTF8
