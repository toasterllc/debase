#include <iostream>
#include <spawn.h>
#include "MainWindow.h"
#include "StateDir.h"
#include "xterm-256color.h"
#include "Terminal.h"
#include "Debase.h"

namespace fs = std::filesystem;

static State::Theme _Theme = State::Theme::None;

static UI::ColorPalette _Colors;
static UI::ColorPalette _ColorsPrev;
static UI::CursorState _CursorState;

static UI::Color _ColorSet(const UI::Color& c) {
    UI::Color prev(c.idx);
    color_content(prev.idx, &prev.r, &prev.g, &prev.b);
    ::init_color(c.idx, c.r, c.g, c.b);
    return prev;
}

static UI::ColorPalette _ColorsSet(const UI::ColorPalette& p) {
    UI::ColorPalette pcopy = p;
    
    // Set the values for the custom colors, and remember the old values
    for (UI::Color& c : pcopy.custom()) {
        c = _ColorSet(c);
    }
    
    for (const UI::Color& c : p.colors()) {
        ::init_pair(c.idx, c.idx, -1);
    }
    
    return pcopy;
}

static UI::ColorPalette _ColorsCreate(State::Theme theme) {
    const std::string termProg = getenv("TERM_PROGRAM");
    const bool themeDark = (theme==State::Theme::None || theme == State::Theme::Dark);
    
    UI::ColorPalette colors;
    UI::Color black;
    UI::Color gray;
    UI::Color purple;
    UI::Color purpleLight;
    UI::Color greenMint;
    UI::Color red;
    
    if (termProg == "Apple_Terminal") {
        // Colorspace: unknown
        // There's no simple relation between these numbers and the resulting colors because Apple's
        // Terminal.app applies some kind of filtering on top of these numbers. These values were
        // manually chosen based on their appearance.
        if (themeDark) {
            colors.normal           = COLOR_BLACK;
            colors.dimmed           = colors.add( 77,  77,  77);
            colors.selection        = colors.add(  0,   2, 255);
            colors.selectionSimilar = colors.add(140, 140, 255);
            colors.selectionCopy    = colors.add(  0, 229, 130);
            colors.menu             = colors.selectionCopy;
            colors.error            = colors.add(194,   0,  71);
        
        } else {
            colors.normal           = COLOR_BLACK;
            colors.dimmed           = colors.add(128, 128, 128);
            colors.selection        = colors.add(  0,   2, 255);
            colors.selectionSimilar = colors.add(140, 140, 255);
            colors.selectionCopy    = colors.add( 52, 167,   0);
            colors.menu             = colors.add(194,   0,  71);
            colors.error            = colors.menu;
        }
    
    } else {
        // Colorspace: sRGB
        // These colors were derived by sampling the Apple_Terminal values when they're displayed on-screen
        
        if (themeDark) {
            colors.normal           = COLOR_BLACK;
            colors.dimmed           = colors.add(.486*255, .486*255, .486*255);
            colors.selection        = colors.add(.463*255, .275*255, 1.00*255);
            colors.selectionSimilar = colors.add(.663*255, .663*255, 1.00*255);
            colors.selectionCopy    = colors.add(.204*255, .965*255, .569*255);
            colors.menu             = colors.selectionCopy;
            colors.error            = colors.add(.969*255, .298*255, .435*255);
        
        } else {
            colors.normal           = COLOR_BLACK;
            colors.dimmed           = colors.add(.592*255, .592*255, .592*255);
            colors.selection        = colors.add(.369*255, .208*255, 1.00*255);
            colors.selectionSimilar = colors.add(.627*255, .627*255, 1.00*255);
            colors.selectionCopy    = colors.add(.306*255, .737*255, .153*255);
            colors.menu             = colors.add(.969*255, .298*255, .435*255);
            colors.error            = colors.menu;
        }
    }
    
    return colors;
}

