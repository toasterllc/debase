#pragma once
#include <set>
#include <fstream>
#include <sys/stat.h>
#include <fcntl.h>
#include "lib/Toastbox/RuntimeError.h"
#include "lib/Toastbox/FileDescriptor.h"
#include "lib/Toastbox/FDStream.h"
#include "lib/nlohmann/json.h"
#include "History.h"
#include "Git.h"

template <typename T>
inline void from_json(const nlohmann::json& j, Git::Repo repo, std::deque<T>& out);
template <typename T>
inline void from_json(const nlohmann::json& j, Git::Repo repo, std::set<T>& out);
template <typename T_Key, typename T_Val>
inline void from_json(const nlohmann::json& j, Git::Repo repo, std::map<T_Key,T_Val>& out);

struct RefState {
    Git::Commit head;
    std::set<Git::Commit> selection;
    std::set<Git::Commit> selectionPrev;
    
    bool operator==(const RefState& x) const {
        if (head != x.head) return false;
        if (selection != x.selection) return false;
        if (selectionPrev != x.selectionPrev) return false;
        return true;
    }
    
    bool operator!=(const RefState& x) const {
        return !(*this==x);
    }
};

using RefHistory = T_History<RefState>;

// MARK: - Ref Serialization
inline void from_json(const nlohmann::json& j, Git::Repo repo, Git::Ref& out) {
    std::string name;
    j.get_to(name);
    out = repo.refLookup(name);
}

// MARK: - Commit Serialization
inline void from_json(const nlohmann::json& j, Git::Repo repo, Git::Commit& out) {
    std::string id;
    j.get_to(id);
    out = repo.commitLookup(id);
}

// MARK: - RefState Serialization
inline void to_json(nlohmann::json& j, const RefState& out) {
    j = {
        {"head", out.head},
        {"selection", out.selection},
        {"selectionPrev", out.selectionPrev},
    };
}

inline void from_json(const nlohmann::json& j, Git::Repo repo, RefState& out) {
    ::from_json(j.at("head"), repo, out.head);
    ::from_json(j.at("selection"), repo, out.selection);
    ::from_json(j.at("selectionPrev"), repo, out.selectionPrev);
}

// MARK: - RefHistory Serialization
inline void to_json(nlohmann::json& j, const RefHistory& out) {
    j = {
        {"prev", out._prev},
        {"next", out._next},
        {"current", out._current},
    };
}

inline void from_json(const nlohmann::json& j, Git::Repo repo, RefHistory& out) {
    ::from_json(j.at("prev"), repo, out._prev);
    ::from_json(j.at("next"), repo, out._next);
    ::from_json(j.at("current"), repo, out._current);
}

template <typename T>
inline void from_json_vector(const nlohmann::json& j, Git::Repo repo, T& out) {
    using namespace nlohmann;
    using T_Elm = typename T::value_type;
    std::vector<json> elms;
    j.get_to(elms);
    for (const json& j : elms) {
        T_Elm elm;
        ::from_json(j, repo, elm);
        out.insert(out.end(), elm);
    }
}

template <typename T>
inline void from_json(const nlohmann::json& j, Git::Repo repo, std::deque<T>& out) {
    from_json_vector(j, repo, out);
}

template <typename T>
inline void from_json(const nlohmann::json& j, Git::Repo repo, std::set<T>& out) {
    from_json_vector(j, repo, out);
}

template <typename T_Key, typename T_Val>
inline void from_json(const nlohmann::json& j, Git::Repo repo, std::map<T_Key,T_Val>& out) {
    using namespace nlohmann;
    std::map<json,json> elms;
    j.get_to(elms);
    for (const auto& i : elms) {
        T_Key key;
        T_Val val;
        ::from_json(i.first, repo, key);
        ::from_json(i.second, repo, val);
        out[key] = val;
    }
}

class RepoState {
private:
    using _Path = std::filesystem::path;
    using _Json = nlohmann::json;
    
