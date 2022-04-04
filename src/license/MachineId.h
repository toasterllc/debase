#pragma once
#include <filesystem>
#include <sys/stat.h>
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
    
    std::stringstream stream;
    for (const fs::path& path : paths) {
        struct stat info;
        int ir = 0;
        do ir = stat(path.c_str(), &info);
        while (ir && errno==EINTR);
        
        const ino_t inode = (ir==0 ? info.st_ino : 0);
        stream << path << "=" << std::to_string(inode) << Separator;
    }
    
    stream << "ram=" << std::to_string(_RAMCapacity()) << Separator;
    
    std::string str = stream.str();
    sha512_state s;
    sha512half_init(&s);
    const uint8_t* data = (const uint8_t*)str.data();
    const uint8_t* dataEnd = data+str.size();
    for (; data<dataEnd; data+=SHA512_BLOCK_SIZE) {
        sha512_block(&s, data);
    }
    sha512_final(&s, data, str.size());
    
    return "";
}

} // namespace License
