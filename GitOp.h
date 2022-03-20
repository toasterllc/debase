#pragma once
#include <fstream>
#include "Git.h"
#include "lib/Toastbox/Defer.h"

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
    
    Repo repo;
    
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
    struct {
        Rev rev;
    } src;
    
    struct {
        Rev rev;
        std::set<Commit> commits; // The commits that were added
    } dst;
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
        
//        std::vector<Commit> parents = commit.parents();
//        // Remove the commit's original parent[0]
//        if (!parents.empty()) parents.erase(parents.begin());
//        // Set the commit's parent[0] to head, if head!=nullptr
//        if (head) parents.insert(parents.begin(), head);
//        head = repo.commitParentsSet(commit, parents);
        
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

inline OpResult _Exec_MoveCommits(const Op& op) {
    // When moving commits, the source and destination must be references (branches
    // or tags) since we're modifying both
    if (!op.src.rev.ref) throw RuntimeError("source must be a reference (branch or tag)");
    if (!op.dst.rev.ref) throw RuntimeError("destination must be a reference (branch or tag)");
    
    if (_MoveIsNop(op)) {
        return {
            .src = {
                .rev = op.src.rev,
            },
            .dst = {
                .rev = op.dst.rev,
                .commits = op.src.commits,
            },
        };
    }
    
    // Move commits within the same ref (branch/tag)
    if (op.src.rev.ref == op.dst.rev.ref) {
        // References are the same, so the commits must be the same too
        assert(op.src.rev.commit == op.dst.rev.commit);
        
        // Add and remove commits
        _AddRemoveResult srcDstResult = _AddRemoveCommits(
            op.repo,            // repo:        Repo
            op.dst.rev.commit,  // dst:         Commit
            op.src.commits,     // add:         std::set<Commit>
            op.src.rev.commit,  // addSrc:      Commit
            op.dst.position,    // addPosition: Commit
            op.src.commits      // remove:      std::set<Commit>
        );
        
        // Replace the source branch/tag
        Rev srcDstRev = op.repo.refReplace(op.src.rev.ref, srcDstResult.commit);
        return {
            .src = {
                .rev = srcDstRev,
            },
            .dst = {
                .rev = srcDstRev,
                .commits = srcDstResult.added,
            },
        };
    
    // Move commits between different refs (branches/tags)
    } else {
        // Remove commits from `op.src`
        _AddRemoveResult srcResult = _AddRemoveCommits(
            op.repo,            // repo:        Repo
            op.src.rev.commit,  // dst:         Commit
            {},                 // add:         std::set<Commit>
            nullptr,            // addSrc:      Commit
            nullptr,            // addPosition: Commit
            op.src.commits      // remove:      std::set<Commit>
        );
        
        // Add commits to `op.dst`
        _AddRemoveResult dstResult = _AddRemoveCommits(
            op.repo,            // repo:        Repo
            op.dst.rev.commit,  // dst:         Commit
            op.src.commits,     // add:         std::set<Commit>
            op.src.rev.commit,  // addSrc:      Commit
            op.dst.position,    // addPosition: Commit
            {}                  // remove:      std::set<Commit>
        );
        
        // Replace the source and destination branches/tags
        Rev srcRev = op.repo.refReplace(op.src.rev.ref, srcResult.commit);
        Rev dstRev = op.repo.refReplace(op.dst.rev.ref, dstResult.commit);
        return {
            .src = {
                .rev = srcRev,
            },
            .dst = {
                .rev = dstRev,
                .commits = dstResult.added,
            },
        };
    }
}

inline OpResult _Exec_CopyCommits(const Op& op) {
    if (!op.dst.rev.ref) throw RuntimeError("destination must be a reference (branch or tag)");
    
    // Add commits to `op.dst`
    _AddRemoveResult dstResult = _AddRemoveCommits(
        op.repo,            // repo:        Repo
        op.dst.rev.commit,  // dst:         Commit
        op.src.commits,     // add:         std::set<Commit>
        op.src.rev.commit,  // addSrc:      Commit
        op.dst.position,    // addPosition: Commit
        {}                  // remove:      std::set<Commit>
    );
    
    // Replace the destination branch/tag
    Rev dstRev = op.repo.refReplace(op.dst.rev.ref, dstResult.commit);
    return {
        .src = {
            .rev = op.src.rev,
        },
        .dst = {
            .rev = dstRev,
            .commits = dstResult.added,
        },
    };
}

