#pragma once
#include <sstream>

inline std::string _RuntimeErrorFmtMsg(const char* str) {
    char msg[256];
    int sr = snprintf(msg, sizeof(msg), "%s", str);
    if (sr<0 || (size_t)sr>=(sizeof(msg)-1)) throw std::runtime_error("failed to create RuntimeError");
    return msg;
}

template <typename ...Args>
inline std::string _RuntimeErrorFmtMsg(const char* fmt, Args&&... args) {
    char msg[256];
    int sr = snprintf(msg, sizeof(msg), fmt, std::forward<Args>(args)...);
    if (sr<0 || (size_t)sr>=(sizeof(msg)-1)) throw std::runtime_error("failed to create RuntimeError");
    return msg;
}

template <typename ...Args>
inline std::runtime_error RuntimeError(const char* fmt, Args&&... args) {
    return std::runtime_error(_RuntimeErrorFmtMsg(fmt, std::forward<Args>(args)...));
}
