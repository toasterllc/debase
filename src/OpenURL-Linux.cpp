#include "OpenURL.h"
#include <spawn.h>
#include <string.h>
#include <sys/wait.h>
#include "lib/toastbox/RuntimeError.h"

extern "C" {
    extern char** environ;
};

static void _Spawn(std::string_view prog, std::string_view arg) {
    const char*const argv[] = { prog.data(), arg.data(), nullptr };
    
    // Spawn the text editor and wait for it to exit
    pid_t pid = -1;
    int ir = posix_spawnp(&pid, argv[0], nullptr, nullptr, (char *const*)argv, environ);
    if (ir) throw Toastbox::RuntimeError("posix_spawnp failed: %s", strerror(ir));

    int status = 0;
    ir = 0;
    do ir = waitpid(pid, &status, 0);
    while (ir==-1 && errno==EINTR);
    if (ir == -1) throw Toastbox::RuntimeError("waitpid failed: %s", strerror(errno));
    if (ir != pid) throw Toastbox::RuntimeError("unknown waitpid result: %d", ir);
}

void OpenURL(std::string_view url) noexcept {
    try {
        _Spawn("xdg-open", url);
    } catch (...) {}
}
