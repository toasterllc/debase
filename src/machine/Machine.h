#pragma once
#include <filesystem>
#include <sstream>
#include <sys/stat.h>
#include <unistd.h>
#include "SHA512Half.h"
#include "Syscall.h"

namespace Machine {

using MachineId = std::string;
using MachineInfo = std::string;

// Platform-specific implementations
std::string MachineIdContent() noexcept;
MachineInfo MachineInfoCalc() noexcept;

inline uint64_t _RAMCapacity() {
    long pageCount = sysconf(_SC_PHYS_PAGES);
    if (pageCount < 0) return 0;
    long pageSize = sysconf(_SC_PAGESIZE);
    if (pageSize < 0) return 0;
    return (uint64_t)pageCount * (uint64_t)pageSize;
}

inline std::string MachineIdContentBasic() noexcept {
    namespace Fs = std::filesystem;
    constexpr const char* Separator = ":";
    
    std::stringstream stream;
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
        ino_t inode = 0;
        try {
            const struct stat info = Stat(path);
            inode = info.st_ino;
        } catch (...) {}
        
        stream << path.c_str() << "=" << std::to_string(inode) << Separator;
    }
    
    stream << "ram=" << std::to_string(_RAMCapacity()) << Separator;
    return stream.str();
}

inline MachineId MachineIdCalc(std::string_view domain) noexcept {
    namespace Fs = std::filesystem;
    const std::string str = std::string(domain) + ":" + MachineIdContent();
    return SHA512Half::StringForHash(SHA512Half::Calc(str));
}

} // namespace Machine
