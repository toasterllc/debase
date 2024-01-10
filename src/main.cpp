#include <iostream>
#include <spawn.h>
#include "state/StateDir.h"
#include "lib/toastbox/Stringify.h"
#include "lib/toastbox/String.h"
#include "App.h"
#include "Terminal.h"
#include "Debase.h"
#include "DebaseGitHash.h"
#include "UsageText.h"
#include "LibsText.h"
#include "Syscall.h"
#include "Rev.h"

//extern "C" {
//#include "lib/libcurl/include/curl/curl.h"
//#include "lib/libcurl/lib/curl_base64.h"
//}

struct _Args {
    struct {
        bool en = false;
        std::vector<std::string> revs;
    } run;
    
    struct {
        bool en = false;
    } help;
    
    struct {
        bool en = false;
        std::string theme;
    } setTheme;
    
    struct {
        bool en = false;
    } libs;
};

static _Args _ParseArgs(int argc, const char* argv[]) {
    using namespace Toastbox;
    
    std::vector<std::string> strs;
    for (int i=0; i<argc; i++) strs.push_back(argv[i]);
    
    _Args args;
    if (strs.size() < 1) {
        return _Args{ .run = {.en = true}, };
    }
    
    std::string arg0 = strs[0];
    std::transform(arg0.begin(), arg0.end(), arg0.begin(),
        [](unsigned char c){ return std::tolower(c); });
    
    if (arg0=="-h" || arg0=="--help") {
        return _Args{ .help = {.en = true}, };
    }
    
    if (arg0 == "--theme") {
        if (strs.size() < 2) throw std::runtime_error("no theme specified");
        if (strs.size() > 2) throw std::runtime_error("too many arguments supplied");
        return _Args{
            .setTheme = {
                .en = true,
                .theme = strs[1],
            },
        };
    }
    
    if (arg0 == "--libs") {
        return _Args{
            .libs = {
                .en = true,
            },
        };
    }
    
    return _Args{
        .run = {
            .en = true,
            .revs = strs,
        },
    };
}

static void _PrintUsage() {
    std::cout << "debase version " Stringify(_DebaseVersion) " (" _DebaseGitHashShort ")" "\n";
    std::cout << UsageText;
}

static void _PrintLibs() {
    std::cout << LibsText;
}

//static void _StdinFlush() {
//    for (int i=0; i<100000; i++) {
//        int avail = 0;
//        Ioctl(STDIN_FILENO, FIONREAD, &avail);
////        if (avail <= 0) break;
//        
//        uint8_t buf[128];
//        Toastbox::Read(STDIN_FILENO, buf, avail);
//        if (avail) printf("Flushed %d bytes\n", avail);
//    }
//}

static void _StdinFlush(std::chrono::steady_clock::duration timeout) {
    auto deadline = std::chrono::steady_clock::now()+timeout;
    uint8_t buf[128];
    Toastbox::Read(STDIN_FILENO, buf, sizeof(buf), deadline);
}

//// _CurrentExecutableIsInEnvironmentPath() returns true if the current executable
//// is located in a directory listed in the PATH environment variable
//static bool _CurrentExecutableIsInEnvironmentPath() {
//    return PathIsInEnvironmentPath(CurrentExecutablePath().parent_path());
//}

Rev _RevLookup(const Git::Repo& repo, const std::string& str) {
    Rev rev;
    (Git::Rev&)rev = repo.revLookup(str);
    if (rev.ref) return rev;
    
    // Try to get a ref-backed rev (with skip parameter) by removing the ^ or ~ suffix from `str`
    Rev skipRev;
    {
        std::string name(str);
        size_t pos = name.find("^");
        if (pos != std::string::npos) {
            name.erase(pos);
            (Git::Rev&)skipRev = repo.revLookup(name);
        }
    }
    
    if (!skipRev.ref) {
        std::string name(str);
        size_t pos = name.find("~");
        if (pos != std::string::npos) {
            name.erase(pos);
            (Git::Rev&)skipRev = repo.revLookup(name);
        }
    }
    
    // If we found a `skipRev.ref` by removing the ^~ suffix, calculate the `skip` value
    if (skipRev.ref) {
        size_t skip = 0;
        Git::Commit head = skipRev.commit;
        while (head && head!=rev.commit) {
            head = head.parent();
            skip++;
        }
        
        if (head) {
            skipRev.skip = skip;
            return skipRev;
        }
    }
    
    // We couldn't parse `str` directly as a ref, nor could we parse it as "ref+skip" (ie rev^ / rev~X).
    // So just return the commit itself.
    return rev;
}

