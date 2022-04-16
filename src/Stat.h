#pragma once
#include <sys/stat.h>

inline struct stat Stat(const std::filesystem::path& p) {
    struct stat info;
    int ir = 0;
    do ir = stat(p.c_str(), &info);
    while (ir && errno==EINTR);
    
    if (ir) throw std::system_error(errno, std::generic_category());
    return info;
}