    static constexpr uint32_t _Version = 0;
    
    _Path _stateDir;
    _Path _repoStateDir;
    Git::Repo _repo;
    
    std::map<Git::Ref,RefHistory> _refHistorysPrev;
    std::map<Git::Ref,RefHistory> _refHistorys;
    
    static _Path _VersionLockFilePath(_Path dir) {
        return dir / "Version";
    }
    
    static _Path _RepoStateDirPath(_Path dir, Git::Repo repo) {
        std::string name = std::filesystem::canonical(repo.path());
        std::replace(name.begin(), name.end(), '/', '-'); // Replace / with ~
        return dir / "Repo" / name;
    }
    
    static uint32_t _VersionRead(std::istream& stream) {
        nlohmann::json j;
        stream >> j;
        return j;
    }
    
    static void _VersionWrite(std::ostream& stream, uint32_t version) {
        nlohmann::json j = version;
        stream << j;
    }
    
    static Toastbox::FDStreamInOut _VersionLockFileOpen(_Path path, bool create) {
        constexpr mode_t Mode = (S_IRUSR|S_IWUSR) | S_IRGRP | S_IROTH;
        const int opts = (O_RDWR|O_EXLOCK) | (create ? (O_CREAT|O_EXCL) : 0);
        int ir = -1;
        do ir = open(path.c_str(), opts, Mode);
        while (errno==EINTR);
        if (ir < 0) return Toastbox::FDStreamInOut();
        Toastbox::FDStreamInOut f(ir);
        f.exceptions(std::ios::failbit | std::ios::badbit);
        return f;
    }
    
    void _migrate(uint32_t versionPrev) {
        // Stub until we have a new version
    }
    
    void _checkVersionAndMigrate(std::istream& stream, bool migrate) {
        uint32_t version = _VersionRead(stream);
        if (version > _Version) {
            throw Toastbox::RuntimeError(
                "version of debase state on disk (v%ju) is newer than this version of debase (v%ju);\n"
                "please use a newer version of debase, or delete:\n"
                "  %s", (uintmax_t)version, (uintmax_t)_Version, _stateDir.c_str());
        
        } else if (version < _Version) {
            if (migrate) {
                _migrate(version);
            
            } else {
                throw Toastbox::RuntimeError(
                    "invalid version of debase state on disk (expected v%ju, got v%ju)",
                    (uintmax_t)_Version, (uintmax_t)version);
            }
        }
    }
    
public:
    RepoState() {}
    RepoState(_Path stateDir, Git::Repo repo, const std::set<Git::Ref>& refs) :
    _stateDir(stateDir), _repoStateDir(_RepoStateDirPath(_stateDir, repo)), _repo(repo) {
        // Create the state directory
        std::filesystem::create_directories(_stateDir);
        
        // Try to create the version lock file
        _Path versionLockFilePath = _VersionLockFilePath(_stateDir);
        Toastbox::FDStreamInOut versionLockFile = _VersionLockFileOpen(versionLockFilePath, true);
        if (versionLockFile) {
            // We created the version lock file
            // Write our version number
            _VersionWrite(versionLockFile, _Version);
        
        } else {
            // We weren't able to create the version lock file, presumably because it already exists.
            // Try just opening it instead.
            versionLockFile = _VersionLockFileOpen(versionLockFilePath, false);
            _checkVersionAndMigrate(versionLockFile, true);
        }
        
        // Manually decode refHistorys
        // We want to do this manually because we want to implement exception
        // handling at specific points in the ref->commit->RefHistory graph
        // decoding: if we failed to deserialize the current ref, we still
        // want to try to deserialize the next ref; if we failed to
        // deserialize a commit/RefHistory entry, we still want to try to
        // deserialize the next entry.
        std::map<Git::Ref,std::map<Git::Commit,RefHistory>> refHistorys;
        nlohmann::json refHistorysJson;
        try {
            _Path fpath = _repoStateDir / "RefHistory";
            std::ifstream f(fpath);
            f.exceptions(std::ofstream::failbit | std::ofstream::badbit);
            f >> refHistorysJson;
        // Ignore file errors (eg file not existing)
        } catch (...) {}
            
        std::map<_Json,std::map<_Json,_Json>> refHistorysJsonMap;
        refHistorysJson.get_to(refHistorysJsonMap);
        
        for (const auto& i : refHistorysJsonMap) {
            const std::map<_Json,_Json>& map = i.second; // Commit -> RefHistory
            Git::Ref ref;
            try {
                ::from_json(i.first, _repo, ref);
            } catch (...) { continue; }
            
            for (const auto& i : map) {
                Git::Commit commit;
                RefHistory refHistory;
                
                try {
                    ::from_json(i.first, _repo, commit);
                    ::from_json(i.second, _repo, refHistory);
                } catch (...) { continue; }
                
                refHistorys[ref][commit] = refHistory;
            }
        }
        
        // Populate _refHistory by looking up the RefHistory for each ref,
        // by looking at its current commit.
        // We also delete the matching commit->RevHistory entry after copying it
        // into `_refHistory`, because during this session we may modify the
        // ref, and so that entry will be stale (and if we didn't prune them,
        // the state file would grow indefinitely.)
        for (const Git::Ref& ref : refs) {
            Git::Commit refCommit = ref.commit();
            std::map<Git::Commit,RefHistory>& refHistoryMap = refHistorys[ref];
            RefHistory& refHistory = _refHistorys[ref];
            if (auto find=refHistoryMap.find(refCommit); find!=refHistoryMap.end()) {
                refHistory = find->second;
            } else {
                refHistory.clear();
                refHistory.set(RefState{
                    .head = refCommit,
                });
            }
            
            _refHistorysPrev[ref] = refHistory;
        }
    }
    
