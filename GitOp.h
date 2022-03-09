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
    
    Reference srcRef;
    std::set<Commit> srcCommits;
    
    Reference dstRef;
    Commit dstCommit;
};

struct OpResult {
    Reference srcRef;
    Reference dstRef;
};



inline std::vector<Commit> _Sorted(Reference ref, const std::set<Commit>& s) {
    std::vector<Commit> r;
    std::set<Commit> tmp = s;
    Commit c = ref.commit();
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
    Reference ref,
    Commit addPoint,
    const std::vector<Commit>& add,
    const std::set<Commit>& remove
) {
    assert(remove.find(addPoint) == remove.end());
    
    std::deque<Commit> combined;
    Commit cherry;
    {
        const bool adding = !add.empty();
        std::set<Commit> r = remove;
        Commit c = ref.commit();
        bool foundAddPoint = false;
        for (;;) {
            if (!c) throw RuntimeError("ran out of commits");
            if (!r.erase(c)) {
                if (adding && c==addPoint) {
                    assert(!foundAddPoint);
                    combined.insert(combined.begin(), add.begin(), add.end());
                    foundAddPoint = true;
                }
                
                if (r.empty() && (!adding || foundAddPoint)) break;
                combined.push_front(c);
            }
            
            c = c.parent();
        }
        
        cherry = c;
    }
    
    // Apply `combined`
    for (Commit commit : combined) {
        cherry = CherryPick(repo, cherry, commit);
    }
    
    return cherry;
}









//inline Commit _AddRemoveCommits(
//    Repo repo,
//    Reference ref,
//    Commit addPoint,
//    const std::vector<Commit>& add,
//    const std::set<Commit>& remove
//) {
//    assert(remove.find(addPoint) == remove.end());
//    
//    std::deque<Commit> combined;
//    Commit cherry;
//    {
//        const bool adding = !add.empty();
//        std::set<Commit> r = remove;
//        Commit c = ref.commit();
//        bool foundAddPoint = false;
//        for (;;) {
//            if (!c) throw RuntimeError("ran out of commits");
//            
//            if (!r.erase(c)) {
//                if (adding && c==addPoint) {
//                    assert(!foundAddPoint);
//                    combined.insert(combined.begin(), add.begin(), add.end());
//                    foundAddPoint = true;
//                }
//                
//                combined.push_front(c);
//                
//                if (r.empty() && (!adding || foundAddPoint)) break;
//            }
//            c = c.parent();
//        }
//        
//        cherry = combined.front();
//        combined.pop_front();
//    }
//    
//    // Apply `combined`
//    for (Commit commit : combined) {
//        cherry = CherryPick(repo, cherry, commit);
//    }
//    
//    return cherry;
//}



inline Branch _ReplaceBranch(Repo repo, Branch branch, Commit commit) {
    Branch upstream = branch.upstream();
    Branch newBranch = Branch::Create(repo, branch.name(), commit, true);
    if (upstream) newBranch.upstreamSet(upstream);
    return newBranch;
}

