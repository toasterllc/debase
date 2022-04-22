#pragma once
#include <set>
#include <fstream>
#include <sys/stat.h>
#include <sys/file.h>
#include <fcntl.h>
#include "lib/toastbox/RuntimeError.h"
#include "lib/toastbox/FileDescriptor.h"
#include "lib/toastbox/FDStream.h"
#include "lib/nlohmann/json.h"
#include "git/Git.h"
#include "git/GitOp.h"
#include "license/License.h"
#include "Version.h"
#include "History.h"

namespace State {

struct Ref : std::string {
    using std::string::string;
    Ref(const std::string& x) : std::string(x) {}
};

struct Commit : std::string {
    using std::string::string;
    Commit(const std::string& x) : std::string(x) {}
};

// State::X -> Git::X

inline Git::Ref Convert(Git::Repo repo, const Ref& x) { return repo.refLookup(x); }
inline Git::Commit Convert(Git::Repo repo, const Commit& x) { return repo.commitLookup(x); }

template <typename T_DstElm, typename T_SrcElm>
inline auto _Convert(Git::Repo repo, const std::vector<T_SrcElm>& x) {
    std::vector<T_DstElm> r;
    for (const T_SrcElm& e : x) r.push_back(Convert(repo, e));
    return r;
}

template <typename T_DstElm, typename T_SrcElm>
inline auto _Convert(Git::Repo repo, const std::set<T_SrcElm>& x) {
    std::set<T_DstElm> r;
    for (const T_SrcElm& e : x) r.insert(Convert(repo, e));
    return r;
}

inline auto Convert(Git::Repo repo, const std::vector<Commit>& x) { return _Convert<Git::Commit>(repo, x); }
inline auto Convert(Git::Repo repo, const std::set<Commit>& x) { return _Convert<Git::Commit>(repo, x); }
inline auto Convert(Git::Repo repo, const std::vector<Ref>& x) { return _Convert<Git::Ref>(repo, x); }
inline auto Convert(Git::Repo repo, const std::set<Ref>& x) { return _Convert<Git::Ref>(repo, x); }

// Git::X -> State::X

inline Ref Convert(const Git::Ref& x) { return Ref(x.fullName()); }
inline Commit Convert(const Git::Commit& x) { return Commit(x.idStr()); }

template <typename T_DstElm, typename T_SrcElm>
inline auto _Convert(const std::vector<T_SrcElm>& x) {
    std::vector<T_DstElm> r;
    for (const T_SrcElm& e : x) r.push_back(Convert(e));
    return r;
}

template <typename T_DstElm, typename T_SrcElm>
inline auto _Convert(const std::set<T_SrcElm>& x) {
    std::set<T_DstElm> r;
    for (const T_SrcElm& e : x) r.insert(Convert(e));
    return r;
}

inline auto Convert(const std::vector<Git::Commit>& x) { return _Convert<Commit>(x); }
inline auto Convert(const std::set<Git::Commit>& x) { return _Convert<Commit>(x); }
inline auto Convert(const std::vector<Git::Ref>& x) { return _Convert<Ref>(x); }
inline auto Convert(const std::set<Git::Ref>& x) { return _Convert<Ref>(x); }

struct RefState {
    Commit head;
    std::set<Commit> selection;
    std::set<Commit> selectionPrev;
    
    bool operator ==(const RefState& x) const {
        if (head != x.head) return false;
        if (selection != x.selection) return false;
        if (selectionPrev != x.selectionPrev) return false;
        return true;
    }
    
    bool operator !=(const RefState& x) const {
        return !(*this==x);
    }
};

using History = T_History<RefState>;

struct Snapshot {
    Snapshot() {}
    Snapshot(Commit c) : creationTime(time(nullptr)), head(c) {}
    Snapshot(Git::Commit c) : Snapshot(Convert(c)) {}
    
    bool operator <(const Snapshot& x) const {
        if (creationTime != x.creationTime) return creationTime < x.creationTime;
        if (head != x.head) return head < x.head;
        return false;
    }
    
    bool operator ==(const Snapshot& x) const {
        if (creationTime != x.creationTime) return false;
        if (head != x.head) return false;
        return true;
    }
    
    bool operator !=(const Snapshot& x) const {
        return !(*this==x);
    }
    
    int64_t creationTime = 0;
    Commit head;
};

inline void to_json(nlohmann::json& j, const History& out);
inline void from_json(const nlohmann::json& j, History& out);

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(RefState, head, selection, selectionPrev);
//NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Snapshot::History, _prev, _next, _current);
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Snapshot, creationTime, head);

