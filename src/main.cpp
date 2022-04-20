#include <iostream>
#include <spawn.h>
#include "state/StateDir.h"
#include "lib/toastbox/Stringify.h"
#include "lib/toastbox/StringSplit.h"
#include "machine/Machine.h"
#include "App.h"
#include "Terminal.h"
#include "Debase.h"
#include "UsageText.h"
#include "LibsText.h"

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
    
    // Hidden options
    struct {
        bool en = false;
    } machineId;
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
    
    // Hidden options
    if (arg0 == "--machineid") {
        return _Args{
            .machineId = {
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
    std::cout << "debase version " Stringify(_DebaseVersion) "\n";
    std::cout << UsageText;
}

static void _PrintLibs() {
    std::cout << LibsText;
}

//// _CurrentExecutableIsInEnvironmentPath() returns true if the current executable
//// is located in a directory listed in the PATH environment variable
//static bool _CurrentExecutableIsInEnvironmentPath() {
//    return PathIsInEnvironmentPath(CurrentExecutablePath().parent_path());
//}

int main(int argc, const char* argv[]) {
//    sleep(1);
    
//    char* res = nullptr;
//    size_t resLen = 0;
//    CURLcode cr = Curl_base64_encode("hello", 5, &res, &resLen);
//    assert(cr==CURLE_OK);
//    printf("%s\n", res);
    
//    SHA512Half::Hash hash = SHA512Half::Calc("hello");
//    printf("%s", SHA512Half::StringForHash(hash).c_str());
//    return 0;
    
//    printf("%s\n", License::Calc().c_str());
//    return 0;
    
    #warning TODO: linux: OpenURL(): use /dev/null for stdin/stdout, so that errors don't get printed
    
    #warning TODO: linux: sometimes we get the mouse-moved escape codes upon exit
    
    #warning TODO: linux: step through machine-id generation in debugger and make sure it looks sane
    
    #warning TODO: review MachineId-generation scheme. are the chosen directories stable enough?
    
    #warning TODO: obfuscate paths in Machine.cpp
    
    #warning TODO: linux: do lots of testing
    #warning TODO: macos-x86: do lots of testing
    #warning TODO: macos-arm64: do lots of testing
    
//  Future:
    
    #warning TODO: linux: starting/exiting is slow (apparently due to detaching/attaching head). detach head lazily when the branch relevant branch is modified?
    
    #warning TODO: move commits away from dragged commits to show where the commits will land
    
    #warning TODO: add column scrolling
    
    #warning TODO: if we cant speed up git operations, show a progress indicator
    
    #warning TODO: figure out why moving/copying commits is slow sometimes
    
    #warning TODO: ? add feature requests field in register panel
    
//  DONE:
//    #warning TODO: Add --libs subcommand to print legalese
//
//    #warning TODO: linux: try binary on fresh linux installs
//
//    #warning TODO: State: switch to flock() since Linux doesn't support O_EXLOCK flag to open()
//
//    #warning TODO: Alert: make button highlight color match window highlight color
//
//    #warning TODO: rename XXXPanel -> XXXAlert, if it's derived from Alert
//
//    #warning TODO: stress test DebaseLicenseServer with lots of connections
//
//    #warning TODO: cleanup synchronous modal panel code (ie XXXRun() functions)
//
//    #warning TODO: notify user about debase updates
//
//    #warning TODO: allow debase to be called from git repo subdirectories
//
//    #warning TODO: add installation affordances: suggest moving debase to a permanent location
//
//    #warning TODO: linux: implement URL opening
//
//    #warning TODO: add hidden --machineId flag for end user debugging
//
//    #warning TODO: mac: use IOPlatformSerial instead of current MachineId scheme
//
//    #warning TODO: server: trials: keep track of how many trials are activated for a given machine id
//
//    #warning TODO: server: license: record date + OS type that a MachineID was added to a license, so we can prune them periodically
//
//    #warning TODO: reconsider doing network IO on first launch without the user first clicking "start free 7-day trial"
//        // https://news.ycombinator.com/item?id=30921231
//
//    #warning TODO: implement 7-day trial
//    
//    #warning TODO: implement registration
//
//    #warning TODO: make trial countdown overlay
//
//    #warning TODO: have a trial button on the renew-license dialog?
//
//    #warning TODO: round trial countdown up
//
//    #warning TODO: re-evaluate our uses of optional in Color.h
//    
//    #warning TODO: hide cursor on start -- it flickers
//
//    #warning TODO: hide cursor when modal panel opens with textfield in the background window
//
//    #warning TODO: show animation while performing network IO
//
//    #warning TODO: add OK / Cancel buttons to RegisterAlert
//    
//    #warning TODO: fix: events should be handled from back of subviews() to front, so that Alert's can eat all the events
//
//    #warning TODO: fix: modal error panel isn't resizing/redrawing with window resize
//
//    #warning TODO: can we get rid of the Window requirement from draw/layout/handleEvent?
//    
//    #warning TODO: make View::draw() relative to the view's bounds, instead of the window's bounds
//
//    #warning TODO: everything is redrawing when dragging commits
//
//    #warning TODO: context menu hit test expand doesn't seem to be working
//
//    #warning TODO: encapsulate View's hitTestExpand
//
//    #warning TODO: remove colors requirement from View/Alert/etc constructor
//
//    #warning TODO: switch to a subviewCreate() model so our subclasses don't have to maintain the list of subviews
//
//    #warning TODO: make Window inherit from View
//
//    #warning TODO: move track() into View
//
//    #warning TODO: RevColumn: make headers a label
//
//    #warning TODO: fix: rapid click of the same commit between 2 different columns triggers double click
//
//    #warning TODO: have Control/View return its subviews to eliminate boilerplate
//
//    #warning TODO: Control -> View
//
//    #warning TODO: fix: context menu not drawing separators
//
//    #warning TODO: fix: clicking on disabled button within context menu closes the menu
//
//    #warning TODO: test new @{0} @{-1} @{-2} behavior with a git repo that has a reflog without that many entries
//
//    #warning TODO: if no arguments are specified, have debase launch with: @{0}, @{-1}, @{-2}
//
//    #warning TODO: figure out how to remove the layoutNeeded=true / drawNeeded=true from MainWindow layout()/draw()
//    
//    #warning TODO: try to optimize drawing. maybe draw using a random color so we can tell when things refresh?
//
//    #warning TODO: move from master~3 -> master doesn't select commits in master
//
//    #warning TODO: verify that ctrl-c still works with RegPanel visible
//
//    #warning TODO: right click on panel doesn't draw purple
//
//    #warning TODO: screen flickers when showing error panel (throw exception when deleting commit)
//
//    #warning TODO: get snapshots menu working again
//
//    #warning TODO: have message panel implement track() like menu does
//
//    #warning TODO: MainWindow -> App
//
//    #warning TODO: get reg panel working again
//
//    #warning TODO: figure out a way to not redraw Menu every cycle, but support button highlight on hover
//
//    #warning TODO: switch Menu to follow TextField handleEvent() model
//
//    #warning TODO: get expanded hit-testing of menu buttons working again
//
//    #warning TODO: switch Button to follow TextField handleEvent() model
//
//    #warning TODO: switch classes to not be shared_ptr where possible. particularly Window.
//
//    #warning TODO: TextField: don't lose content when resizing window
//
//    #warning TODO: TextField: filter printable characters better
//
//    #warning TODO: TextField: make clicking to reposition cursor work
//
//    #warning TODO: TextField: make text truncation work, including arrow keys to change left position//
//
//    #warning TODO: TextField: make enter key work
//
//    #warning TODO: TextField: make tabbing between fields work
//
//    #warning TODO: add help flag
//
//    #warning TODO: fix: if the mouse is moving upon exit, we get mouse characters printed to the terminal
//
//    #warning TODO: handle not being able to materialize commits via Convert()
//
//    #warning TODO: make dark-mode error color more reddish
//
//    #warning TODO: support light mode
//
//    #warning TODO: improve error when working directory isn't a git repo
//
//    #warning TODO: when switching snapshots, clear the selection
//
//    #warning TODO: move branch name to top
//
//    #warning TODO: fix: snapshots line wrap is 1-character off
//
//    #warning TODO: nevermind: show warning on startup: Take care when rewriting history. As with any software, debase may be bugs. As a safety precaution, debase will automatically backup all branches before modifying them, as <BranchName>-DebaseBackup
//
//    #warning TODO: rename all hittest functions -> updateMouse
//    
//    #warning TODO: fix: handle case when snapshot menu won't fit entirely on screen
//
//    #warning TODO: nevermind: don't show the first snapshot if it's the same as the "Session Start" snapshot
//
//    #warning TODO: make sure works with submodules
//
//    #warning TODO: keep the oldest snapshots, instead of the newest
//
//    #warning TODO: nevermind: figure out how to hide a snapshot if it's the same as the "Session Start" snapshot
//
//    #warning TODO: improve appearance of active snapshot marker in snapshot menu
//
//    #warning TODO: nevermind: only show the current-session marker on the "Session Start" element if there's no other snapshot that has it
//
//    #warning TODO: fully flesh out when to create new snapshots
//
//    #warning TODO: show "Session Start" snapshot
//
//    #warning TODO: fix: crash when deleting prefs dir while debase is running
//
//    #warning TODO: get command-. working while context menu/snapshot menu is open
//
//    #warning TODO: when creating a new snapshot, if an equivalent one already exists, remove the old one.
//
//    #warning TODO: further, snapshots should be created only at startup ("Session Start"), and on exit, the snapshot to only be enqueued if it differs from the current HEAD of the repo
//    #warning TODO: when restoring a snapshot, it should just restore the HEAD of the snapshot, and push an undo operation (as any other operation) that the user can undo/redo as normal
//    #warning TODO: turn snapshots into merely keeping track of what commit was the HEAD, not the full undo history.
//
//    #warning TODO: backup all supplied revs before doing anything
//    
//    #warning TODO: nevermind: implement log of events, so that if something goes wrong, we can manually get back
//
//    #warning TODO: test a commit stored in the undo history not existing
//
//    #warning TODO: use advisory locking in State class to prevent multiple entities from modifying state on disk simultaneously. RepoState class will need to use this facility when writing its state. Whenever we acquire the lock, we should read the version, because the version may have changed since the last read! if so -> bail!
//
//    #warning TODO: write a version number at the root of the StateDir, and check it somewhere
//    
//    #warning TODO: don't lose the UndoHistory when: a ref no longer exists that we previously observed
//    #warning TODO: don't lose the UndoHistory when: debase is run and a ref points to a different commit than we last observed
//
//    #warning TODO: show detailed error message if we fail to restore head due to conflicts
//
//    #warning TODO: refuse to run if there are uncomitted changes and we're detaching head
//
//    #warning TODO: perf: if we aren't modifying the current checked-out branch, don't detach head
//
//    #warning TODO: implement _ConfigDir() for real
//
//    #warning TODO: undo: remember selection as a part of the undo state
//    
//    #warning TODO: fix: (1) copy a commit to col A, (2) swap elements 1 & 2 of col A. note how the copied commit doesn't get selected when performing undo/redo
//    
//    #warning TODO: fix: copying a commit from a column shouldn't affect column's undo state (but it does)
//
//    #warning TODO: support undo/redo
//
//    #warning TODO: when deserializing, if a ref doesn't exist, don't throw an exception, just prune the entry
//    
//    #warning TODO: undo: fix UndoHistory deserialization
//
//    #warning TODO: make "(HEAD)" suffix persist across branch modifications
//    
//    #warning TODO: make sure debase still works when running on a detached HEAD
//
//    #warning TODO: fix wrong column being selected after moving commit in master^
//
//    #warning TODO: improve error messages when we can't lookup supplied refs
//
//    #warning TODO: when supplying refs on the command line in the form ref^ or ref~N, can we use a ref-backed rev (instead of using a commit-backed rev), and just offset the RevColumn, so that the rev is mutable?
//
//    #warning TODO: improve error messages: merge conflicts, deleting last branch commit
//
//    #warning TODO:   dragging commits to their current location -> no repo changes
//    #warning TODO:   same commit message -> no repo changes
//    #warning TODO: no-op guarantee:
//
//    #warning TODO: make sure we can round-trip with the same date/time. especially test commits with negative UTC offsets!
//    
//    #warning TODO: bring back _CommitTime so we don't need to worry about the 'sign' field of git_time
//
//    #warning TODO: handle merge conflicts
//
//    #warning TODO: set_escdelay: not sure if we're going to encounter issues?
//
//    #warning TODO: don't allow combine when a merge commit is selected
//
//    #warning TODO: always show the same contextual menu, but gray-out the disabled options
//    
//    #warning TODO: make sure moving/deleting commits near the root commit still works
//
//    #warning TODO: properly handle moving/copying merge commits
//
//    #warning TODO: show some kind of indication that a commit is a merge commit
//
//    #warning TODO: handle window resizing
//
//    #warning TODO: re-evaluate size of drag-cancel affordance since it's not as reliable in iTerm
//
//    #warning TODO: don't allow double-click/return key on commits in read-only columns
//
//    #warning TODO: handle rewriting tags
//    
//    #warning TODO: create special color palette for apple terminal
//
//    #warning TODO: if can_change_color() returns false, use default color palette (COLOR_RED, etc)
//
//    #warning TODO: fix: colors aren't restored when exiting
//
//    #warning TODO: figure out whether/where/when to call git_libgit2_shutdown()
//
//    #warning TODO: implement error messages
//
//    #warning TODO: double-click broken: click commit, wait, then double-click
//
//    #warning TODO: implement double-click to edit
//
//    #warning TODO: implement key combos for combine/edit
//
//    #warning TODO: show indication in the UI that a column is immutable
//
//    #warning TODO: special-case opening `mate` when editing commit, to not call CursesDeinit/CursesInit
//
//    #warning TODO: implement EditCommit
//
//    #warning TODO: allow editing author/date of commit
//    
//    #warning TODO: configure tty before/after calling out to a new editor
//    
//    #warning TODO: fix commit message rendering when there are newlines
//
//    #warning TODO: improve panel dragging along edges. currently the shadow panels get a little messed up
//
//    #warning TODO: add affordance to cancel current drag by moving mouse to edge of terminal
//
//    #warning TODO: implement CombineCommits
//
//    #warning TODO: draw "Move/Copy" text immediately above the dragged commits, instead of at the insertion point
//    
//    #warning TODO: create concept of revs being mutable. if they're not mutable, don't allow moves from them (only copies), moves to them, deletes, etc
//    
//    #warning TODO: don't allow remote branches to be modified (eg 'origin/master')
//    
//    #warning TODO: show similar commits to the selected commit using a lighter color
//    
//    #warning       to do so, we'll need to implement operator< on Rev so we can put them in a set
//    #warning TODO: we need to unique-ify the supplied revs, since we assume that each column has a unique rev
//    
//    #warning TODO: allow escape key to abort a drag
//    
//    #warning TODO: when copying commmits, don't hide the source commits
    
//    {
//        volatile bool a = false;
//        while (!a);
//    }
    
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
        
        } else if (args.machineId.en) {
            const Machine::MachineId machineId = Machine::MachineIdCalc(DebaseProductId);
            printf("%s\n", machineId.c_str());
            return 0;
        
        } else {
            throw Toastbox::RuntimeError("invalid arguments");
        }
        
        // Disable echo before activating ncurses
        // This is necessary to prevent an edge case where the mouse-moved escape
        // sequences can get printed to the console when debase exits.
        // So far we haven't been able to reproduce the issue after adding this
        // code. But if we do see it again in the future, try giving tcsetattr()
        // the TCSAFLUSH or TCSADRAIN flag in Terminal::Settings (instead of
        // TCSANOW) to see if that solves it.
        Terminal::Settings term(STDIN_FILENO);
        term.c_lflag &= ~(ICANON|ECHO);
        term.set();
        
        setlocale(LC_ALL, "");
        
        Git::Repo repo;
        std::vector<Git::Rev> revs;
        try {
            repo = Git::Repo::Open(".");
        } catch (...) {
            throw Toastbox::RuntimeError("current directory isn't a git repository");
        }
        
        if (args.run.revs.empty()) {
            constexpr size_t RevCountDefault = 5;
            std::set<Git::Rev> unique;
            ssize_t i = 0;
            while (revs.size() < RevCountDefault) {
                Git::Rev rev;
                try {
                    rev = (!i ? repo.headResolved() : repo.revLookup("@{" + std::to_string(i) + "}"));
                    
                    if (unique.find(rev) == unique.end()) {
                        revs.emplace_back(rev);
                        unique.insert(rev);
                    }
                
                } catch (...) {
                    break;
                }
                i--;
            }
        
        } else {
            // Unique the supplied revs, because our code assumes a 1:1 mapping between Revs and RevColumns
            std::set<Git::Rev> unique;
            for (const std::string& revName : args.run.revs) {
                Git::Rev rev;
                try {
                    rev = repo.revLookup(revName);
                } catch (...) {
                    throw Toastbox::RuntimeError("invalid rev: %s", revName.c_str());
                }
                
                if (unique.find(rev) == unique.end()) {
                    revs.push_back(rev);
                    unique.insert(rev);
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
