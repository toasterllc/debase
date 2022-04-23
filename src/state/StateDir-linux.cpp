#include "State.h"
#include "Debase.h"
#include "StateDir.h"
#include <filesystem>

std::filesystem::path StateDir() {
    const char* configDirEnv = getenv("XDG_CONFIG_HOME");
    std::filesystem::path configDir = (configDirEnv ? configDirEnv : "");
    if (configDir.empty()) {
        const char* homeEnv = getenv("HOME");
        if (!homeEnv) throw Toastbox::RuntimeError("HOME environment variable isn't set");
        configDir = std::filesystem::path(homeEnv) / ".config";
    }
    return configDir / DebaseProductId;
}
