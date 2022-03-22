#pragma once
#include <deque>
#include "lib/Toastbox/RuntimeError.h"
#include "lib/nlohmann/json.h"

struct UndoHistory {
    using History = std::deque<int>;
    using Iter = History::const_iterator;
    using RIter = History::const_reverse_iterator;
    History history;
    Iter position = history.begin();
    
    void undo() {
        assert(canUndo());
        position++;
    }
    
    void redo() {
        assert(canRedo());
        position--;
    }
    
    Iter undoIter() const { return position; }
    RIter redoIter() const { return std::make_reverse_iterator(position); }
    
    Iter undoEnd() const { return history.end(); }
    RIter redoEnd() const { return history.rend(); }
    
    bool canUndo() const { return undoIter()!=undoEnd(); }
    bool canRedo() const { return redoIter()!=redoEnd(); }
};

inline void to_json(nlohmann::json& j, const UndoHistory& x) {
    using namespace nlohmann;
}

inline void from_json(const nlohmann::json& j, UndoHistory& x) {
    using namespace nlohmann;
}

struct RepoState {
    std::map<Git::Ref,UndoHistory> undoHistorys;
};

inline void to_json(nlohmann::json& j, const RepoState& x) {
    using namespace nlohmann;
}

inline void from_json(const nlohmann::json& j, RepoState& x) {
    using namespace nlohmann;
}

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


inline void to_json(nlohmann::json& j, const State& x) {
    using namespace nlohmann;
    json repoStates;
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
