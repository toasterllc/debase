#pragma once
#include <filesystem>

inline bool PathIsInEnvironmentPath(const std::filesystem::path& dir) {
    using namespace std::filesystem;
    
    struct stat dirInfo;
    try {
        dirInfo = Stat(dir);
    } catch (...) {
        return false;
    }
    
    const char* pathEnv = getenv("PATH");
    if (!pathEnv) return false;
    
    const std::vector<std::string> pathStrs = Toastbox::StringSplit(pathEnv, ":");
    for (const std::string& str : pathStrs) {
        const path envDir = str;
        
        struct stat envDirInfo;
        try {
            envDirInfo = Stat(envDir);
        } catch (...) {
            continue;
        }
        
        // Check if current PATH dir has the same vnode/inode as `dir`
        if (envDirInfo.st_dev==dirInfo.st_dev && envDirInfo.st_ino==dirInfo.st_ino) {
            return true;
        }
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
