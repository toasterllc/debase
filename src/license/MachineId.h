#pragma once
#include <filesystem>
#include <sstream>
#include <sys/stat.h>
#include <unistd.h>
#include "SHA512Half.h"

namespace License {

inline uint64_t _RAMCapacity() {
    long pageCount = sysconf(_SC_PHYS_PAGES);
    if (pageCount < 0) return 0;
    long pageSize = sysconf(_SC_PAGESIZE);
    if (pageSize < 0) return 0;
    return (uint64_t)pageCount * (uint64_t)pageSize;
}

using MachineId = std::string;

inline MachineId MachineIdCalc(std::string_view domain) noexcept {
    namespace Fs = std::filesystem;
    constexpr const char* Separator = ":";
    
    std::stringstream stream;
    stream << domain << ":";
    
    const Fs::path paths[] = {
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
    
    for (const Fs::path& path : paths) {
        struct stat info;
        int ir = 0;
        do ir = stat(path.c_str(), &info);
        while (ir && errno==EINTR);
        
        const ino_t inode = (ir==0 ? info.st_ino : 0);
        stream << path.c_str() << "=" << std::to_string(inode) << Separator;
    }
    
    stream << "ram=" << std::to_string(_RAMCapacity()) << Separator;
    
    SHA512Half::Hash hash = SHA512Half::Calc(stream.str());
    return SHA512Half::StringForHash(hash);
}

} // namespace License
