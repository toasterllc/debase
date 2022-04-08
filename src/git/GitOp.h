#pragma once
#include <fstream>
#include "Git.h"
#include "lib/toastbox/Defer.h"

namespace Git {

struct Op {
    enum class Type {
        None,
        Move,
        Copy,
        Delete,
        Combine,
        Edit,
    };
    
    Type type = Type::None;
    
    struct {
        Rev rev;
        std::set<Commit> commits; // Commits to be operated on
    } src;
    
    struct {
        Rev rev;
        Commit position; // Position in destination after which the commits will be added
    } dst;
};

struct OpResult {
    struct Res {
        Rev rev;
        std::set<Commit> selection;
        std::set<Commit> selectionPrev;
    };
    
    Res src;
    Res dst;
};

// _Sorted: sorts a set of commits according to the order that they appear via `c`
inline std::vector<Commit> _Sorted(Commit head, const std::set<Commit>& commits) {
    std::vector<Commit> sorted;
    std::set<Commit> rem = commits;
    while (!rem.empty()) {
        if (rem.erase(head)) sorted.push_back(head);
        head = head.parent();
    }
    
    std::reverse(sorted.begin(), sorted.end());
    return sorted;
}

inline Commit _FindEarliestCommit(Commit head, const std::set<Commit>& commits) {
    assert(!commits.empty());
    std::set<Commit> rem = commits;
    while (rem.size() > 1) {
        rem.erase(head);
        head = head.parent();
    }
    return *rem.begin();
}

struct _AddRemoveResult {
    Commit commit;
    std::set<Commit> added;
};

inline _AddRemoveResult _AddRemoveCommits(
    Repo repo,
    Commit dst,
    const std::set<Commit>& add,
    Commit addSrc, // Source of `add` commits (to derive their order)
    Commit addPosition, // In `dst`
    const std::set<Commit>& remove
) {
    // CommitAdded: wraps a Commit with an additional `added` flag, which tracks whether
    // this is one of the added commits, so that we can put the commit in our returned
    // _AddRemoveResult.added set
    struct CommitAdded {
        CommitAdded(Commit c) : commit(c) {}
        Commit commit;
        bool added = false;
    };
    
    assert(dst);
    
    std::vector<Commit> addv = _Sorted(addSrc, add);
    
    // Construct `combined` and find `head`
    // `head` is the earliest commit from `dst` that we'll apply the commits on top of.
    // `combined` is the ordered set of commits that need to be applied on top of `head`.
    std::deque<CommitAdded> combined;
    Commit head;
    {
        const bool adding = !addv.empty();
        std::set<Commit> r = remove;
        Commit c = dst;
        bool foundAddPoint = false;
        for (;;) {
            if (adding && c==addPosition) {
                assert(!foundAddPoint);
                combined.insert(combined.begin(), addv.begin(), addv.end());
                for (size_t i=0; i<addv.size(); i++) combined[i].added = true;
                foundAddPoint = true;
            }
            
            if (r.empty() && (!adding || foundAddPoint)) break;
            // The location of this c!=null assertion is important:
            // We explicitly allow adding commits before the root commit, and also removing
            // the root commit itself, which requires us to allow c==null for a single
            // iteration of this loop.
            // Therefore this check needs to occur after our break (above). So if we get to
            // this point and we didn't break, but c==null, then we exhausted our one
            // c==null iteration and have a problem.
            assert(c);
            if (!r.erase(c)) combined.push_front(c);
            c = c.parent(); // TODO:MERGE
        }
        assert(!adding || foundAddPoint);
        assert(r.empty());
        
        head = c;
    }
    
    // Apply `combined` on top of `head`, and keep track of the added commits
    std::set<Commit> added;
    for (const CommitAdded& commit : combined) {
        // TODO:MERGE
        head = repo.commitParentSet(commit.commit, head);
        
        if (commit.added) {
            added.insert(head);
        }
    }
    
    return {
        .commit = head,
        .added = added,
    };
}

inline bool _CommitsHasGap(Git::Commit head, const std::set<Commit>& commits) {
    std::set<Commit> rem = commits;
    bool started = false;
    while (head && !rem.empty()) {
        bool found = rem.erase(head);
        if (started && !found) return true;
        started |= found;
        head = head.parent();
    }
    // If `rem` still has elements, then it's because `commits` contains
    // elements that don't exist in the `head` tree
    assert(rem.empty());
    return false;
}

inline bool _CommitsHasMerge(const std::set<Commit>& commits) {
    for (Commit commit : commits) {
        if (commit.isMerge()) return true;
    }
    return false;
}

inline bool _InsertionIsNop(Commit head, Commit position, const std::set<Commit>& commits) {
    std::set<Commit> positions = commits;
    Commit tail = _FindEarliestCommit(head, positions);
    // tail.parent()==nullptr is OK and desired, because it represents insertion as the root commit
    positions.insert(tail.parent());
    return positions.find(position) != positions.end();
}

inline bool _MoveIsNop(const Op& op) {
    assert(op.src.rev.ref);
    assert(op.dst.rev.ref);
    
    // Not a nop if the source and destination are different
    if (op.src.rev.ref != op.dst.rev.ref) return false;
    // If there's a gap in the commits that are being moved, then it's
    // never a nop, because moving them would remove the gap
    if (_CommitsHasGap(op.src.rev.commit, op.src.commits)) return false;
    return _InsertionIsNop(op.src.rev.commit, op.dst.position, op.src.commits);
}

inline std::optional<OpResult> _Exec_MoveCommits(Repo repo, const Op& op) {
    // When moving commits, the source and destination must be references (branches
    // or tags) since we're modifying both
    if (!op.src.rev.ref) throw RuntimeError("source must be a reference (branch or tag)");
    if (!op.dst.rev.ref) throw RuntimeError("destination must be a reference (branch or tag)");
    
    if (_MoveIsNop(op)) return std::nullopt;
    
    // Move commits within the same ref (branch/tag)
    if (op.src.rev.ref == op.dst.rev.ref) {
        // References are the same, so the commits must be the same too
        assert(op.src.rev.commit == op.dst.rev.commit);
        
        // Add and remove commits
        _AddRemoveResult srcDstResult = _AddRemoveCommits(
            repo,               // repo:        Repo
            op.dst.rev.commit,  // dst:         Commit
            op.src.commits,     // add:         std::set<Commit>
            op.src.rev.commit,  // addSrc:      Commit
            op.dst.position,    // addPosition: Commit
            op.src.commits      // remove:      std::set<Commit>
        );
        
        // Replace the source branch/tag
        repo.revReplace(op.src.rev, srcDstResult.commit);
        // srcRev/dstRev: we're not using the result from revReplace() as the OpResult src/dst revs,
        // and instead use repo.revReload() to get the new revs. This is to handle the case where
        // we're moving commits between eg master and master~4. In this case, the revs are different
        // even though the underlying refs are the same, so we can't use the same rev for both
        // OpResult.src.rev and OpResult.dst.rev.
        Rev srcRev = repo.revReload(op.src.rev);
        Rev dstRev = repo.revReload(op.dst.rev);
        return OpResult{
            .src = {
                .rev = srcRev,
                .selection = srcDstResult.added,
                .selectionPrev = op.src.commits,
            },
            .dst = {
                .rev = dstRev,
                .selection = srcDstResult.added,
                .selectionPrev = op.src.commits,
            },
        };
    
    // Move commits between different refs (branches/tags)
    } else {
        // Remove commits from `op.src`
        _AddRemoveResult srcResult = _AddRemoveCommits(
            repo,               // repo:        Repo
            op.src.rev.commit,  // dst:         Commit
            {},                 // add:         std::set<Commit>
            nullptr,            // addSrc:      Commit
            nullptr,            // addPosition: Commit
            op.src.commits      // remove:      std::set<Commit>
        );
        
        // Add commits to `op.dst`
        _AddRemoveResult dstResult = _AddRemoveCommits(
            repo,               // repo:        Repo
            op.dst.rev.commit,  // dst:         Commit
            op.src.commits,     // add:         std::set<Commit>
            op.src.rev.commit,  // addSrc:      Commit
            op.dst.position,    // addPosition: Commit
            {}                  // remove:      std::set<Commit>
        );
        
        // Replace the source and destination branches/tags
        Rev srcRev = repo.revReplace(op.src.rev, srcResult.commit);
        Rev dstRev = repo.revReplace(op.dst.rev, dstResult.commit);
        return OpResult{
            .src = {
                .rev = srcRev,
                .selection = {},
                .selectionPrev = op.src.commits,
            },
            .dst = {
                .rev = dstRev,
                .selection = dstResult.added,
                .selectionPrev = {},
            },
        };
    }
}

inline std::optional<OpResult> _Exec_CopyCommits(Repo repo, const Op& op) {
    if (!op.dst.rev.ref) throw RuntimeError("destination must be a reference (branch or tag)");
    
    // Add commits to `op.dst`
    _AddRemoveResult dstResult = _AddRemoveCommits(
        repo,               // repo:        Repo
        op.dst.rev.commit,  // dst:         Commit
        op.src.commits,     // add:         std::set<Commit>
        op.src.rev.commit,  // addSrc:      Commit
        op.dst.position,    // addPosition: Commit
        {}                  // remove:      std::set<Commit>
    );
    
    // Replace the destination branch/tag
    Rev dstRev = repo.revReplace(op.dst.rev, dstResult.commit);
    return OpResult{
        .src = {
            .rev = op.src.rev,
        },
        .dst = {
            .rev = dstRev,
            .selection = dstResult.added,
            .selectionPrev = {},
        },
    };
}

inline std::optional<OpResult> _Exec_DeleteCommits(Repo repo, const Op& op) {
//    throw RuntimeError("source must be a reference (branch or tag)");
    if (!op.src.rev.ref) throw RuntimeError("source must be a reference (branch or tag)");
    
    // Remove commits from `op.src`
    _AddRemoveResult srcResult = _AddRemoveCommits(
        repo,               // repo:        Repo
        op.src.rev.commit,  // dst:         Commit
        {},                 // add:         std::set<Commit>
        nullptr,            // addSrc:      Commit
        nullptr,            // addPosition: Commit
        op.src.commits      // remove:      std::set<Commit>
    );
    
    if (!srcResult.commit) {
        throw RuntimeError("can't delete last commit");
    }
    
    // Replace the source branch/tag
    Rev srcRev = repo.revReplace(op.src.rev, srcResult.commit);
    return OpResult{
        .src = {
            .rev = srcRev,
            .selection = {},
            .selectionPrev = op.src.commits,
        },
    };
}

inline std::optional<OpResult> _Exec_CombineCommits(Repo repo, const Op& op) {
    if (!op.src.rev.ref) throw RuntimeError("source must be a reference (branch or tag)");
    if (op.src.commits.size() < 2) throw RuntimeError("at least 2 commits are required to combine");
    if (_CommitsHasMerge(op.src.commits)) throw RuntimeError("can't combine with merge commit");
    
    std::deque<Commit> integrate; // Commits that need to be integrated into a single commit
    std::deque<Commit> attach;    // Commits that need to be attached after the integrate step
    Commit head;
    {
        std::set<Commit> rem = op.src.commits;
        head = op.src.rev.commit;
        for (;;) {
            if (!head) throw RuntimeError("ran out of commits");
            bool erased = rem.erase(head);
            if (rem.empty()) break;
            if (erased) integrate.push_front(head);
            else        attach.push_front(head);
            head = head.parent(); // TODO:MERGE
        }
    }
    
    // Combine `head` with all the commits in `integrate`
    for (Commit commit : integrate) {
        head = repo.commitIntegrate(head, commit);
    }
    
    // Remember the final commit containing all the integrated commits
    Commit integrated = head;
    
    // Attach every commit in `attach` to `head`
    for (Commit commit : attach) {
        // TODO:MERGE
        head = repo.commitParentSet(commit, head);
    }
    
    // Replace the source branch/tag
    Rev srcRev = repo.revReplace(op.src.rev, head);
    return OpResult{
        .src = {
            .rev = srcRev,
            .selection = {integrated},
            .selectionPrev = op.src.commits,
        },
    };
}

inline std::string _EditorCommand(Repo repo) {
    const std::optional<std::string> editorCmd = repo.config().stringGet("core.editor");
    if (editorCmd) return *editorCmd;
    if (const char* x = getenv("VISUAL")) return x;
    if (const char* x = getenv("EDITOR")) return x;
    return "vi";
}

inline std::string _Trim(std::string_view str) {
    std::string x(str);
    x.erase(x.find_last_not_of(' ')+1);
    x.erase(0, x.find_first_not_of(' '));
    return x;
}

inline bool _StartsWith(std::string_view prefix, std::string_view str) {
    return !str.compare(0, prefix.size(), prefix);
}

struct _CommitAuthor {
    std::string name;
    std::string email;
    bool operator ==(const _CommitAuthor& x) const {
        return name==x.name && email==x.email;
    }
};

struct _CommitMessage {
    std::optional<_CommitAuthor> author;
    std::optional<Time> time;
    std::string message;
    
