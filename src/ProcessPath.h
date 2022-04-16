#pragma once
#include <unistd.h>
#include <filesystem>

std::filesystem::path ProcessPathGet(pid_t pid);
