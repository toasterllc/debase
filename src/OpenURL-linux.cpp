#include "OpenURL.h"
#include <spawn.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/fcntl.h>
#include <unistd.h>
#include "lib/toastbox/RuntimeError.h"
#include "lib/toastbox/Defer.h"

extern "C" {
    extern char** environ;
};

static void _Spawn(std::string_view prog, std::string_view arg) {
    const char*const argv[] = { prog.data(), arg.data(), nullptr };
    
    // Route stdin/stdout/stderr to /dev/null
    // We do this because when xdg-open fails, it writes errors to stdout/stderr,
    // and we don't want its error messages appearing in our console -- we'd
    // rather just have the link fail silently.
    posix_spawn_file_actions_t fileActions;
    int ir = posix_spawn_file_actions_init(&fileActions);
    if (ir) throw Toastbox::RuntimeError("posix_spawn_file_actions_init failed: %s", strerror(ir));
    Defer( posix_spawn_file_actions_destroy(&fileActions); )
    
    ir = posix_spawn_file_actions_addopen(&fileActions, STDIN_FILENO, "/dev/null", O_RDONLY, 0);
    if (ir) throw Toastbox::RuntimeError("posix_spawn_file_actions_addopen failed: %s", strerror(ir));
    
    ir = posix_spawn_file_actions_addopen(&fileActions, STDOUT_FILENO, "/dev/null", O_WRONLY, 0);
    if (ir) throw Toastbox::RuntimeError("posix_spawn_file_actions_addopen failed: %s", strerror(ir));
    
    ir = posix_spawn_file_actions_addopen(&fileActions, STDERR_FILENO, "/dev/null", O_WRONLY, 0);
    if (ir) throw Toastbox::RuntimeError("posix_spawn_file_actions_addopen failed: %s", strerror(ir));
    
    // Spawn the text editor and wait for it to exit
    pid_t pid = -1;
    ir = posix_spawnp(&pid, argv[0], &fileActions, nullptr, (char *const*)argv, environ);
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
