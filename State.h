#pragma once
#include <deque>
#include "lib/Toastbox/RuntimeError.h"
#include "lib/nlohmann/json.h"
#include "UndoState.h"
#include "Git.h"

using UndoState = T_UndoState<Git::Commit>;

struct RepoState {
    Git::Repo repo;
    std::map<Git::Ref,UndoState> undoStates;
};

struct State {
    static constexpr uint32_t Version = 0;
    std::map<std::filesystem::path,RepoState> repoStates;
};

// MARK: - Commit Serialization
inline void to_json(nlohmann::json& j, const Git::Commit& x) {
    using namespace nlohmann;
    j = Git::StringForId(x.id());
}

inline void from_json(const nlohmann::json& j, Git::Commit& x, Git::Repo repo) {
    using namespace nlohmann;
    std::string id;
    j.get_to(id);
    x = repo.commitLookup(id);
}

inline void from_json(const nlohmann::json& j, std::deque<Git::Commit>& x, Git::Repo repo) {
    using namespace nlohmann;
    std::vector<json> commits;
    j.get_to(commits);
    for (const json& j : commits) {
        ::from_json(j, x.emplace_back(), repo);
    }
}

// MARK: - Ref Serialization
inline void to_json(nlohmann::json& j, const Git::Ref& x) {
    using namespace nlohmann;
    j = x.fullName();
}

inline void from_json(const nlohmann::json& j, Git::Ref& x, Git::Repo repo) {
    using namespace nlohmann;
    std::string name;
    j.get_to(name);
    x = repo.refLookup(name);
}

// MARK: - UndoState Serialization
inline void to_json(nlohmann::json& j, const UndoState& x) {
    using namespace nlohmann;
    j = {
        {"undo", x._undo},
        {"redo", x._redo},
        {"current", x._current},
    };
}

inline void from_json(const nlohmann::json& j, UndoState& x, Git::Repo repo) {
    using namespace nlohmann;
    ::from_json(j.at("undo"), x._undo, repo);
    ::from_json(j.at("redo"), x._redo, repo);
    ::from_json(j.at("current"), x._current, repo);
}

// MARK: - Repo Serialization
inline void to_json(nlohmann::json& j, const Git::Repo& x) {
    using namespace nlohmann;
    j = std::filesystem::canonical(x.path()).string();
}

inline void from_json(const nlohmann::json& j, Git::Repo& x) {
    using namespace nlohmann;
    std::filesystem::path path;
    j.get_to(path);
    x = Git::Repo::Open(path);
}

// MARK: - RepoState Serialization
inline void to_json(nlohmann::json& j, const RepoState& x) {
    using namespace nlohmann;
    j = {
        {"repo", x.repo},
        {"undoStates", x.undoStates},
    };
}

inline void from_json(const nlohmann::json& j, std::map<Git::Ref,UndoState>& x, Git::Repo repo) {
    using namespace nlohmann;
    std::map<json,json> map;
    j.get_to(map);
    for (const auto& i : map) {
        Git::Ref ref;
        UndoState s;
        ::from_json(i.first, ref, repo);
        ::from_json(i.second, s, repo);
        x[ref] = s;
    }
}

inline void from_json(const nlohmann::json& j, RepoState& x) {
    using namespace nlohmann;
    j.at("repo").get_to(x.repo);
    ::from_json(j.at("undoStates"), x.undoStates, x.repo);
}

// MARK: - State Serialization
inline void to_json(nlohmann::json& j, const State& x) {
    using namespace nlohmann;
    j = {
        {"version", State::Version},
        {"repoStates", x.repoStates},
    };
}

inline void from_json(const nlohmann::json& j, State& x) {
    uint32_t version = 0;
    j.at("version").get_to(version);
    if (version != State::Version) {
        throw Toastbox::RuntimeError("invalid version (expected %ju, got %ju)",
        (uintmax_t)State::Version, (uintmax_t)version);
    }
    
    j.at("repoStates").get_to(x.repoStates);
}
