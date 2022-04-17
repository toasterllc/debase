#import "ProcessPath.h"
#import <libproc.h>
#import <system_error>

std::filesystem::path ProcessPathGet(pid_t pid) {
    auto buf = std::make_unique<char[]>(PROC_PIDPATHINFO_MAXSIZE);
    int ir = proc_pidpath(pid, buf.get(), PROC_PIDPATHINFO_MAXSIZE);
    // proc_pidpath() returns the string length, and 0 upon error
    if (!ir) throw std::system_error(errno, std::generic_category());
    return std::filesystem::path(buf.get());
}
