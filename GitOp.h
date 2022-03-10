#pragma once
#include "Git.h"

namespace Git {

struct Op {
    enum class Type {
        None,
        MoveCommits,
        CopyCommits,
        DeleteCommits,
        CombineCommits,
        EditCommitMessage,
    };
    
    Type type = Type::None;
    
    Repo repo;
    
    struct {
        Rev rev;
        std::set<Commit> commits;
    } src;
    
    struct {
        Rev rev;
        Commit position;
    } dst;
};

struct OpResult {
    Rev src;
    Rev dst;
};

// _Sorted: sorts a set of commits according to the order that they appear via `c`
inline std::vector<Commit> _Sorted(Commit c, const std::set<Commit>& s) {
    std::vector<Commit> r;
    std::set<Commit> tmp = s;
    while (r.size() != s.size()) {
        if (tmp.erase(c)) r.push_back(c);
        c = c.parent();
    }
    
    // Confirm that we found all the commits in `s`
    if (r.size() != s.size()) {
        throw RuntimeError("failed to sort commits");
    }
    
    std::reverse(r.begin(), r.end());
    return r;
}

inline Commit _AddRemoveCommits(
    Repo repo,
    Commit target,
    Commit addPosition,
    const std::vector<Commit>& add,
    const std::set<Commit>& remove
) {
//    // The addPosition can't be in the set of commits to remove
//    assert(remove.find(addPosition) == remove.end());
    
    std::deque<Commit> combined;
    Commit cherry;
    
    // Construct `combined` and find `cherry`
    // `cherry` is the earliest commit from `target` which remains intact
    // `combined` is the ordered set of commits that need to be applied on top of `cherry`
    {
        const bool adding = !add.empty();
        std::set<Commit> r = remove;
        Commit c = target;
        bool foundAddPoint = false;
        for (;;) {
            if (!c) throw RuntimeError("ran out of commits");
            
            if (adding && c==addPosition) {
                assert(!foundAddPoint);
                combined.insert(combined.begin(), add.begin(), add.end());
                foundAddPoint = true;
            }
            
            if (r.empty() && (!adding || foundAddPoint)) break;
            if (!r.erase(c)) combined.push_front(c);
            c = c.parent();
        }
        
        cherry = c;
    }
    
    // Apply `combined`
    for (Commit commit : combined) {
        cherry = repo.cherryPick(cherry, commit);
    }
    
    return cherry;
}

inline OpResult _Exec_MoveCommits(const Op& op) {
    // When moving commits, the source and destination must be references (branches
    // or tags) since we're modifying both
    if (!op.src.rev.ref) throw RuntimeError("source must be a reference (branch or tag)");
    if (!op.dst.rev.ref) throw RuntimeError("destination must be a reference (branch or tag)");
    
    OpResult r;
    
    // Move commits within the same ref (branch/tag)
    if (op.src.rev.ref == op.dst.rev.ref) {
        // References are the same, so the commits must be the same too
        assert(op.src.rev.commit == op.dst.rev.commit);
        
        // Add and remove commits
        r.dst.commit = _AddRemoveCommits(op.repo, op.dst.rev.commit,
            op.dst.position, _Sorted(op.src.rev.commit, op.src.commits), op.src.commits);
    
    // Move commits between different refs (branches/tags)
    } else {
        // Remove commits from `op.src`
        r.src.commit = _AddRemoveCommits(op.repo, op.src.rev.commit, nullptr, {}, op.src.commits);
        
        // Add commits to `op.dst`
        r.dst.commit = _AddRemoveCommits(op.repo, op.dst.rev.commit,
            op.dst.position, _Sorted(op.src.rev.commit, op.src.commits), {});
    }
    
    // Update source/destination refs (branches/tags)
    {
        if (r.src.commit) {
            if (Branch branch = Branch::FromRef(*op.src.rev.ref)) {
                r.src.ref = op.repo.replaceBranch(branch, r.src.commit);
            
            } else if (false) {
                #warning TODO: handle tags
            
            } else {
                throw RuntimeError("unknown source ref type");
            }
        }
        
        if (r.dst.commit) {
            if (Branch branch = Branch::FromRef(*op.dst.rev.ref)) {
                r.dst.ref = op.repo.replaceBranch(branch, r.dst.commit);
            
            } else if (false) {
                #warning TODO: handle tags
            
            } else {
                throw RuntimeError("unknown source ref type");
            }
        }
    }
    
    return r;
}

inline OpResult _Exec_CopyCommits(const Op& op) {
    return {};
}

inline OpResult _Exec_DeleteCommits(const Op& op) {
    return {};
}

inline OpResult _Exec_CombineCommits(const Op& op) {
    return {};
}

inline OpResult _Exec_EditCommitMessage(const Op& op) {
    return {};
}

inline OpResult Exec(const Op& op) {
    // We have to detach the head, otherwise we'll get an error if we try
    // to replace the current branch
    std::string headPrev = op.repo.head().fullName();
    op.repo.detachHead();
    
    OpResult r;
    std::exception_ptr err;
    try {
        switch (op.type) {
        case Op::Type::None:                r = {}; break;
        case Op::Type::MoveCommits:         r = _Exec_MoveCommits(op); break;
        case Op::Type::CopyCommits:         r = _Exec_CopyCommits(op); break;
        case Op::Type::DeleteCommits:       r = _Exec_DeleteCommits(op); break;
        case Op::Type::CombineCommits:      r = _Exec_CombineCommits(op); break;
        case Op::Type::EditCommitMessage:   r = _Exec_EditCommitMessage(op); break;
        }
    } catch (const std::exception& e) {
        err = std::current_exception();
    }
    
    // Restore previous head
    if (!headPrev.empty()) {
        op.repo.checkout(headPrev);
//        const git_checkout_options opts = GIT_CHECKOUT_OPTIONS_INIT;
//        git_checkout_tree(repo, treeish, &opts);
//        op.repo.setHead(headPrev);
    }
    
    if (err) std::rethrow_exception(err);
    return r;
}

} // namespace Op