    void write() {
        _Path fpath = _repoStateDir / "RefHistory";
        
        // Lock the state dir via the version lock
        Toastbox::FDStreamInOut versionLockFile = _VersionLockFileOpen(_VersionLockFilePath(_stateDir), false);
        _checkVersionAndMigrate(versionLockFile, false);
        
        // refHistory: intentional loose '_Json' typing because we want to maintain all
        // of its data, even if, eg we fail to construct a Ref because it doesn't exist.
        // If we had strong typing, and we couldn't construct the Ref, we'd throw out the
        // entry and the data would be lost when we write the json file below.
        std::map<_Json,std::map<_Json,_Json>> refHistorysJson;
        try {
            std::ifstream f(fpath);
            f.exceptions(std::ofstream::failbit | std::ofstream::badbit);
            nlohmann::json j;
            f >> j;
            j.get_to(refHistorysJson);
        
        // Ignore deserialization errors (eg file not existing)
        } catch (...) {}
        
        for (const auto& i : _refHistorysPrev) {
            Git::Ref ref = _repo.refReload(i.first);
            const RefHistory& refHistory = _refHistorys.at(ref);
            const RefHistory& refHistoryPrev = i.second;
            Git::Commit commit = ref.commit();
            // Ignore entries that didn't change
            if (refHistory == refHistoryPrev) continue;
            // The ref was modified, so erase the old Commit->RefHistory entry, and insert the new one.
            std::map<_Json,_Json>& refHistoryJson = refHistorysJson[ref];
            refHistoryJson.erase(refHistoryPrev.get().head);
            refHistoryJson[commit] = _refHistorys.at(ref);
        }
        
        std::filesystem::create_directories(_repoStateDir);
        std::ofstream f(fpath);
        f.exceptions(std::ofstream::failbit | std::ofstream::badbit);
        f << std::setw(4) << refHistorysJson;
    }
    
    RefHistory& refHistory(Git::Ref ref) {
        return _refHistorys.at(ref);
    }
    
    Git::Repo repo() const {
        return _repo;
    }
};
