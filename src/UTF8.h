#pragma once
#include <algorithm>
#include <string>

namespace UTF8 {

inline bool CodepointStart(uint8_t x) {
    return (x & 0xC0) != 0x80;
}

inline size_t Len(std::string_view str) {
    size_t r = 0;
    for (uint8_t b : str) {
        if (CodepointStart(b)) {
            r++;
        }
    }
    return r;
}

template <typename T_Iter>
inline size_t Len(T_Iter start, T_Iter end) {
    size_t r = 0;
    for (auto it=start; it!=end; it++) {
        uint8_t b = *it;
        if (CodepointStart(b)) {
            r++;
        }
    }
    return r;
}

inline std::string Truncated(std::string_view str, size_t len) {
    if (!len) return {};
    // If the string byte length is already less than `len` runes, then it's already truncated
    if (str.size() <= len) return std::string(str);
    
    std::string r;
    r.reserve(len); // Approximate space, assuming `str` is ASCII
    for (uint8_t b : str) {
        if (CodepointStart(b)) {
            if (!len) break;
            len--;
        }
        
        r.push_back(b);
    }
    return r;
}

template <typename T_Iter>
inline T_Iter NextN(T_Iter it, T_Iter end, ssize_t n) {
    if (n > 0) {
        while (n) {
            if (it == end) return it;
            it++;
            if (CodepointStart(*it)) n--;
        }
    
    } else if (n < 0) {
        while (n) {
            if (it == end) return it;
            it--;
            if (CodepointStart(*it)) n++;
        }
    }
    return it;
}

template <typename T_Iter>
inline T_Iter Next(T_Iter it, T_Iter end) {
    return NextN(it, end, 1);
}

template <typename T_Iter>
inline T_Iter Prev(T_Iter it, T_Iter end) {
    return NextN(it, end, -1);
}

} // namespace UTF8