static void _CursesInit() noexcept {
    // Default Linux installs may not contain the /usr/share/terminfo database,
    // so provide a fallback terminfo that usually works.
    nc_set_default_terminfo(xterm_256color, sizeof(xterm_256color));
    
    // Override the terminfo 'kmous' and 'XM' properties to permit mouse-moved events,
    // in addition to the default mouse-down/up events.
    //   kmous = the prefix used to detect/parse mouse events
    //   XM    = the escape string used to enable mouse events (1006=SGR 1006 mouse
    //           event mode; 1003=report mouse-moved events in addition to clicks)
    setenv("TERM_KMOUS", "\x1b[<", true);
    setenv("TERM_XM", "\x1b[?1006;1003%?%p1%{1}%=%th%el%;", true);
    
    ::initscr();
    ::noecho();
    ::raw();
    
    ::use_default_colors();
    ::start_color();
    
    if (can_change_color()) {
        _Colors = _ColorsCreate(_Theme);
    }
    
    _ColorsPrev = _ColorsSet(_Colors);
    
    _CursorState = UI::CursorState(false, {});
    
//    // Hide cursor
//    ::curs_set(0);
    
    ::mousemask(ALL_MOUSE_EVENTS | REPORT_MOUSE_POSITION, NULL);
    ::mouseinterval(0);
    
    ::set_escdelay(0);
}

static void _CursesDeinit() noexcept {
//    ::mousemask(0, NULL);
    
    _CursorState.restore();
    
    _ColorsSet(_ColorsPrev);
    ::endwin();
    
//    sleep(1);
}

