#pragma once
#include <fstream>
#include "Git.h"
#include "lib/toastbox/Defer.h"
#include "lib/toastbox/FDStream.h"

namespace Git {

inline std::string _EditorCommand(const Repo& repo) {
    const std::optional<std::string> editorCmd = repo.config().stringGet("core.editor");
    if (editorCmd) return *editorCmd;
    if (const char* x = getenv("VISUAL")) return x;
    if (const char* x = getenv("EDITOR")) return x;
    return "vi";
}

struct _Argv {
    std::vector<std::string> args;
    std::vector<const char*> argv;
};

inline _Argv _ArgvCreate(const Repo& repo, std::string_view filePath) {
    const std::string editorCmd = _EditorCommand(repo);
    std::vector<std::string> args;
    std::vector<const char*> argv;
    std::istringstream ss(editorCmd);
    std::string arg;
    while (ss >> arg) args.push_back(arg);
    args.push_back(std::string(filePath));
    // Constructing `argv` must be a separate loop from constructing `args`,
    // because modifying `args` can invalidate all its elements, including
    // the c_str's that we'd get from each element
    for (const std::string& a : args) {
        argv.push_back(a.c_str());
    }
    argv.push_back(nullptr);
    return _Argv{std::move(args), std::move(argv)};
}

template <typename T_Spawn>
inline std::string EditorRun(const Repo& repo, T_Spawn spawnFn, std::string_view content) {
    // Write the commit message to `tmpFilePath`
    char tmpFilePath[] = "/tmp/debase.XXXXXX";
    int ir = mkstemp(tmpFilePath);
    if (ir < 0) throw RuntimeError("mkstemp failed: %s", strerror(errno));
    Defer(unlink(tmpFilePath)); // Delete the temporary file upon return
    
    // Write the content to the file
    {
        Toastbox::FDStreamInOut f(ir); // Takes ownership of fd `ir`
        f.exceptions(std::ios::failbit | std::ios::badbit);
        f << content;
    }
    
    // Spawn text editor
    _Argv argv = _ArgvCreate(repo, tmpFilePath);
    spawnFn(argv.argv.data());
    
    // Read back the edited content
    {
        std::ifstream f;
        f.exceptions(std::ifstream::badbit);
        f.open(tmpFilePath);
        
        std::stringstream ss;
        // TODO: this may copy a character at a time; optimize if needed
        ss << f.rdbuf();
        return ss.str();
    }
}

} // namespace Git
