#include "ProcessPath.h"
#include <fstream>
#include <sstream>

std::filesystem::path ProcessPathGet(pid_t pid) {
    const std::filesystem::path link("/proc/" + std::to_string(pid) + "/exe");
    return std::filesystem::read_symlink(link);
}