    bool operator ==(const _CommitMessage& x) const {
        return author==x.author &&
               time==x.time &&
               message==x.message;
    }
};

inline std::optional<_CommitAuthor> _CommitAuthorParse(std::string_view str) {
    size_t emailStart = str.find_first_of('<');
    if (emailStart == std::string::npos) return std::nullopt;
    size_t emailEnd = str.find_last_of('>');
    if (emailEnd == std::string::npos) return std::nullopt;
    if (emailStart >= emailEnd) return std::nullopt;
    
    std::string name = _Trim(str.substr(0, emailStart));
    if (name.empty()) return std::nullopt;
    
    std::string email = _Trim(str.substr(emailStart+1, emailEnd-emailStart-1));
    if (email.empty()) return std::nullopt;
    
    return _CommitAuthor{
        .name = name,
        .email = email,
    };
}

inline std::optional<Time> _CommitTimeParse(std::string_view str) {
    try {
        return TimeFromString(str);
    } catch (...) {
        return std::nullopt;
    }
}

constexpr const char _AuthorPrefix[] = "Author:";
constexpr const char _TimePrefix[]   = "Date:";

inline _CommitMessage _CommitMessageForCommit(Commit commit) {
    const git_signature* sig = git_commit_author(*commit);
    return _CommitMessage{
        .author  = _CommitAuthor{sig->name, sig->email},
        .time    = TimeForGitTime(sig->when),
        .message = git_commit_message(*commit),
    };
}

inline void _CommitMessageWrite(_CommitMessage msg, const std::filesystem::path& path) {
    assert(msg.author);
    assert(msg.time);
    
    std::ofstream f;
    f.exceptions(std::ofstream::failbit | std::ofstream::badbit);
    f.open(path);
    
    f << _AuthorPrefix << " " << msg.author->name << " <" << msg.author->email << ">" << '\n';
    f << _TimePrefix << "   " << StringFromTime(*msg.time) << '\n';
    f << '\n';
    f << msg.message;
}

inline _CommitMessage _CommitMessageRead(const std::filesystem::path& path) {
    // Read back the file
    std::vector<std::string> lines;
    {
        std::ifstream f;
        f.exceptions(std::ifstream::badbit);
        f.open(path);
        
        std::string line;
        do {
            std::getline(f, line);
            lines.push_back(line);
        } while (!f.eof());
    }
    
    auto iter = lines.begin();
    
    // Find author string
    std::optional<std::string> authorStr;
    for (; !authorStr && iter!=lines.end(); iter++) {
        std::string line = _Trim(*iter);
        if (_StartsWith(_AuthorPrefix, line)) {
            authorStr = _Trim(line.substr(std::size(_AuthorPrefix)-1));
        }
        else if (!line.empty()) break;
    }
    
    // Find time string
    std::optional<std::string> timeStr;
    for (; !timeStr && iter!=lines.end(); iter++) {
        std::string line = _Trim(*iter);
        if (_StartsWith(_TimePrefix, line)) {
            timeStr = _Trim(line.substr(std::size(_TimePrefix)-1));
        }
        else if (!line.empty()) break;
    }
    
    // Skip whitespace until message starts
    for (; iter!=lines.end(); iter++) {
        std::string line = _Trim(*iter);
        if (!line.empty()) break;
    }
    
    // Re-compose message after skipping author+time+whitspace
    std::string msg;
    for (; iter!=lines.end(); iter++) {
        msg += (msg.empty() ? *iter : "\n" + *iter);
    }
    
    std::optional<_CommitAuthor> author;
    std::optional<Time> time;
    
    if (authorStr) author = _CommitAuthorParse(*authorStr);
    if (timeStr) time = _CommitTimeParse(*timeStr);
    
    return _CommitMessage{
        .author = author,
        .time = time,
        .message = msg,
    };
}

struct _Argv {
    std::vector<std::string> args;
    std::vector<const char*> argv;
};

inline _Argv _CreateArgv(Repo repo, std::string_view filePath) {
    const std::string editorCmd = _EditorCommand(repo);
    std::vector<std::string> args;
    std::vector<const char*> argv;
    std::istringstream ss(editorCmd);
    std::string arg;
    while (ss >> arg) args.push_back(arg);
    args.push_back(std::string(filePath));
    // Constructing `argv` must be a separate loop from constructing `args`,
    // because modifying `args` can invalidate all its elements, including
    // the c_str's that we'd get from each element
    for (const std::string& a : args) {
        argv.push_back(a.c_str());
    }
    argv.push_back(nullptr);
    return _Argv{args, argv};
}

template <typename T_SpawnFn>
inline std::optional<OpResult> _Exec_EditCommit(Repo repo, const Op& op, T_SpawnFn spawnFn) {
    using File = RefCounted<int, close>;
    
    assert(op.src.commits.size() == 1); // Programmer error
    if (!op.src.rev.ref) throw RuntimeError("source must be a reference (branch or tag)");
    
    // Write the commit message to `tmpFilePath`
    char tmpFilePath[] = "/tmp/debase.XXXXXX";
    int ir = mkstemp(tmpFilePath);
    if (ir < 0) throw RuntimeError("mkstemp failed: %s", strerror(errno));
    File fd(ir); // Handles closing the file descriptor upon return
    Defer(unlink(tmpFilePath)); // Delete the temporary file upon return
    
    // Write the commit message to the file
    Commit origCommit = *op.src.commits.begin();
    const git_signature* origAuthor = git_commit_author(*origCommit);
    _CommitMessage origMsg = _CommitMessageForCommit(origCommit);
    _CommitMessageWrite(origMsg, tmpFilePath);
    
    // Spawn text editor
    _Argv argv = _CreateArgv(repo, tmpFilePath);
    spawnFn(argv.argv.data());
    
    // Read back the edited commit message
    _CommitMessage newMsg = _CommitMessageRead(tmpFilePath);
    
    // Nop if the message wasn't changed
    if (origMsg == newMsg) return std::nullopt;
    
    // Construct a new signature, using the original signature values if a new value didn't exist
    const char* newName = newMsg.author ? newMsg.author->name.c_str() : origAuthor->name;
    const char* newEmail = newMsg.author ? newMsg.author->email.c_str() : origAuthor->email;
    time_t newTime = newMsg.time ? newMsg.time->time : origAuthor->when.time;
    int newOffset = newMsg.time ? newMsg.time->offset : origAuthor->when.offset;
    Signature newAuthor = Signature::Create(newName, newEmail, newTime, newOffset);
    Commit newCommit = repo.commitAmend(origCommit, newAuthor, newMsg.message);
    
    // Rewrite the rev
    // Add and remove commits
    _AddRemoveResult srcResult = _AddRemoveCommits(
        repo,               // repo:        Repo
        op.src.rev.commit,  // dst:         Commit
        {newCommit},        // add:         std::set<Commit>
        newCommit,          // addSrc:      Commit
        origCommit,         // addPosition: Commit
        {origCommit}        // remove:      std::set<Commit>
    );
    
    // Replace the source branch/tag
    Rev srcRev = repo.revReplace(op.src.rev, srcResult.commit);
    return OpResult{
        .src = {
            .rev = srcRev,
            .selection = srcResult.added,
            .selectionPrev = op.src.commits,
        },
    };
}

template <typename T_SpawnFn>
inline std::optional<OpResult> Exec(Repo repo, const Op& op, T_SpawnFn spawnFn) {
    switch (op.type) {
    case Op::Type::None:    return std::nullopt;
    case Op::Type::Move:    return _Exec_MoveCommits(repo, op);
    case Op::Type::Copy:    return _Exec_CopyCommits(repo, op);
    case Op::Type::Delete:  return _Exec_DeleteCommits(repo, op);
    case Op::Type::Combine: return _Exec_CombineCommits(repo, op);
    case Op::Type::Edit:    return _Exec_EditCommit(repo, op, spawnFn);
    }
}

} // namespace Op