// MARK: - History Serialization
inline void to_json(nlohmann::json& j, const History& out) {
    j = {
        {"prev", out._prev},
        {"next", out._next},
        {"current", out._current},
    };
}

inline void from_json(const nlohmann::json& j, History& out) {
    j.at("prev").get_to(out._prev);
    j.at("next").get_to(out._next);
    j.at("current").get_to(out._current);
}

// MARK: - Theme
enum class Theme {
    None,
    Dark,
    Light,
};

NLOHMANN_JSON_SERIALIZE_ENUM(Theme, {
    {Theme::None, nullptr},
    {Theme::Dark, "dark"},
    {Theme::Light, "light"},
});

// MARK: - State
class State {
private:
    using _Path = std::filesystem::path;
    
    struct _State {
        License::SealedLicense license;
        bool trialExpired = false;
        Theme theme = Theme::None;
        Version lastMoveOfferVersion = 0; // The last debase version that was offerred to be moved
        
        Version updateVersion = 0;       // Cached value for the most recent version, returned by server
        int64_t updateCheckTime = 0;     // The most recent time that we checked for an update
        Version updateIgnoreVersion = 0; // The most recent version that the user ignored
    };
    
    // _StateVersion: version of our state data structure; different than Version,
    // which is the debase version!
    using _StateVersion = uint32_t;
    static constexpr _StateVersion _Version = 0; // State format version (not Debase version)
    static _Path _VersionLockFilePath(_Path dir) {
        return dir / "Version";
    }
    
    static _Path _StateFilePath(_Path rootDir) {
        return rootDir / "State";
    }
    
    static void _StateRead(_Path path, _State& state) {
        std::ifstream f(path);
        if (f) {
            f.exceptions(std::ios::failbit | std::ios::badbit);
            nlohmann::json j;
            f >> j;
            j.at("license").get_to(state.license);
            j.at("trialExpired").get_to(state.trialExpired);
            j.at("theme").get_to(state.theme);
            
            j.at("lastMoveOfferVersion").get_to(state.lastMoveOfferVersion);
            
            j.at("updateVersion").get_to(state.updateVersion);
            j.at("updateCheckTime").get_to(state.updateCheckTime);
            j.at("updateIgnoreVersion").get_to(state.updateIgnoreVersion);
        }
    }
    
    static void _StateWrite(_Path path, const _State& state) {
        std::ofstream f(path);
        f.exceptions(std::ios::failbit | std::ios::badbit);
        nlohmann::json j = {
            {"license", state.license},
            {"trialExpired", state.trialExpired},
            {"theme", state.theme},
            
            {"lastMoveOfferVersion", state.lastMoveOfferVersion},
            
            {"updateVersion", state.updateVersion},
            {"updateCheckTime", state.updateCheckTime},
            {"updateIgnoreVersion", state.updateIgnoreVersion},
        };
        f << std::setw(4) << j;
    }
    
    static _StateVersion _StateVersionRead(std::istream& stream) {
        nlohmann::json j;
        stream >> j;
        
        _StateVersion vers;
        j.get_to(vers);
        return vers;
    }
    
    static void _StateVersionWrite(std::ostream& stream, _StateVersion version) {
        nlohmann::json j = version;
        stream << j;
    }
    
    static Toastbox::FDStreamInOut _VersionLockFileOpen(_Path path) {
        constexpr mode_t Mode = (S_IRUSR|S_IWUSR) | S_IRGRP | S_IROTH;
        const int opts = (O_CREAT|O_RDWR|O_CLOEXEC);
        
        int ir = -1;
        do ir = open(path.c_str(), opts, Mode);
        while (ir<0 && errno==EINTR);
        if (ir < 0) throw std::system_error(errno, std::generic_category());
        const int fd = ir;
        
        // Lock the file
        // Ideally we'd give O_EXLOCK to open() to atomically open+lock the file,
        // which would allow us to guarantee that the file can never be observed
        // with empty content. But Linux doesn't support O_EXLOCK, so instead we
        // have to open+lock nonatomically, and after locking, read the file to
        // determine whether we need to write the version (because the file's
        // empty/corrupt) or whether someone else already wrote the version.
        do ir = flock(fd, LOCK_EX);
        while (ir && errno==EINTR);
        if (ir < 0) throw std::system_error(errno, std::generic_category());
        
        Toastbox::FDStreamInOut f(fd);
        f.exceptions(std::ios::failbit | std::ios::badbit);
        return f;
    }
    
    static void _Migrate(_Path rootDir, _StateVersion versionPrev) {
        // Stub until we have a new version
    }
    
