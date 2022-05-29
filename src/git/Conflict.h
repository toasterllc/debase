#pragma once
#include <string>
#include <vector>
#include <filesystem>
#include "lib/toastbox/String.h"

namespace Git {

struct FileConflict {
    struct Hunk {
        enum class Type {
            Normal,
            Conflict,
        };
        
        Type type = Type::Normal;
        
        struct {
            std::vector<std::string> lines;
        } normal;
        
        struct {
            std::vector<std::string> linesOurs;
            std::vector<std::string> linesTheirs;
        } conflict;
        
        bool empty() const {
            switch (type) {
            case Type::Normal:      return normal.lines.empty();
            case Type::Conflict:    return conflict.linesOurs.empty() && conflict.linesTheirs.empty();
            default: abort();
            }
        }
    };
    
    const git_index_entry* entry = nullptr;
    std::filesystem::path path;
    std::vector<Hunk> hunks;
};

std::vector<FileConflict> ConflictsGet(const Repo& repo, const Index& index) {
    using namespace Toastbox;
    using _Path = std::filesystem::path;
    
    struct _IndexConflict {
        const git_index_entry* ancestor = nullptr;
        const git_index_entry* ours = nullptr;
        const git_index_entry* theirs = nullptr;
    };
    
    std::map<_Path,_IndexConflict> conflicts;
    
    for (size_t i=0;; i++) {
        const git_index_entry*const entry = index[i];
        // End of index
        if (!entry) break;
        // We only care about conflicts
        if (!git_index_entry_is_conflict(entry)) continue;
        
        const git_index_stage_t stage = (git_index_stage_t)GIT_INDEX_ENTRY_STAGE(entry);
        _IndexConflict& conflict = conflicts[entry->path];
        
        if (stage == GIT_INDEX_STAGE_ANCESTOR) {
            assert(!conflict.ancestor);
            conflict.ancestor = entry;
        
        } else if (stage == GIT_INDEX_STAGE_OURS) {
            assert(!conflict.ours);
            conflict.ours = entry;
        
        } else if (stage == GIT_INDEX_STAGE_THEIRS) {
            assert(!conflict.theirs);
            conflict.theirs = entry;
        }
    }
    
    std::vector<FileConflict> fileConflicts;
    for (auto const& [path, conflict] : conflicts) {
        #warning TODO: we have to handle cases where conflict.ours/conflict.theirs are null
        const Git::MergeFileResult mergeResult = repo.merge(conflict.ancestor, conflict.ours, conflict.theirs);
        const std::string content(mergeResult->ptr, mergeResult->len);
        const std::vector<std::string> lines = Toastbox::String::Split(content, "\n");
        
        FileConflict fc = {
            .entry = conflict.ours,
            .path = path,
        };
        
        FileConflict::Hunk hunk;
        
        enum class _ParseState {
            Normal,
            ConflictOurs,
            ConflictTheirs,
        };
        
        _ParseState parseState = _ParseState::Normal;
        for (const std::string& line : lines) {
            switch (parseState) {
            case _ParseState::Normal:
                if (String::StartsWith(Repo::MergeMarkerOurs, line)) {
                    if (!hunk.empty()) fc.hunks.push_back(std::move(hunk));
                    parseState = _ParseState::ConflictOurs;
                    hunk.type = FileConflict::Hunk::Type::Conflict;
                } else {
                    hunk.normal.lines.push_back(line);
                }
                break;
            
            case _ParseState::ConflictOurs:
                if (String::StartsWith(Repo::MergeMarkerSeparator, line)) {
                    if (!hunk.empty()) fc.hunks.push_back(std::move(hunk));
                    parseState = _ParseState::ConflictTheirs;
                    hunk.type = FileConflict::Hunk::Type::Conflict;
                } else {
                    hunk.conflict.linesOurs.push_back(line);
                }
                break;
            
            case _ParseState::ConflictTheirs:
                if (String::StartsWith(Repo::MergeMarkerTheirs, line)) {
                    if (!hunk.empty()) fc.hunks.push_back(std::move(hunk));
                    parseState = _ParseState::Normal;
                    hunk.type = FileConflict::Hunk::Type::Normal;
                } else {
                    hunk.conflict.linesTheirs.push_back(line);
                }
                break;
            
            default:
                abort(); // Invalid state
            }
        }
        
        if (!hunk.empty()) fc.hunks.push_back(std::move(hunk));
        fileConflicts.push_back(std::move(fc));
    }
    
    return fileConflicts;
}

void ConflictResolve(const FileConflict& conflict, const Index& index, const std::string& content) {
    int ir = git_index_add_from_buffer(*index, conflict.entry, content.data(), content.size());
    if (ir) throw Error(ir, "git_index_add_from_buffer failed");
}

} // namespace Git