static void _Spawn(const char*const* argv) {
    // preserveTerminalCmds: these commands don't modify the terminal, and therefore
    // we don't want to deinit/reinit curses when calling them.
    // When invoking commands such as vi/pico, we need to deinit/reinit curses
    // when calling out to them, because those commands reconfigure the terminal.
    static const std::set<std::string> preserveTerminalCmds = {
        "mate"
    };
    
    const bool preserveTerminal = preserveTerminalCmds.find(argv[0]) != preserveTerminalCmds.end();
    if (!preserveTerminal) _CursesDeinit();
    
    // Spawn the text editor and wait for it to exit
    {
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
    
    if (!preserveTerminal) _CursesInit();
}

static State::Theme _ThemeRead() {
    State::State state(StateDir());
    State::Theme theme = state.theme();
    if (theme != State::Theme::None) return theme;
    
    bool write = false;
    // If a theme isn't set, ask the terminal for its background color,
    // and we'll choose the theme based on that
    Terminal::Background bg = Terminal::Background::Dark;
    try {
        bg = Terminal::BackgroundGet();
    } catch (...) {
        // We failed to get the terminal background color, so write the theme
        // for the default background color to disk, so we don't try to get
        // the background color again in the future. (This avoids a timeout
        // delay in Terminal::BackgroundGet() that occurs if the terminal
        // doesn't support querying the background color.)
        write = true;
    }
    
    switch (bg) {
    case Terminal::Background::Dark:    theme = State::Theme::Dark; break;
    case Terminal::Background::Light:   theme = State::Theme::Light; break;
    }
    
    if (write) {
        state.theme(theme);
        state.write();
    }
    return theme;
}

static void _ThemeWrite(State::Theme theme) {
    State::State state(StateDir());
    state.theme(theme);
    state.write();
}

struct _Args {
    struct {
        bool en = false;
    } help;
    
    struct {
        bool en = false;
        std::string theme;
    } setTheme;
    
    struct {
        bool en = false;
        std::vector<std::string> revs;
    } normal;
};

static _Args _ParseArgs(int argc, const char* argv[]) {
    using namespace Toastbox;
    
    std::vector<std::string> strs;
    for (int i=0; i<argc; i++) strs.push_back(argv[i]);
    
    _Args args;
    if (strs.size() < 1) {
        return _Args{ .normal = {.en = true}, };
    }
    
    if (strs[0]=="-h" || strs[0]=="--help") {
        return _Args{ .help = {.en = true}, };
    }
    
    if (strs[0] == "--theme") {
        if (strs.size() < 2) throw std::runtime_error("no theme specified");
        if (strs.size() > 2) throw std::runtime_error("too many arguments supplied");
        return _Args{
            .setTheme = {
                .en = true,
                .theme = strs[1],
            },
        };
    }
    
    return _Args{
        .normal = {
            .en = true,
            .revs = strs,
        },
    };
}

static void _PrintUsage() {
    using namespace std;
    cout << "debase version " DebaseVersionString "\n";
    cout << "\n";
    cout << "Usage:\n";
    cout << "  -h, --help\n";
    cout << "      Print this help message\n";
    cout << "\n";
    cout << "  --theme <auto|dark|light>\n";
    cout << "      Set theme\n";
    cout << "\n";
    cout << "  [<rev>...]\n";
    cout << "      Open the specified git revisions in debase\n";
    cout << "\n";
}

int main(int argc, const char* argv[]) {
    #warning TODO: MainWindow -> MainWindow
    
    #warning TODO: figure out how to remove the layoutNeeded=true / drawNeeded=true from MainWindow layout()/draw()
    
    #warning TODO: have message panel implement track() like menu does
    
    #warning TODO: get snapshots menu working again
    
    #warning TODO: verify that ctrl-c still works with RegPanel visible
    
    #warning TODO: try to optimize drawing. maybe draw using a random color so we can tell when things refresh?
    
    #warning TODO: implement 7-day trial
    
    #warning TODO: implement registration
    
    #warning TODO: do lots of testing
    
//  Future:
    
    #warning TODO: move commits away from dragged commits to show where the commits will land
    
    #warning TODO: add column scrolling
    
    #warning TODO: if we can't speed up git operations, show a progress indicator
    
    #warning TODO: figure out why moving/copying commits is slow sometimes
    
//  DONE:
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
        
        if (args.help.en) {
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
            _ThemeWrite(theme);
            return 0;
        
        } else if (!args.normal.en) {
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
        
        _Theme = _ThemeRead();
        Git::Repo repo;
        std::vector<Git::Rev> _Revs;
        
        try {
            repo = Git::Repo::Open(".");
        } catch (...) {
            throw Toastbox::RuntimeError("current directory isn't a git repository");
        }
        
        Git::Rev _Head = repo.head();
        
        if (args.normal.revs.empty()) {
            _Revs.emplace_back(_Head);
        
        } else {
            // Unique the supplied revs, because our code assumes a 1:1 mapping between Revs and RevColumns
            std::set<Git::Rev> unique;
            for (const std::string& revName : args.normal.revs) {
                Git::Rev rev;
                try {
                    rev = repo.revLookup(revName);
                } catch (...) {
                    throw Toastbox::RuntimeError("invalid rev: %s", revName.c_str());
                }
                
                if (unique.find(rev) == unique.end()) {
                    _Revs.push_back(rev);
                    unique.insert(rev);
                }
            }
        }
        
        // Create _RepoState
        std::set<Git::Ref> refs;
        for (Git::Rev rev : _Revs) {
            if (rev.ref) refs.insert(rev.ref);
        }
        State::RepoState _RepoState(StateDir(), repo, refs);
        
        // Determine if we need to detach head.
        // This is required when a ref (ie a branch or tag) is checked out, and the ref is specified in _Revs.
        // In other words, we need to detach head when whatever is checked-out may be modified.
        bool detachHead = _Head.ref && std::find(_Revs.begin(), _Revs.end(), _Head)!=_Revs.end();
        
        if (detachHead && repo.dirty()) {
            throw Toastbox::RuntimeError("please commit or stash your outstanding changes before running debase on %s", _Head.displayName().c_str());
        }
        
        // Detach HEAD if it's attached to a ref, otherwise we'll get an error if
        // we try to replace that ref.
        if (detachHead) repo.headDetach();
        Defer(
            if (detachHead) {
                // Restore previous head on exit
                std::cout << "Restoring HEAD to " << _Head.ref.name() << std::endl;
                std::string err;
                try {
                    repo.checkout(_Head.ref);
                } catch (const Git::ConflictError& e) {
                    err = "Error: checkout failed because these untracked files would be overwritten:\n";
                    for (const fs::path& path : e.paths) {
                        err += "  " + std::string(path) + "\n";
                    }
                    
                    err += "\n";
                    err += "Please move or delete them and run:\n";
                    err += "  git checkout " + _Head.ref.name() + "\n";
                
                } catch (const std::exception& e) {
                    err = std::string("Error: ") + e.what();
                }
                
                std::cout << (!err.empty() ? err : "Done") << std::endl;
            }
        );
        
        try {
            _CursesInit();
            Defer(_CursesDeinit());
            
            UI::MainWindow<_Spawn> win(_Colors, _RepoState, repo, _Head, _Revs);
            win.run();
            
        } catch (const UI::ExitRequest&) {
            // Nothing to do
        } catch (...) {
            throw;
        }
        
        _RepoState.write();
    
    } catch (const std::exception& e) {
        fprintf(stderr, "Error: %s\n", e.what());
        return 1;
    }
    
    return 0;
}