    static void _CheckVersionAndMigrate(_Path rootDir, _StateVersion version, bool migrate) {
        if (version > _Version) {
            throw Toastbox::RuntimeError(
R"#(version of debase state on disk (v%ju) is newer than this version of debase (v%ju);
please use a newer version of debase, or delete:

   %s

*** Warning: deleting the above directory will delete all debase state, such as:
***   debase license
***   debase undo/redo history for all repositories
***   debase snapshots for all repositories
)#",
                
                (uintmax_t)version, (uintmax_t)_Version, rootDir.c_str()
            );
        
        } else if (version < _Version) {
            if (migrate) {
                _Migrate(rootDir, version);
            
            } else {
                throw Toastbox::RuntimeError(
                    "invalid version of debase state on disk (expected v%ju, got v%ju)",
                    (uintmax_t)_Version, (uintmax_t)version);
            }
        }
    }
    
    _Path _rootDir;
    _State _state;
    Toastbox::FDStreamInOut _versionLock;
    
public:
    static Toastbox::FDStreamInOut AcquireVersionLock(_Path rootDir, bool migrate) {
        // Create the state directory
        std::filesystem::create_directories(rootDir);
        
        // Try to create the version lock file
        _Path versionLockFilePath = _VersionLockFilePath(rootDir);
        Toastbox::FDStreamInOut versionLockFile = _VersionLockFileOpen(versionLockFilePath);
        _StateVersion version = _Version;
        try {
            // Try to read the state version
            version = _StateVersionRead(versionLockFile);
        
        } catch (...) {
            // We failed to read the version number; assume that we need to write it
            versionLockFile.clear(); // Clear errors that occurred when attempting to the read the version (eg EOF)
            versionLockFile.seekp(0); // Reset output position to 0
            _StateVersionWrite(versionLockFile, _Version);
        }
        
        // Check the version and migrate data if allowed
        _CheckVersionAndMigrate(rootDir, version, migrate);
        
        return versionLockFile;
    }
    
    State(_Path rootDir) : _rootDir(rootDir) {
        _versionLock = AcquireVersionLock(_rootDir, true);
        // Decode _state
        _StateRead(_StateFilePath(_rootDir), _state);
    }
    
    void write() {
        _StateWrite(_StateFilePath(_rootDir), _state);
    }
    
    const auto& license() const { return _state.license; }
    template <typename T> void license(const T& x) { _state.license = x; }
    
    const auto& trialExpired() const { return _state.trialExpired; }
    template <typename T> void trialExpired(const T& x) { _state.trialExpired = x; }
    
    const auto& theme() const { return _state.theme; }
    template <typename T> void theme(const T& x) { _state.theme = x; }
    
    const auto& lastMoveOfferVersion() const { return _state.lastMoveOfferVersion; }
    template <typename T> void lastMoveOfferVersion(const T& x) { _state.lastMoveOfferVersion = x; }
    
    const auto& updateVersion() const { return _state.updateVersion; }
    template <typename T> void updateVersion(const T& x) { _state.updateVersion = x; }
    
    const auto& updateCheckTime() const { return _state.updateCheckTime; }
    template <typename T> void updateCheckTime(const T& x) { _state.updateCheckTime = x; }
    
    const auto& updateIgnoreVersion() const { return _state.updateIgnoreVersion; }
    template <typename T> void updateIgnoreVersion(const T& x) { _state.updateIgnoreVersion = x; }
};

// MARK: - RepoState
class RepoState {
private:
    using _Path = std::filesystem::path;
    using _Json = nlohmann::json;
    
    struct _RepoState {
        std::map<Ref,History> history;
        std::map<Ref,std::vector<Snapshot>> snapshots;
    };
    
    _Path _rootDir;
    _Path _repoStateDir;
    Git::Repo _repo;
    
    _RepoState _repoState;
    std::map<Ref,History> _historyPrev;
    std::map<Ref,Snapshot> _initialSnapshot;
    
    static _Path _RepoStateDirPath(_Path dir, Git::Repo repo) {
        std::string name = std::filesystem::canonical(repo.path());
        std::replace(name.begin(), name.end(), '/', '-'); // Replace / with ~
        return dir / "Repo" / name;
    }
    
    static _Path _RepoStateFilePath(_Path repoStateDir) {
        return repoStateDir / "State";
    }
    
    static void _RepoStateRead(_Path path, _RepoState& state) {
        std::ifstream f(path);
        if (f) {
            f.exceptions(std::ios::failbit | std::ios::badbit);
            nlohmann::json j;
            f >> j;
            j.at("history").get_to(state.history);
            j.at("snapshots").get_to(state.snapshots);
        }
    }
    
