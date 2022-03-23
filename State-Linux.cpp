#import "State.h"
#import "Debase.h"
#import <filesystem>

std::filesystem::path ConfigDir() {
    std::filesystem::path configDir = getenv("XDG_CONFIG_HOME");
    if (path.empty()) {
        std::filesystem::path home = getenv("HOME");
        if (home.empty()) throw Toastbox::RuntimeError("HOME environment variable isn't set");
        configDir = home / ".config";
    }
    return configDir / DebaseBundleId;
}
