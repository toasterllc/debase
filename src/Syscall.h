#pragma once
#include <sys/stat.h>
#include <sys/ioctl.h>

inline struct stat Stat(const std::filesystem::path& p) {
    struct stat info;
    int ir = 0;
    do ir = stat(p.c_str(), &info);
    while (ir && errno==EINTR);
    
    if (ir) throw std::system_error(errno, std::generic_category());
    return info;
}

template <typename ...Args>
inline void Ioctl(int fd, unsigned long req, Args&&... args) {
    int ir = 0;
    do ir = ioctl(fd, req, std::forward<Args>(args)...);
    while (ir && errno==EINTR);
    if (ir) throw std::system_error(errno, std::generic_category());
}