inline OpResult _Exec_DeleteCommits(const Op& op) {
    if (!op.src.rev.ref) throw RuntimeError("source must be a reference (branch or tag)");
    
    // Remove commits from `op.src`
    _AddRemoveResult srcResult = _AddRemoveCommits(
        op.repo,            // repo:        Repo
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
    Rev srcRev = op.repo.refReplace(op.src.rev.ref, srcResult.commit);
    return {
        .src = {
            .rev = srcRev,
        },
        .dst = {
            .rev = op.dst.rev,
            .commits = {},
        },
    };
}

inline OpResult _Exec_CombineCommits(const Op& op) {
    if (!op.src.rev.ref) throw RuntimeError("source must be a reference (branch or tag)");
    if (op.src.commits.size() < 2) throw RuntimeError("at least 2 commits are required to combine");
    
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
        head = op.repo.commitIntegrate(head, commit);
    }
    
    // Remember the final commit containing all the integrated commits
    Commit integrated = head;
    
    // Attach every commit in `attach` to `head`
    for (Commit commit : attach) {
        // TODO:MERGE
        head = op.repo.commitParentSet(commit, head);
    }
    
    // Replace the source branch/tag
    Rev srcRev = op.repo.refReplace(op.src.rev.ref, head);
    return {
        .src = {
            .rev = srcRev,
        },
        .dst = {
            .rev = srcRev,
            .commits = {integrated},
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
    bool operator==(const _CommitAuthor& x) const {
        return name==x.name && email==x.email;
    }
};

struct _CommitMessage {
    std::optional<_CommitAuthor> author;
    std::optional<Time> time;
    std::string message;
    
    bool operator==(const _CommitMessage& x) const {
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




//inline Signature _SignatureParse(std::string_view authorEmailStr, std::string_view timeStr) {
//    size_t emailStart = authorEmailStr.find_first_of('<');
//    if (emailStart == std::string::npos) throw RuntimeError("invalid name/email");
//    size_t emailEnd = authorEmailStr.find_last_of('>');
//    if (emailEnd == std::string::npos) throw RuntimeError("invalid name/email");
//    if (emailStart >= emailEnd) throw RuntimeError("invalid name/email");
//    
//    std::string name = _Trim(authorEmailStr.substr(0, emailStart));
//    std::string email = _Trim(authorEmailStr.substr(emailStart+1, emailEnd-emailStart-1));
//    
//    int offset = 0;
//    git_time_t time = TimeFromString(timeStr, &offset);
//    
//    git_signature* sig = nullptr;
//    int ir = git_signature_new(&sig, name.c_str(), email.c_str(), time, offset);
//    if (ir) throw RuntimeError("git_signature_new failed: %s", git_error_last()->message);
//    return sig;
//}

//inline std::optional<git_time> _TimeParse(std::string_view str) {
//    return {};
//}

constexpr const char _AuthorPrefix[] = "Author:";
constexpr const char _TimePrefix[]   = "Date:";



//inline void _CommitMessageWrite(_CommitMessage commitMessage, const std::filesystem::path& path) {
//    assert(commitMessage.author);
//    assert(commitMessage.time);
//    
//    const git_signature* sig = git_commit_author(*commit);
//    std::string timeStr = StringFromTime(sig->when.time);
//    const char* msg = git_commit_message(*commit);
//    
//    commitMessage.time
//    
//    std::ofstream f;
//    f.exceptions(std::ofstream::failbit | std::ofstream::badbit);
//    f.open(path);
//    
//    f << _AuthorPrefix << " " << commitMessage.author->name << " <" << commitMessage.author->email << ">" << '\n';
//    f << _TimePrefix << "   " << timeStr << '\n';
//    f << '\n';
//    f << msg;
//}

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
    for (const std::string& arg : args) {
        argv.push_back(arg.c_str());
    }
    argv.push_back(nullptr);
    return _Argv{args, argv};
}

template <auto T_SpawnFn>
inline OpResult _Exec_EditCommit(const Op& op) {
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
    _Argv argv = _CreateArgv(op.repo, tmpFilePath);
    T_SpawnFn(argv.argv.data());
    
    // Read back the edited commit message
    _CommitMessage newMsg = _CommitMessageRead(tmpFilePath);
    
    // Nop if the message wasn't changed
    if (origMsg == newMsg) {
        return {
            .src = {
                .rev = op.src.rev,
            },
            .dst = {
                .rev = op.src.rev,
                .commits = op.src.commits,
            },
        };
    }
    
    // Construct a new signature, using the original signature values if a new value didn't exist
    const char* newName = newMsg.author ? newMsg.author->name.c_str() : origAuthor->name;
    const char* newEmail = newMsg.author ? newMsg.author->email.c_str() : origAuthor->email;
    time_t newTime = newMsg.time ? newMsg.time->time : origAuthor->when.time;
    int newOffset = newMsg.time ? newMsg.time->offset : origAuthor->when.offset;
    Signature newAuthor = Signature::Create(newName, newEmail, newTime, newOffset);
    Commit newCommit = op.repo.commitAmend(origCommit, newAuthor, newMsg.message);
    
    // Rewrite the rev
    // Add and remove commits
    _AddRemoveResult srcDstResult = _AddRemoveCommits(
        op.repo,            // repo:        Repo
        op.src.rev.commit,  // dst:         Commit
        {newCommit},        // add:         std::set<Commit>
        newCommit,          // addSrc:      Commit
        origCommit,         // addPosition: Commit
        {origCommit}        // remove:      std::set<Commit>
    );
    
    // Replace the source branch/tag
    Rev srcDstRev = op.repo.refReplace(op.src.rev.ref, srcDstResult.commit);
    return {
        .src = {
            .rev = srcDstRev,
        },
        .dst = {
            .rev = srcDstRev,
            .commits = srcDstResult.added,
        },
    };
}

template <auto T_SpawnFn>
inline OpResult Exec(const Op& op) {
    // We have to detach the head, otherwise we'll get an error if we try
    // to replace the current branch
    std::string headPrev = op.repo.head().fullName();
    op.repo.headDetach();
    
    OpResult r;
    std::exception_ptr err;
    try {
        switch (op.type) {
        case Op::Type::None:    r = {}; break;
        case Op::Type::Move:    r = _Exec_MoveCommits(op); break;
        case Op::Type::Copy:    r = _Exec_CopyCommits(op); break;
        case Op::Type::Delete:  r = _Exec_DeleteCommits(op); break;
        case Op::Type::Combine: r = _Exec_CombineCommits(op); break;
        case Op::Type::Edit:    r = _Exec_EditCommit<T_SpawnFn>(op); break;
        }
    } catch (const std::exception& e) {
        err = std::current_exception();
    }
    
    // Restore previous head
    if (!headPrev.empty()) {
        op.repo.checkout(headPrev);
    }
    
    if (err) std::rethrow_exception(err);
    return r;
}

} // namespace Op
