#pragma once
#include <set>
#include <fstream>
#include "lib/Toastbox/RuntimeError.h"
#include "lib/nlohmann/json.h"
#include "History.h"
#include "Git.h"

//namespace State {

std::filesystem::path StateDir();

inline std::string _Sub(std::string x) {
    std::replace(x.begin(), x.end(), '/', '~'); // Replace / with ~
    return x;
}

inline std::filesystem::path RepoStateDir(Git::Repo repo) {
    std::string name = _Sub(std::filesystem::canonical(repo.path()));
    return StateDir() / "Repo" / name;
}

inline std::filesystem::path RefHistoryStateDir(Git::Repo repo) {
    return RepoStateDir(repo) / "RefHistory";
}

inline std::filesystem::path RefHistoryStateName(Git::Ref ref) {
    return _Sub(ref.fullName());
}

struct RefState {
    Git::Commit head;
    std::set<Git::Commit> selection;
    std::set<Git::Commit> selectionPrev;
};

using RefHistory = T_History<RefState>;
using RefHistorys = std::map<Git::Commit,RefHistory>;

inline void to_json(nlohmann::json& j, const Git::Commit& x);
inline void from_json(const nlohmann::json& j, Git::Commit& x, Git::Repo repo);

inline void to_json(nlohmann::json& j, const RefState& x);
inline void from_json(const nlohmann::json& j, RefState& x, Git::Repo repo);

inline void to_json(nlohmann::json& j, const RefHistory& x);
inline void from_json(const nlohmann::json& j, RefHistory& x, Git::Repo repo);

#warning TODO: reconsider from_json arg ordering; should the thing being set always be the last argument?

template <typename T>
inline void from_json_vector(const nlohmann::json& j, T& v, Git::Repo repo) {
    using namespace nlohmann;
    using T_Elm = typename T::value_type;
    std::vector<json> elms;
    j.get_to(elms);
    for (const json& j : elms) {
        T_Elm x;
        ::from_json(j, x, repo);
        v.insert(v.end(), x);
    }
}

template <typename T>
inline void from_json_map(const nlohmann::json& j, T& m, Git::Repo repo) {
    using namespace nlohmann;
    using T_Key = typename T::key_type;
    using T_Val = typename T::mapped_type;
    std::map<json,json> elms;
    j.get_to(elms);
    for (const auto& i : elms) {
        T_Key key;
        T_Val val;
        ::from_json(i.first, key, repo);
        ::from_json(i.second, val, repo);
        m[key] = val;
    }
}

class RepoState {
public:
    RepoState() {}
    RepoState(Git::Repo repo, const std::vector<Git::Rev>& revs) : _repo(repo) {
        for (const Git::Rev& rev : revs) {
            try {
                _Path fpath = RefHistoryStateDir(_repo) / RefHistoryStateName(rev.ref);
                std::ifstream f(fpath);
                f.exceptions(std::ofstream::failbit | std::ofstream::badbit);
                nlohmann::json j;
                f >> j;
                
                RefHistorys& hs = _refHistorys[rev.ref];
                ::from_json_map(j, hs, repo);
            
            // Ignore deserialization errors (eg file not existing)
            } catch (...) {}
        }
        
        // Populate _refHistory by looking up the RefHistory for each ref,
        // by looking at its current commit.
        // We also delete the matching commit->RevHistory entry after copying it
        // into `_refHistory`, because during this session we may modify the
        // ref, and so that entry will be stale (and if we didn't prune them,
        // the state file would grow indefinitely.)
        for (const Git::Rev& rev : revs) {
            if (rev.ref) {
                RefHistorys& hs = _refHistorys[rev.ref];
                RefHistory& h = _refHistory[rev.ref];
                if (auto find=hs.find(rev.commit); find!=hs.end()) {
                    h = find->second;
                } else {
                    h.clear();
                    h.set(RefState{
                        .head = rev.commit,
                    });
                }
                
                hs.erase(rev.commit);
            }
        }
        
//        try {
//            _Path fdir = RepoStateDir();
//            _Path fpath = fdir / RepoStateFileName(_repo.path());
//            std::ifstream f(fpath);
//            f.exceptions(std::ofstream::failbit | std::ofstream::badbit);
//            nlohmann::json j;
//            f >> j;
//            
//            uint32_t version = 0;
//            j.at("version").get_to(version);
//            if (version != _Version) {
//                throw Toastbox::RuntimeError("invalid version (expected %ju, got %ju)",
//                (uintmax_t)_Version, (uintmax_t)version);
//            }
//            
//            j.at("history").get_to(_historyRaw);
//        
//        // Ignore deserialization errors (eg file not existing)
//        } catch (...) {}
    }
    
