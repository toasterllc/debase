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
    
    Branch srcBranch;
    std::set<Commit> srcCommits;
    
    Branch dstBranch;
    Commit dstCommit;
};

inline void _Exec_MoveCommits(const Op& op) {
    Commit finalCherryDst;
    Commit finalCherrySrc;
    
    // Create destination branch
    {
        // srcCommits = `op.srcCommits`, in the order that they appear in `srcBranch` (where index 0 is
        // the commit closest to the repository root commit)
        std::deque<Commit> srcCommits;
        {
            std::set<Commit> tmp = op.srcCommits;
            Commit commit = op.srcBranch.commit();
            while (srcCommits.size() != op.srcCommits.size()) {
                if (tmp.find(commit) != tmp.end()) {
                    srcCommits.push_front(commit);
                    tmp.erase(commit);
                }
                
                commit = commit.parent();
            }
            
            // Confirm that we found all the commits in `op.srcCommits` when iterating over the branch
            if (srcCommits.size() != op.srcCommits.size()) {
                throw RuntimeError("failed to sort commits");
            }
        }
        
        // dstCommits = commits to apply after `srcCommits`
        std::deque<Commit> dstCommits;
        {
            Commit commit = op.dstBranch.commit();
            while (commit != op.dstCommit) {
                dstCommits.push_front(commit);
            }
        }
        
        Commit finalCherry = op.dstCommit;
        
        // Apply `srcCommits`
        for (Commit commit : srcCommits) {
            finalCherry = CherryPick(op.repo, finalCherry, commit);
        }
        
        // Apply `dstCommits`
        for (Commit commit : dstCommits) {
            finalCherry = CherryPick(op.repo, finalCherry, commit);
        }
        
        finalCherryDst = finalCherry;
    }
    
    // Create source branch
    {
        // Collect all commits from `srcBranch` that should remain intact, until we hit
        // the parent of the earliest commit to be moved (`op.srcCommits`).
        std::deque<Commit> srcCommits;
        Commit commit = op.srcBranch.commit();
        {
            std::set<Commit> tmp = op.srcCommits;
            while (!tmp.empty()) {
                if (!tmp.erase(commit)) {
                    srcCommits.push_front(commit);
                }
                commit = commit.parent();
            }
        }
        
        Commit finalCherry = commit;
        
        // Apply `srcCommits`
        for (Commit commit : srcCommits) {
            finalCherry = CherryPick(op.repo, finalCherry, commit);
        }
        
        finalCherrySrc = finalCherry;
    }
    
    // Create our branches
    {
        Branch upstreamDst = op.dstBranch.upstream();
        Branch upstreamSrc = op.srcBranch.upstream();
        
        // Create a new branch from `finalCherry`
        Branch newBranchDst = Branch::Create(op.repo, op.dstBranch.name(), finalCherryDst, true);
        Branch newBranchSrc = Branch::Create(op.repo, op.srcBranch.name(), finalCherrySrc, true);
        
        // Copy upstream info to the new branches
        newBranchDst.upstreamSet(upstreamDst);
        newBranchSrc.upstreamSet(upstreamSrc);
    }
    
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

inline void _Exec_CopyCommits(const Op& op) {
    
}

inline void _Exec_DeleteCommits(const Op& op) {
    
}

inline void _Exec_CombineCommits(const Op& op) {
    
}

inline void _Exec_EditCommitMessage(const Op& op) {
    
}

inline void Exec(const Op& op) {
    switch (op.type) {
    case Op::Type::None:                return;
    case Op::Type::MoveCommits:         return _Exec_MoveCommits(op);
    case Op::Type::CopyCommits:         return _Exec_CopyCommits(op);
    case Op::Type::DeleteCommits:       return _Exec_DeleteCommits(op);
    case Op::Type::CombineCommits:      return _Exec_CombineCommits(op);
    case Op::Type::EditCommitMessage:   return _Exec_EditCommitMessage(op);
    }
}

} // namespace Op
