#pragma once
#include <string>
#include <vector>
#include <filesystem>
#include "Git.h"
#include "lib/toastbox/String.h"

namespace Git {

struct FileConflict {
    enum class Side {
        Ours,
        Theirs,
    };
    
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
        
        const std::vector<std::string>& lines(Side side) const {
            switch (type) {
            case Type::Normal:      return normal.lines;
            case Type::Conflict:
                switch (side) {
                case Side::Ours:    return conflict.linesOurs;
                case Side::Theirs:  return conflict.linesTheirs;
                default:            abort();
                }
            default: abort();
            }
        }
    };
    
    std::filesystem::path path;
    std::vector<Hunk> hunks;
    
    // noFile(side): returns whether the given `side` of the conflict
    // represents a non-existent file
    bool noFile(Side side) const {
        return  hunks.size()==1                     &&
                hunks[0].type==Hunk::Type::Conflict &&
                hunks[0].lines(side).empty();
    }
    
    // content(side): returns the content for the given `side` of the conflict
    std::optional<std::string> content(Side side) const {
        if (noFile(side)) return std::nullopt;
        std::vector<std::string> lines;
        for (const Hunk& hunk : hunks) {
            const auto& l = hunk.lines(side);
            lines.insert(lines.end(), l.begin(), l.end());
        }
        return Toastbox::String::Join(lines, "\n");
    }
    
    size_t conflictCount() const {
        size_t x = 0;
        for (const Hunk& hunk : hunks) {
            if (hunk.type == Hunk::Type::Conflict) {
                x++;
            }
        }
        return x;
    }
};

struct ConflictMarkers {
    const char* start     = nullptr;
    const char* separator = nullptr;
    const char* end       = nullptr;
};

inline std::vector<FileConflict::Hunk> HunksFromConflictString(const ConflictMarkers& markers, std::string_view str) {
    const std::vector<std::string> lines = Toastbox::String::Split(str, "\n");
    
    enum class _ParseState {
        Normal,
        ConflictOurs,
        ConflictTheirs,
    };
    
    std::vector<FileConflict::Hunk> hunks;
    FileConflict::Hunk hunk;
    _ParseState parseState = _ParseState::Normal;
    for (const std::string& line : lines) {
        switch (parseState) {
        case _ParseState::Normal:
            if (String::StartsWith(markers.start, line)) {
                if (!hunk.empty()) hunks.push_back(std::move(hunk));
                parseState = _ParseState::ConflictOurs;
                hunk.type = FileConflict::Hunk::Type::Conflict;
            } else {
                hunk.normal.lines.push_back(line);
            }
            break;
        
        case _ParseState::ConflictOurs:
            if (String::StartsWith(markers.separator, line)) {
                parseState = _ParseState::ConflictTheirs;
                hunk.type = FileConflict::Hunk::Type::Conflict;
            } else {
                hunk.conflict.linesOurs.push_back(line);
            }
            break;
        
        case _ParseState::ConflictTheirs:
            if (String::StartsWith(markers.end, line)) {
                if (!hunk.empty()) hunks.push_back(std::move(hunk));
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
    
    if (!hunk.empty()) hunks.push_back(std::move(hunk));
    return hunks;
}

inline std::string ConflictStringFromHunks(const ConflictMarkers& markers, const std::vector<FileConflict::Hunk>& hunks) {
    std::vector<std::string> lines;
    for (const Git::FileConflict::Hunk& hunk : hunks) {
        switch (hunk.type) {
        case FileConflict::Hunk::Type::Normal:
            lines.insert(lines.end(), hunk.normal.lines.begin(), hunk.normal.lines.end());
            break;
        
        case FileConflict::Hunk::Type::Conflict:
            lines.push_back(markers.start);
            lines.insert(lines.end(), hunk.conflict.linesOurs.begin(), hunk.conflict.linesOurs.end());
            lines.push_back(markers.separator);
            lines.insert(lines.end(), hunk.conflict.linesTheirs.begin(), hunk.conflict.linesTheirs.end());
            lines.push_back(markers.end);
            break;
        
        default:
            abort();
        }
    }
    return Toastbox::String::Join(lines, "\n");
}

inline bool ConflictStringContainsConflictMarkers(const ConflictMarkers& markers, std::string_view str) {
    const std::vector<std::string> lines = Toastbox::String::Split(str, "\n");
    for (const std::string& line : lines) {
        if (String::StartsWith(markers.start, line)     ||
            String::StartsWith(markers.separator, line) ||
            String::StartsWith(markers.end, line))
            return true;
    }
    return false;
}

inline std::vector<FileConflict> ConflictsGet(const Repo& repo, const Index& index) {
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
        FileConflict fc = {
            .path = path,
        };
        
        if (conflict.ours && conflict.theirs) {
            const Git::MergeFileResult mergeResult = repo.merge(conflict.ancestor, conflict.ours, conflict.theirs);
            const std::string content(mergeResult->ptr, mergeResult->len);
            fc.hunks = HunksFromConflictString({
                .start      = Repo::MergeMarkerStart,
                .separator  = Repo::MergeMarkerSeparator,
                .end        = Repo::MergeMarkerEnd,
            }, content);
        
        } else if ((conflict.ours && !conflict.theirs) || (!conflict.ours && conflict.theirs)) {
            std::vector<std::string> linesOurs;
            std::vector<std::string> linesTheirs;
            
            if (conflict.ours) {
                Blob blob = repo.blobLookup(conflict.ours->id);
                std::string content((const char*)blob.data(), blob.size());
                linesOurs = Toastbox::String::Split(content, "\n");
            }
            
            if (conflict.theirs) {
                Blob blob = repo.blobLookup(conflict.theirs->id);
                std::string content((const char*)blob.data(), blob.size());
                linesTheirs = Toastbox::String::Split(content, "\n");
            }
            
            fc.hunks.push_back({
                .type = FileConflict::Hunk::Type::Conflict,
                .conflict = {
                    .linesOurs = linesOurs,
                    .linesTheirs = linesTheirs,
                },
            });
        
        } else {
            // Is this possible? Ie, conflict.ours==conflict.theirs==null but we still have a conflict?
            abort();
        }
        
        fileConflicts.push_back(std::move(fc));
    }
    
    return fileConflicts;
}

// ConflictResolve(): resolve a conflict for a particular file, using the supplied file content `content`.
// content == nullopt means that the file shouldn't exist.
inline void ConflictResolve(const Repo& repo, const Index& index, const FileConflict& conflict,
    const std::optional<std::string>& content) {
    
    const char* path = conflict.path.c_str();
    if (content) {
        const Blob blob = repo.blobCreate(content->data(), content->size());
        const git_index_entry* entryPrev = index.find(conflict.path, GIT_INDEX_STAGE_THEIRS);
        if (!entryPrev) entryPrev = index.find(conflict.path, GIT_INDEX_STAGE_OURS);
        assert(entryPrev);
        
        const git_index_entry entry = {
            .mode = entryPrev->mode,
            .file_size = (uint32_t)content->size(),
            .id = blob.id(),
            .path = path,
        };
        
        index.add(entry);
    
    } else {
        index.remove(path);
    }
    
    index.conflictClear(path);
    
//    ir = git_index_write(*index);
//    if (ir) throw Error(ir, "git_index_write failed");
    
//    int ir = git_index_add_from_buffer(*index, conflict.entry, content.data(), content.size());
//    if (ir) throw Error(ir, "git_index_add_from_buffer failed");
}

} // namespace Git