    void write() {
        // Update _refHistorys from _refHistory
        for (const auto& i : _refHistory) {
            Git::Ref ref = _repo.refReload(i.first);
            const RefHistory& h = i.second;
            _refHistorys[ref][ref.commit()] = h;
        }
        
        for (const auto& i : _refHistorys) {
            const Git::Ref& ref = i.first;
            _Path fdir = RefHistoryStateDir(_repo);
            _Path fpath = fdir / RefHistoryStateName(ref);
            std::filesystem::create_directories(fdir);
            std::ofstream f(fpath);
            f.exceptions(std::ofstream::failbit | std::ofstream::badbit);
            
            nlohmann::json j = _refHistorys.at(ref);
            f << std::setw(4) << j;
        }
        
        
//        for (const auto& i : _refHistory) {
//            _historyRaw[i.first] = i.second;
//        }
    }
    
    RefHistory& refHistory(Git::Ref ref) {
        return _refHistory.at(ref);
    }
    
//    CommitHistoryMap commitHistoryMap(Git::Ref ref) {
//        CommitHistoryMap x;
//        auto find = _history.find(ref.fullName());
//        if (find != _history.end()) {
//            ::from_json_map(find->second, x, _repo);
//        }
//        return x;
//    }
//    
//    void setCommitHistoryMap(Git::Ref ref, const CommitHistoryMap& x) {
//        _Json j = x;
//        _history[ref.fullName()] = j;
//    }
    
    Git::Repo repo() const {
        return _repo;
    }
    
//    void _RepoStateRead(RepoState& state) {
//        try {
//            fs::path fdir = RepoStateDir();
//            fs::path fpath = fdir / _RepoStateFileName(state.repo().path());
//            std::ifstream f(fpath);
//            f.exceptions(std::ofstream::failbit | std::ofstream::badbit);
//            nlohmann::json j;
//            f >> j;
//            ::from_json(j, state, state.repo());
//        
//        // Ignore errors (eg file not existing)
//        } catch (...) {}
//    }

//    static void _RepoStateWrite(const RepoState& state) {
//        fs::path fdir = RepoStateDir();
//        fs::create_directories(fdir);
//        fs::path fpath = fdir / _RepoStateFileName(state.repo().path());
//        std::ofstream f(fpath);
//        f.exceptions(std::ofstream::failbit | std::ofstream::badbit);
//        nlohmann::json j = state;
//        f << std::setw(4) << j;
//    }
private:
    using _Json = nlohmann::json;
    using _Path = std::filesystem::path;
    static constexpr uint32_t _Version = 0;
    Git::Repo _repo;
    std::map<Git::Ref,RefHistorys> _refHistorys;
    std::map<Git::Ref,RefHistory> _refHistory;
};

// MARK: - Commit Serialization
inline void to_json(nlohmann::json& j, const Git::Commit& x) {
    j = Git::StringForId(x.id());
}

inline void from_json(const nlohmann::json& j, Git::Commit& x, Git::Repo repo) {
    std::string id;
    j.get_to(id);
    x = repo.commitLookup(id);
}

