#pragma once
#include <unistd.h>
#include <filesystem>
#include "ProcessPath.h"

inline std::filesystem::path CurrentExecutablePath() {
    return ProcessPathGet(getpid());
}
