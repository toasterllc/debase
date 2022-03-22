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
    
//    Json serialize() const {
//        return {
//            {"repo", repo},
//            {"undoStates", undoStates},
//        };
//    }
//    
//    void deserialize(const Json& j) {
//        j.at("repo").get_to(repo);
//        
//        std::map<Json,Json> us;
//        j.at("undoStates").get_to(us);
//        for (const auto& i : us) {
//            Git::Ref ref;
//            UndoState s;
//            from_json(i.first, ref);
//            from_json(i.second, s, repo);
//            undoStates[ref] = s;
//        }
//    }
};

struct State {
    static constexpr uint32_t Version = 0;
    std::map<std::filesystem::path,RepoState> repoStates;
    
//    Json serialize() const {
//        Json data;
//        data["version"] = _Version;
//        data["repoStates"] = 0;
//        return data;
//    }
//    
//    void deserialize(const Json& data) {
//        
//    }
    
//    void write(std::ofstream& f) const {
//        f.write((char*)&_Version, sizeof(_Version));
//        assert(repoState.size() <= UINT32_MAX);
//        
//        uint32_t count = (uint32_t)repoState.size();
//        f.write((char*)&count, sizeof(count));
//        
//        for (const auto& keyval : repoState) {
//            const std::string_view path = keyval.first.native();
//            const RepoState& repoState = keyval.second;
//            assert(path.size() <= UINT32_MAX);
//            uint32_t len = (uint32_t)path.size();
//            f.write((char*)&len, sizeof(len));
//            f.write(path.data(), len);
//            repoState.write(f);
//        }
//    }
//    
//    void read(std::ifstream& f) {
//        uint32_t version = 0;
//        f.read((char*)&version, sizeof(version));
//        if (version != _Version)
//            throw Toastbox::RuntimeError("invalid version (expected %ju, got %ju)",
//            (uintmax_t)_Version, (uintmax_t)version);
//        
//        uint32_t count = 0;
//        f.read(&count, sizeof(count));
//        
//        for (uint32_t i=0; i<count; i++) {
//            uint32_t len = 0;
//            f.read(&len, sizeof(len));
//            
//            std::string str;
//            str.reserve();
//            f.read
//        }
//        
//        for (const auto& keyval : repoState) {
//            const std::string_view path = keyval.first.native();
//            const RepoState& repoState = keyval.second;
//            assert(path.size() <= UINT32_MAX);
//            uint32_t len = (uint32_t)path.size();
//            f.write((char*)&len, sizeof(len));
//            f.write(path.data(), len);
//            repoState.write(f);
//        }
//    }
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
    
//    json undoStates;
//    for (const auto& keyval : x.undoStates) {
//        std::string refName = keyval.first.fullName();
//        undoStates[refName] = 1;
//    }
//    
//    j = {
//        {"repo", std::filesystem::canonical(x.repo.path())},
//        {"undoStates", undoStates},
//    };
}

inline void from_json(const nlohmann::json& j, RepoState& x) {
    using namespace nlohmann;
    
    j.at("repo").get_to(x.repo);
    
    std::map<json,json> us;
    j.at("undoStates").get_to(us);
    for (const auto& i : us) {
        Git::Ref ref;
        UndoState s;
        ::from_json(i.first, ref, x.repo);
        ::from_json(i.second, s, x.repo);
        x.undoStates[ref] = s;
    }
    
//    std::filesystem::path repoPath;
//    j.at("repo").get_to(repoPath);
//    x.repo = Git::Repo::Open(repoPath);
//    
//    const json& undoStates = j.at("undoStates");
//    if (undoStates.type() != json::value_t::object) {
//        throw Toastbox::RuntimeError("undoStates isn't an object");
//    }
//    
//    for (const auto& keyval : undoStates) {
//        keyval
//    }
//    
//    j.at("undoStates").get_to(x.undoStates);
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