int main(int argc, const char* argv[]) {
    #warning TODO: linux:       run through TestChecklist.txt
    #warning TODO: macos-x86:   run through TestChecklist.txt
    #warning TODO: macos-arm64: run through TestChecklist.txt
    
//  Future:
    
    #warning TODO: handle submodule conflicts, where the source/destination branches have a submodule with different commits checked out.
    #warning TODO:   to handle this, we'll need to look at git_index_entry's mode field. when the high 4 bits == 1110, it's a 'gitlink', which (presumably) means a submodule.
    #warning TODO:   add a 'type' field or similar to Git::Conflict to allow us to signify where it's a regular file conflict, or a submodule conflict.
    #warning TODO:   -------
    #warning TODO:   to reproduce, use MDC repo. drag commit 60dfc7c (source branch) onto 7e18563 (destination branch).
    
    #warning TODO: ConflictPanel: improve appearance of conflict text on linux
    #warning TODO:   any way to determine background color?
    
    #warning TODO: improve UI of snapshots menu by separating the entries into 2 sections: 'Session Start' and 'Saved'
    
    #warning TODO: add tag indicator for tag columns
    
    #warning TODO: add branch duplication; action menu? drag branch name? both?
    #warning TODO:   add branch to reflog so that it appears in subsequent launches
    
    #warning TODO: ConflictPanel: handle indentation -- if the conflicted block is indented a lot, unindent the text
    #warning TODO:   need to handle tabs properly -- do the de-indenting after filtering the text (which replaces
    #warning TODO:   tabs with spaces)
    
    #warning TODO: add scrolling
    
    #warning TODO: integrate debase-releases as a submodule into debase.
    #warning TODO: invoke with `make release`; its tasks are:
    #warning TODO:   - in the debase repo: create a tag `v<VersionNumber>`
    #warning TODO:   - in the submodule: copy the binaries, create the zip, commit
    #warning TODO:   - in the debase repo: create a commit with the updated debase-releases submodule
    
    #warning TODO: make update mechanism automatically replace debase binary, instead of forcing user to go to website
    
    #warning TODO: when changing the author of a commit, change the committer too
    #warning TODO: look at the content of github when doing so -- if the author/committer are different, they're both displayed, which isn't what we want
    #warning TODO: maybe both author and committer should be shown in editor? but maybe only if theyre different?
    
    #warning TODO: move commits away from dragged commits to show where the commits will land
    
    #warning TODO: add column scrolling
    
    #warning TODO: if we cant speed up git operations, show a progress indicator
    
    #warning TODO: figure out why moving/copying commits is slow sometimes
    
    try {
        _Args args = _ParseArgs(argc-1, argv+1);
        
        if (args.run.en) {
            // Nothing to do
        
        } else if (args.help.en) {
            _PrintUsage();
            return 0;
        
        } else if (args.setTheme.en) {
            State::Theme theme = State::Theme::None;
            if (args.setTheme.theme == "auto") {
                theme = State::Theme::None;
            } else if (args.setTheme.theme == "dark") {
                theme = State::Theme::Dark;
            } else if (args.setTheme.theme == "light") {
                theme = State::Theme::Light;
            } else {
                throw Toastbox::RuntimeError("invalid theme: %s", args.setTheme.theme.c_str());
            }
            State::ThemeWrite(theme);
            return 0;
        
        } else if (args.libs.en) {
            _PrintLibs();
            return 0;
        
        } else {
            throw Toastbox::RuntimeError("invalid arguments");
        }
        
        // Prevent mouse-moved escape sequences from being printed to the console
        // when debase exits, using a 2-step strategy:
        // 
        //   1. disable echo before activating ncurses;
        //   2. perform a best-effort flush of stdin right before we re-enable
        //      echo and exit.
        // 
        // Both steps are needed to ensure that mouse-moved events are queued only
        // while echo is disabled, otherwise they'll end up being printed.
        // 
        // We previously tried omitting #2, and instead re-enabling echo using
        // the TCSAFLUSH/TCSADRAIN flags to tcsetattr(), but that still allowed
        // escape sequences to slip through, so presumably those flags don't
        // flush/drain queued mouse escape sequences for some reason.
        constexpr std::chrono::milliseconds FlushTimeout(50);
        Terminal::Settings term(STDIN_FILENO);
        term.c_lflag &= ~(ICANON|ECHO);
        term.set();
        Defer(_StdinFlush(FlushTimeout));
        
        setlocale(LC_ALL, "");
        
        Git::Repo repo;
        std::vector<Rev> revs;
        try {
            repo = Git::Repo::Open(".");
        } catch (...) {}
        
//        repo.headDetach();
//        repo.headAttach(repo.revLookup("master"));
//        repo.headAttach(repo.revLookup("master"));
//        repo.headAttach(repo.revLookup("v1"));
//        repo.headAttach(repo.revLookup("v1"));
//        repo.headDetach();
//        exit(0);
        
        if (repo) {
            if (args.run.revs.empty()) {
                constexpr size_t RevCountDefault = 5;
                std::set<Rev> unique;
                
                // Add HEAD to `revs`
                {
                    Rev rev;
                    (Git::Rev&)rev = repo.headResolved();
                    revs.emplace_back(rev);
                    unique.insert(rev);
                }
                
                // Fill out `revs` with recent revs that were checked out, until we hit `RevCountDefault`
                Git::Reflog reflog = repo.reflogForRef(repo.head());
                for (size_t i=0; revs.size()<RevCountDefault; i++) {
                    const git_reflog_entry*const entry = reflog[i];
                    if (!entry) break; // End of reflog
                    
                    try {
                        Rev rev;
                        (Git::Rev&)rev = repo.reflogRevForCheckoutEntry(entry);
                        // Ignore non-ref reflog entries
                        if (!rev.ref) continue;
                        const auto [_, inserted] = unique.insert(rev);
                        if (inserted) revs.emplace_back(rev);
                        
                    // Ignore errors -- refs mentioned in the reflog may have been deleted,
                    // which will throw when we try to lookup
                    } catch (const std::exception& e) {}
                }
            
            } else {
                // Unique the supplied revs, because our code assumes a 1:1 mapping between Revs and RevColumns
                std::set<Rev> unique;
                for (const std::string& revName : args.run.revs) {
                    Rev rev;
                    try {
                        rev = _RevLookup(repo, revName);
                    } catch (...) {
                        throw Toastbox::RuntimeError("invalid rev: %s", revName.c_str());
                    }
                    
                    if (unique.find(rev) == unique.end()) {
                        revs.push_back(rev);
                        unique.insert(rev);
                    }
                }
            }
        }
        
        auto app = std::make_shared<App>(repo, revs);
        app->run();
    
    } catch (const std::exception& e) {
        fprintf(stderr, "Error: %s\n", e.what());
        return 1;
    }
    
    return 0;
}
