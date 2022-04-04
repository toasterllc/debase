#pragma once
#include <filesystem>
#include <sys/stat.h>

namespace License {

inline uint64_t _RAMCapacity() {
    long pageCount = sysconf(_SC_PHYS_PAGES);
    if (pageCount < 0) return 0;
    long pageSize = sysconf(_SC_PAGESIZE);
    if (pageSize < 0) return 0;
    return (uint64_t)pageCount * (uint64_t)pageSize;
}

using MachineId = std::string;

inline MachineId MachineIdGet() {
    constexpr const char* Separator = ":";
    
    namespace fs = std::filesystem;
    const fs::path paths[] = {
        // Directories
        "/bin",
        "/etc",
        "/home",
        "/opt",
        "/root",
        "/sbin",
        "/usr",
        "/usr/bin",
        "/usr/lib",
        "/usr/local",
        "/usr/sbin",
        "/var",
        // Binaries
        "/bin/ls",
        "/bin/whoami",
        "/bin/touch",
        "/bin/mkdir",
    };
    
    std::stringstream str;
    for (const fs::path& path : paths) {
        struct stat info;
        int ir = 0;
        do ir = stat(path.c_str(), &info);
        while (ir && errno==EINTR);
        
        const ino_t inode = (ir==0 ? info.st_ino : 0);
        str << path << "=" << std::to_string(inode) << Separator;
    }
    
    str << "ram=" << std::to_string(_RAMCapacity()) << Separator;
    
    return "";
}

} // namespace License
