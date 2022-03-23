#pragma once
#include <deque>
#include "lib/Toastbox/RuntimeError.h"
#include "lib/nlohmann/json.h"
#include "UndoHistory.h"
#include "Git.h"

struct RefState {
    Git::Commit head;
    std::set<Git::Commit> selection;
    std::set<Git::Commit> selectionPrev;
};

using UndoHistory = T_UndoHistory<RefState>;

struct RepoState {
    static constexpr uint32_t Version = 0;
//    std::filesystem::path repo;
    std::map<Git::Ref,UndoHistory> undoStates;
};

#warning TODO: reconsider from_json arg ordering; should the thing being set always be the last argument?

inline void from_json(const nlohmann::json& j, Git::Commit& x, Git::Repo repo);
inline void from_json(const nlohmann::json& j, RefState& x, Git::Repo repo);

template <typename T>
inline void from_json_container(const nlohmann::json& j, T& container, Git::Repo repo) {
    using namespace nlohmann;
    using T_Elm = typename T::value_type;
    std::vector<json> elms;
    j.get_to(elms);
    for (const json& j : elms) {
        T_Elm x;
        ::from_json(j, x, repo);
        container.insert(container.end(), x);
    }
}

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
    ::from_json_container(j.at("selection"), x.selection, repo);
    ::from_json_container(j.at("selectionPrev"), x.selectionPrev, repo);
}

// MARK: - Ref Serialization
inline void from_json(const nlohmann::json& j, Git::Ref& x, Git::Repo repo) {
    std::string name;
    j.get_to(name);
    x = repo.refLookup(name);
}

// MARK: - UndoHistory Serialization
inline void to_json(nlohmann::json& j, const UndoHistory& x) {
    j = {
        {"undo", x._undo},
        {"redo", x._redo},
        {"current", x._current},
    };
}

inline void from_json(const nlohmann::json& j, UndoHistory& x, Git::Repo repo) {
    ::from_json_container(j.at("undo"), x._undo, repo);
    ::from_json_container(j.at("redo"), x._redo, repo);
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

// MARK: - RepoState Serialization
inline void to_json(nlohmann::json& j, const RepoState& x) {
    j = {
//        {"repo", x.repo},
        {"version", RepoState::Version},
        {"undoStates", x.undoStates},
    };
    
//    printf("%s\n", j.dump().c_str());
}

inline void from_json(const nlohmann::json& j, std::map<Git::Ref,UndoHistory>& x, Git::Repo repo) {
    using namespace nlohmann;
    std::map<json,json> map;
    j.get_to(map);
    for (const auto& i : map) {
        Git::Ref ref;
        UndoHistory s;
        try {
            ::from_json(i.first, ref, repo);
        } catch (...) {
            // If we fail to deserialize a ref, ignore this UndoHistory entry
            continue;
        }
        ::from_json(i.second, s, repo);
        x[ref] = s;
    }
}

inline void from_json(const nlohmann::json& j, RepoState& x, Git::Repo repo) {
//    j.at("repo").get_to(x.repo);
    uint32_t version = 0;
    j.at("version").get_to(version);
    if (version != RepoState::Version) {
        throw Toastbox::RuntimeError("invalid version (expected %ju, got %ju)",
        (uintmax_t)RepoState::Version, (uintmax_t)version);
    }
    
    ::from_json(j.at("undoStates"), x.undoStates, repo);
}

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
