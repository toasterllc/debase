#pragma once
#include <algorithm>
#include "Window.h"

namespace UTF8 {

inline bool CodepointStart(uint8_t x) {
    return (x & 0xC0) != 0x80;
}

inline size_t Strlen(std::string_view str) {
    size_t r = 0;
    for (uint8_t b : str) {
        if (CodepointStart(b)) {
            r++;
        }
    }
    return r;
}

inline size_t Strlen(std::string::const_iterator start, std::string::const_iterator end) {
    size_t r = 0;
    for (auto it=start; it!=end; it++) {
        uint8_t b = *it;
        if (CodepointStart(b)) {
            r++;
        }
    }
    return r;
}

template <typename T_Iter>
inline T_Iter Next(T_Iter it, T_Iter end) {
    for (;;) {
        if (it == end) return it;
        it++;
        if (CodepointStart(*it)) return it;
    }
}

template <typename T_Iter>
inline T_Iter Prev(T_Iter it, T_Iter begin) {
    for (;;) {
        if (it == begin) return it;
        it--;
        if (CodepointStart(*it)) return it;
    }
}

} // namespace UTF8