//inline void from_json(const nlohmann::json& j, std::deque<Git::Commit>& x, Git::Repo repo) {
//    using namespace nlohmann;
//    std::vector<json> commits;
//    j.get_to(commits);
//    for (const json& j : commits) {
//        ::from_json(j, x.emplace_back(), repo);
//    }
//}

// MARK: - RefState Serialization
inline void to_json(nlohmann::json& j, const RefState& x) {
    j = {
        {"head", x.head},
        {"selection", x.selection},
        {"selectionPrev", x.selectionPrev},
    };
}

inline void from_json(const nlohmann::json& j, RefState& x, Git::Repo repo) {
    ::from_json(j.at("head"), x.head, repo);
    ::from_json_vector(j.at("selection"), x.selection, repo);
    ::from_json_vector(j.at("selectionPrev"), x.selectionPrev, repo);
}

// MARK: - Ref Serialization
inline void from_json(const nlohmann::json& j, Git::Ref& x, Git::Repo repo) {
    std::string name;
    j.get_to(name);
    x = repo.refLookup(name);
}

// MARK: - History Serialization
inline void to_json(nlohmann::json& j, const RefHistory& x) {
    j = {
        {"prev", x._prev},
        {"next", x._next},
        {"current", x._current},
    };
}

inline void from_json(const nlohmann::json& j, RefHistory& x, Git::Repo repo) {
    ::from_json_vector(j.at("prev"), x._prev, repo);
    ::from_json_vector(j.at("next"), x._next, repo);
    ::from_json(j.at("current"), x._current, repo);
}

//// MARK: - Repo Serialization
//inline void to_json(nlohmann::json& j, const Git::Repo& x) {
//    j = std::filesystem::canonical(x.path()).string();
//}
//
//inline void from_json(const nlohmann::json& j, Git::Repo& x) {
//    std::filesystem::path path;
//    j.get_to(path);
//    x = Git::Repo::Open(path);
//}

//// MARK: - RepoState Serialization
//inline void to_json(nlohmann::json& j, const RepoState& x) {
//    j = {
////        {"repo", x.repo},
//        {"version", RepoState::_Version},
//        {"history", x._history},
//    };
//    
////    printf("%s\n", j.dump().c_str());
//}

//inline void from_json(const nlohmann::json& j, RepoState& x, Git::Repo repo) {
////    j.at("repo").get_to(x.repo);
//    uint32_t version = 0;
//    j.at("version").get_to(version);
//    if (version != RepoState::_Version) {
//        throw Toastbox::RuntimeError("invalid version (expected %ju, got %ju)",
//        (uintmax_t)RepoState::_Version, (uintmax_t)version);
//    }
//    
//    j.at("history").get_to(x._history);
//    
////    ::from_json(j.at("history"), x._history);
//}



//inline void from_json(const nlohmann::json& j, RefCommitHistory& x, Git::Repo repo) {
//    using namespace nlohmann;
//    std::map<json,json> map;
//    j.get_to(map);
//    for (const auto& i : map) {
//        Git::Ref ref;
//        History s;
//        try {
//            ::from_json(i.first, ref, repo);
//        } catch (...) {
//            // If we fail to deserialize a ref, ignore this History entry
//            continue;
//        }
//        ::from_json(i.second, s, repo);
//        x[ref] = s;
//    }
//}


//// MARK: - State Serialization
//inline void to_json(nlohmann::json& j, const State& x) {
//    j = {
//        {"version", State::Version},
//        {"repoStates", x.repoStates},
//    };
//}
//
//inline void from_json(const nlohmann::json& j, State& x) {
//    uint32_t version = 0;
//    j.at("version").get_to(version);
//    if (version != State::Version) {
//        throw Toastbox::RuntimeError("invalid version (expected %ju, got %ju)",
//        (uintmax_t)State::Version, (uintmax_t)version);
//    }
//    
//    j.at("repoStates").get_to(x.repoStates);
//}

//} // namespace State
