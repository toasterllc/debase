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

inline bool RevMutable(const Rev& rev) {
    assert(rev);
    // Only ref-backed revs are mutable
    if (!rev.ref) return false;
    return rev.ref.isBranch() || rev.ref.isTag();
}

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
    std::vector<Commit> addv = _Sorted(addSrc, add);
    
    // Construct `combined` and find `head`
    // `head` is the earliest commit from `dst` that we'll apply the commits on top of.
    // `combined` is the ordered set of commits that need to be applied on top of `head`.
    std::deque<Commit> combined;
    Commit head;
    {
        const bool adding = !addv.empty();
        std::set<Commit> r = remove;
        Commit c = dst;
        bool foundAddPoint = false;
        for (;;) {
            if (!c) throw RuntimeError("ran out of commits");
            
            if (adding && c==addPosition) {
                assert(!foundAddPoint);
                combined.insert(combined.begin(), addv.begin(), addv.end());
                foundAddPoint = true;
            }
            
            if (r.empty() && (!adding || foundAddPoint)) break;
            if (!r.erase(c)) combined.push_front(c);
            c = c.parent();
        }
        
        head = c;
    }
    
    // Apply `combined` on top of `head`, and keep track of the added commits
    std::set<Commit> added;
    for (Commit commit : combined) {
        head = repo.commitAttach(head, commit);
        if (add.find(commit) != add.end()) {
            added.insert(head);
        }
    }
    
    return {
        .commit = head,
        .added = added,
    };
}

inline OpResult _Exec_MoveCommits(const Op& op) {
    // When moving commits, the source and destination must be references (branches
    // or tags) since we're modifying both
    if (!op.src.rev.ref) throw RuntimeError("source must be a reference (branch or tag)");
    if (!op.dst.rev.ref) throw RuntimeError("destination must be a reference (branch or tag)");
    
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
            head = head.parent();
        }
    }
    
    // Combine `head` with all the commits in `integrate`
    for (Commit commit : integrate) {
        head = op.repo.commitIntegrate(head, commit);
    }
    
    // Remember the final commit containing all the integrated commits
    Commit integrated = head;
    
    // Attach every commit in `attach` to `head`
    for (Commit commit : integrate) {
        head = op.repo.commitAttach(head, commit);
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

inline OpResult _Exec_EditCommitMessage(const Op& op) {
    return {};
}

inline OpResult Exec(const Op& op) {
    // We have to detach the head, otherwise we'll get an error if we try
    // to replace the current branch
    std::string headPrev = op.repo.head().fullName();
    op.repo.headDetach();
    
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
    }
    
    if (err) std::rethrow_exception(err);
    return r;
}

} // namespace Op