inline OpResult _Exec_MoveCommits(const Op& op) {
    Commit srcCommit;
    Commit dstCommit;
    
    // Handle moving commits within the same branch
    if (op.srcRef == op.dstRef) {
        // Add and remove commits
        dstCommit = _AddRemoveCommits(op.repo, op.dstRef,
            op.dstCommit, _Sorted(op.srcRef, op.srcCommits), op.srcCommits);
    
    // Handle moving commits across different branches
    } else {
        // Remove commits from `op.srcRef`
        srcCommit = _AddRemoveCommits(op.repo, op.srcRef, nullptr, {}, op.srcCommits);
        
        // Add commits to `op.dstRef`
        dstCommit = _AddRemoveCommits(op.repo, op.dstRef,
            op.dstCommit, _Sorted(op.srcRef, op.srcCommits), {});
    }
    
    // Get the References for srcCommit / dstCommit
    Reference srcRef = (srcCommit ? Reference::Lookup(op.repo, srcCommit.id()) : nullptr);
    Reference dstRef = (dstCommit ? Reference::Lookup(op.repo, dstCommit.id()) : nullptr);
    
    // Create branches if the original refs (op.srcRef / op.dstRef) were branches
    {
        if (srcCommit) {
            if (Branch branch = Branch::FromReference(op.srcRef)) {
                srcRef = _ReplaceBranch(op.repo, branch, srcCommit);
            }
        }
        
        if (dstCommit) {
            if (Branch branch = Branch::FromReference(op.dstRef)) {
                dstRef = _ReplaceBranch(op.repo, branch, dstCommit);
            }
        }
    }
    
    return OpResult{
        .srcRef = srcRef,
        .dstRef = dstRef,
    };
    
    
    
//    _Sorted()
//    
//    
//    Commit finalCherryDst;
//    Commit finalCherrySrc;
//    
//    // Create destination branch
//    {
//        // srcCommits = `op.srcCommits`, in the order that they appear in `srcBranch` (where index 0 is
//        // the commit closest to the repository root commit)
//        std::deque<Commit> srcCommits;
//        {
//            std::set<Commit> tmp = op.srcCommits;
//            Commit commit = op.srcRef.commit();
//            while (srcCommits.size() != op.srcCommits.size()) {
//                if (tmp.find(commit) != tmp.end()) {
//                    srcCommits.push_front(commit);
//                    tmp.erase(commit);
//                }
//                
//                commit = commit.parent();
//            }
//            
//            // Confirm that we found all the commits in `op.srcCommits` when iterating over the branch
//            if (srcCommits.size() != op.srcCommits.size()) {
//                throw RuntimeError("failed to sort commits");
//            }
//        }
//        
//        // dstCommits = commits to apply after `srcCommits`
//        std::deque<Commit> dstCommits;
//        {
//            Commit commit = op.dstRef.commit();
//            while (commit != op.dstCommit) {
//                dstCommits.push_front(commit);
//            }
//        }
//        
//        Commit finalCherry = op.dstCommit;
//        
//        // Apply `srcCommits`
//        for (Commit commit : srcCommits) {
//            finalCherry = CherryPick(op.repo, finalCherry, commit);
//        }
//        
//        // Apply `dstCommits`
//        for (Commit commit : dstCommits) {
//            finalCherry = CherryPick(op.repo, finalCherry, commit);
//        }
//        
//        finalCherryDst = finalCherry;
//    }
//    
//    // Create source branch
//    {
//        // Collect all commits from `srcBranch` that should remain intact, until we hit
//        // the parent of the earliest commit to be moved (`op.srcCommits`).
//        std::deque<Commit> srcCommits;
//        Commit commit = op.srcRef.commit();
//        {
//            std::set<Commit> tmp = op.srcCommits;
//            while (!tmp.empty()) {
//                if (!tmp.erase(commit)) {
//                    srcCommits.push_front(commit);
//                }
//                commit = commit.parent();
//            }
//        }
//        
//        Commit finalCherry = commit;
//        
//        // Apply `srcCommits`
//        for (Commit commit : srcCommits) {
//            finalCherry = CherryPick(op.repo, finalCherry, commit);
//        }
//        
//        finalCherrySrc = finalCherry;
//    }
//    
//    // Create our branches
//    {
//        if (Branch branch = Branch::FromReference(op.dstRef)) {
//            Branch upstream = branch.upstream();
//            Branch newBranch = Branch::Create(op.repo, branch.name(), finalCherryDst, true);
//            if (upstream) newBranch.upstreamSet(upstream);
//        }
//        
//        if (Branch branch = Branch::FromReference(op.srcRef)) {
//            Branch upstream = branch.upstream();
//            Branch newBranch = Branch::Create(op.repo, branch.name(), finalCherrySrc, true);
//            if (upstream) newBranch.upstreamSet(upstream);
//        }
//    }
//    
//    return {};
    
//    // Find the earliest commit
//    Commit earliestCommit;
//    {
//        assert(!op.srcCommits.empty());
//        
//        std::set<Commit> srcCommits = op.srcCommits;
//        Commit commit = op.srcBranch.commit();
//        while (commit && srcCommits.size()!=1) {
//            srcCommits.erase(commit);
//            commit = commit.parent();
//        }
//        
//        if (srcCommits.size() != 1) {
//            throw RuntimeError("failed to find earliest commit");
//        }
//        
//        earliestCommit = *srcCommits.begin();
//    }
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
    switch (op.type) {
    case Op::Type::None:                return {};
    case Op::Type::MoveCommits:         return _Exec_MoveCommits(op);
    case Op::Type::CopyCommits:         return _Exec_CopyCommits(op);
    case Op::Type::DeleteCommits:       return _Exec_DeleteCommits(op);
    case Op::Type::CombineCommits:      return _Exec_CombineCommits(op);
    case Op::Type::EditCommitMessage:   return _Exec_EditCommitMessage(op);
    }
}

} // namespace Op
