#pragma once

namespace String {

inline bool StartsWith(std::string_view prefix, std::string_view str) {
    return !str.compare(0, prefix.size(), prefix);
}

} // namespace String