    static void _RepoStateWrite(_Path path, const _RepoState& state) {
        std::ofstream f(path);
        f.exceptions(std::ios::failbit | std::ios::badbit);
        nlohmann::json j = {
            {"history", state.history},
            {"snapshots", state.snapshots},
        };
        f << std::setw(4) << j;
    }
    
    static std::vector<Snapshot> _CleanSnapshots(const std::vector<Snapshot>& snapshots) {
        std::map<Commit,Snapshot> earliestSnapshot;
        for (const Snapshot& snap : snapshots) {
            auto find = earliestSnapshot.find(snap.head);
            if (find==earliestSnapshot.end() || snap.creationTime<find->second.creationTime) {
                earliestSnapshot[snap.head] = snap;
            }
        }
        
        std::vector<Snapshot> r;
        for (auto i : earliestSnapshot) {
            r.push_back(i.second);
        }
        std::sort(r.begin(), r.end());
        return r;
    }
    
public:
    RepoState() {}
    RepoState(_Path rootDir, Git::Repo repo, const std::set<Git::Ref>& refs) :
    _rootDir(rootDir), _repoStateDir(_RepoStateDirPath(_rootDir, repo)), _repo(repo) {
        Toastbox::FDStreamInOut versionLockFile = State::AcquireVersionLock(_rootDir, true);
        
        // Decode _repoState
        _RepoStateRead(_RepoStateFilePath(_repoStateDir), _repoState);
        
        // Ensure that _state.history has an entry for each ref
        // Also ensure that if the stored history head doesn't
        // match the current head, we create a fresh history.
        for (Git::Ref ref : refs) {
            Git::Commit headCurrent = ref.commit();
            Git::Commit headStored;
            auto find = _repoState.history.find(Convert(ref));
            if (find != _repoState.history.end()) {
                // Handle the stored commit not existing
                try {
                    headStored = Convert(_repo, find->second.get().head);
                } catch (...) {}
            }
            
            if (headStored != headCurrent) {
                _repoState.history[Convert(ref)] = History(RefState{.head = Convert(headCurrent)});
            }
        }
        
        // Remember the initial history so we can tell if it changed upon exit,
        // so we know whether to save it. (We don't want to save histories that
        // didn't change to help prevent clobbering changes from another
        // session.)
        _historyPrev = _repoState.history;
        
        // Ensure that `_snapshots` has an entry for every ref
        for (Git::Ref ref : refs) {
            _repoState.snapshots[Convert(ref)];
        }
        
        // Populate _initialSnapshot
        for (Git::Ref ref : refs) {
            _initialSnapshot[Convert(ref)] = ref.commit();
        }
    }
    
    void write() {
        Toastbox::FDStreamInOut versionLockFile = State::AcquireVersionLock(_rootDir, false);
        
        // Read existing state
        _RepoState repoState;
        _RepoStateRead(_RepoStateFilePath(_repoStateDir), repoState);
        
        // Update `state.history`/`state.snapshots` with only the entries that changed, and
        // therefore need to be written.
        // We use this strategy so that we don't clobber data written by another debase
        // session, for unrelated refs. If two debase sessions made modifications to the same
        // refs, then the one that write later wins.
        for (const auto& i : _repoState.history) {
            Ref ref = i.first;
            const History& refHistory = i.second;
            const History& refHistoryPrev = _historyPrev.at(ref);
            if (refHistory != refHistoryPrev) {
                repoState.history[ref] = refHistory;
                
                std::vector<Snapshot> refSnapshots = _repoState.snapshots.at(ref);
                refSnapshots.push_back(refHistory.get().head);
                refSnapshots.push_back(_initialSnapshot.at(ref));
                // Remove duplicate snapshots and sort them
                refSnapshots = _CleanSnapshots(refSnapshots);
                
                repoState.snapshots[ref] = refSnapshots;
            }
        }
        
        std::filesystem::create_directories(_repoStateDir);
        _RepoStateWrite(_RepoStateFilePath(_repoStateDir), repoState);
    }
    
    Snapshot& initialSnapshot(Git::Ref ref) {
        return _initialSnapshot.at(Convert(ref));
    }
    
    History& history(Git::Ref ref) {
        return _repoState.history.at(Convert(ref));
    }
    
    const std::vector<Snapshot>& snapshots(Git::Ref ref) const {
        return _repoState.snapshots.at(Convert(ref));
    }
    
    Git::Repo repo() const {
        return _repo;
    }
};

} // namespace State
