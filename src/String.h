#pragma once

namespace String {

inline bool StartsWith(std::string_view prefix, std::string_view str) {
    if (str.size() < prefix.size()) return false;
    return std::equal(prefix.begin(), prefix.end(), str.begin());
}

inline bool EndsWith(std::string_view suffix, std::string_view str) {
    if (str.size() < suffix.size()) return false;
    return std::equal(suffix.rbegin(), suffix.rend(), str.rbegin());
}

} // namespace String
