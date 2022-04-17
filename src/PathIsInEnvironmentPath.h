#pragma once
#include <filesystem>

inline bool PathIsInEnvironmentPath(const std::filesystem::path& x) noexcept {
    using namespace std::filesystem;
    
    std::filesystem::path dirPath = x;
    
    // fs::canonical() will throw if the path doesn't exist, but we want to ignore that
    // and still offer string comparison
    try {
        dirPath = std::filesystem::canonical(x);
    } catch (...) {}
    
//    struct stat dirInfo;
//    try {
//        dirInfo = Stat(dir);
//    } catch (...) {
//        return false;
//    }
    
    const char* pathEnv = getenv("PATH");
    if (!pathEnv) return false;
    
    const std::vector<std::string> pathStrs = Toastbox::StringSplit(pathEnv, ":");
    for (const std::string& pathStr : pathStrs) {
        if (pathStr.empty()) continue;
        
        std::filesystem::path path = pathStr;
        // First do direct string comparison
        if (path == dirPath) return true;
        
        try {
            // ::canonical throws if it doesn't exist; just continue if that happens
            path = std::filesystem::canonical(pathStr);
            // Perform string comparison on canonical result too
            if (path == dirPath) return true;
            if (std::filesystem::equivalent(path, dirPath)) return true;
        } catch (...) {}
        
//        if ()
//        
//        
//        
//        if ()
//        
//        struct stat envDirInfo;
//        try {
//            envDirInfo = Stat(pathStr);
//        } catch (...) {
//            continue;
//        }
//        
//        // Check if current PATH dir has the same vnode/inode as `dir`
//        if (envDirInfo.st_dev==dirInfo.st_dev && envDirInfo.st_ino==dirInfo.st_ino) {
//            return true;
//        }
    }
    
    return false;
}






//// _CurrentExecutableIsInEnvironmentPath() returns true if the current executable 
//static bool _CurrentExecutableIsInEnvironmentPath(const std::filesystem::path& ) {
//    using namespace std::filesystem;
//    
//    const path execPath = CurrentExecutablePath();
//    const path execDir = execPath.parent_path();
//    struct stat execDirInfo;
//    try {
//        execDirInfo = Stat(execDir);
//    } catch (...) {
//        return false;
//    }
//    
//    const char* pathEnv = getenv("PATH");
//    if (!pathEnv) return false;
//    
//    const std::vector<std::string> pathStrs = Toastbox::StringSplit(pathEnv, ":");
//    for (const std::string& str : pathStrs) {
//        const path envDir = str;
//        
//        struct stat envDirInfo;
//        try {
//            envDirInfo = Stat(envDir);
//        } catch (...) {
//            continue;
//        }
//        
//        // Compare
//        if (envDirInfo.st_dev==execDirInfo.st_dev && envDirInfo.st_ino==execDirInfo.st_ino) {
//            return true;
//        }
//    }
//    
//    return false;
//}
