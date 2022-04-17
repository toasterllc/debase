#pragma once
#include <filesystem>

inline std::filesystem::path UserBinRelativePath() noexcept {
#if __APPLE__
    return "bin";
#elif __linux__
    // Following the XDG Base Directory Specification
    return ".local/bin";
#else
    #error Invalid platform
#endif
}
