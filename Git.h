#pragma once
#include "RefCounted.h"
#include "RuntimeError.h"
#include "lib/libgit2/include/git2.h"

namespace Git {

using Repo = RefCounted<git_repository*, git_repository_free>;
using RevWalk = RefCounted<git_revwalk*, git_revwalk_free>;
using Commit = RefCounted<git_commit*, git_commit_free>;
using Object = RefCounted<git_object*, git_object_free>;
using Tree = RefCounted<git_tree*, git_tree_free>;
using Index = RefCounted<git_index*, git_index_free>;
using Reference = RefCounted<git_reference*, git_reference_free>;
using Buf = RefCounted<git_buf, git_buf_dispose>;
using Branch = Reference;

inline std::string Str(const git_oid& oid) {
    char str[16];
    sprintf(str, "%02x%02x%02x%02x", oid.id[0], oid.id[1], oid.id[2], oid.id[3]);
    str[7] = 0;
    return str;
}

inline std::string Str(git_time_t t) {
    const std::time_t tmp = t;
    std::tm tm;
    localtime_r(&tmp, &tm);
    
    std::stringstream ss;
    ss << std::put_time(&tm, "%a %b %d %R");
    return ss.str();
}

inline Repo RepoOpen(std::string_view path) {
    git_repository* x = nullptr;
    int ir = git_repository_open(&x, path.data());
    if (ir) throw RuntimeError("git_repository_open failed: %s", git_error_last()->message);
    return x;
}

inline RevWalk RevWalkCreate(Repo repo) {
    git_revwalk* x = nullptr;
    int ir = git_revwalk_new(&x, *repo);
    if (ir) throw RuntimeError("git_revwalk_new failed: %s", git_error_last()->message);
    return x;
}

inline Commit CommitLookup(Repo repo, const git_oid& commitId) {
    git_commit* x = nullptr;
    int ir = git_commit_lookup(&x, *repo, &commitId);
    if (ir) throw RuntimeError("git_commit_lookup failed: %s", git_error_last()->message);
    return x;
}

inline Commit CommitLookup(Repo repo, std::string_view commitIdStr) {
    git_oid commitId;
    int ir = git_oid_fromstr(&commitId, commitIdStr.data());
    if (ir) throw RuntimeError("git_oid_fromstr failed: %s", git_error_last()->message);
    return CommitLookup(repo, commitId);
}

inline Branch BranchLookup(Repo repo, std::string_view name) {
    git_reference* x;
    int ir = git_branch_lookup(&x, *repo, name.data(), GIT_BRANCH_LOCAL);
    if (ir) throw RuntimeError("git_branch_lookup failed: %s", git_error_last()->message);
    return x;
}

inline Branch BranchCreate(Repo repo, std::string_view name, Commit commit, bool force=false) {
    git_reference* x = nullptr;
    int ir = git_branch_create(&x, *repo, name.data(), *commit, force);
    if (ir) throw RuntimeError("git_branch_create failed: %s", git_error_last()->message);
    return x;
}

inline const char* BranchNameGet(Branch branch) {
    const char* x = nullptr;
    int ir = git_branch_name(&x, *branch);
    if (ir) throw RuntimeError("git_branch_name failed: %s", git_error_last()->message);
    return x;
}

inline Branch BranchUpstreamGet(Branch branch) {
    git_reference* x;
    int ir = git_branch_upstream(&x, *branch);
    if (ir) throw RuntimeError("git_branch_upstream failed: %s", git_error_last()->message);
    return x;
}

inline void BranchUpstreamSet(Branch branch, Branch upstream) {
//    git_buf upstreamName;
//    int ir = git_branch_upstream_name(&upstreamName, *repo, BranchName(branch));
//    if (ir) throw RuntimeError("git_branch_upstream_name failed: %s", git_error_last()->message);
    
    int ir = git_branch_set_upstream(*branch, BranchNameGet(upstream));
    if (ir) throw RuntimeError("git_branch_upstream_name failed: %s", git_error_last()->message);
    
//    GIT_EXTERN(int) git_branch_name(
//    const char **out,
//    const git_reference *ref);
//    
//    
//    git_branch_name(<#const char **out#>, <#const git_reference *ref#>)
//    
//    GIT_EXTERN(int) git_branch_set_upstream(
//        git_reference *branch,
//        const char *branch_name);
//        
//    GIT_EXTERN(int) git_branch_upstream_name(
//        git_buf *out,
//        git_repository *repo,
//        const char *refname);
//    
//    
//    git_reference* x;
//    int ir = git_branch_set_upstream(<#git_reference *branch#>, <#const char *branch_name#>)
//    if (ir) throw RuntimeError("git_branch_upstream failed: %s", git_error_last()->message);
//    return x;
}

inline std::string CurrentBranchName(Repo repo) {
    Reference head;
    {
        git_reference* x = nullptr;
        int ir = git_repository_head(&x, *repo);
        if (ir) return {};
        head = x;
    }
    return git_reference_shorthand(*head);
}

inline Tree TreeGet(Commit x) {
    git_tree* t = nullptr;
    int ir = git_commit_tree(&t, *x);
    if (ir) throw RuntimeError("git_commit_tree failed: %s", git_error_last()->message);
    assert(!ir);
    return t;
}

inline Commit ParentGet(Commit x, unsigned int n=0) {
    git_commit* c = nullptr;
    int ir = git_commit_parent(&c, *x, n);
    if (ir) throw RuntimeError("git_commit_tree failed: %s", git_error_last()->message);
    return c;
}

inline Index TreesMerge(Repo repo, Tree ancestorTree, Tree dstTree, Tree srcTree) {
    git_merge_options mergeOpts = GIT_MERGE_OPTIONS_INIT;
    git_index* i = nullptr;
    int ir = git_merge_trees(&i, *repo, *ancestorTree, *dstTree, *srcTree, &mergeOpts);
    if (ir) throw RuntimeError("git_merge_trees failed: %s", git_error_last()->message);
    return i;
}

inline Tree IndexWrite(Repo repo, Index index) {
    git_oid treeId;
    int ir = git_index_write_tree_to(&treeId, *index, *repo);
    if (ir) throw RuntimeError("git_index_write_tree_to failed: %s", git_error_last()->message);
    
    git_tree* newTree = nullptr;
    ir = git_tree_lookup(&newTree, *repo, &treeId);
    if (ir) throw RuntimeError("git_tree_lookup failed: %s", git_error_last()->message);
    return newTree;
}

// Creates a new child commit of `parent` with the tree `tree`, using the metadata from `metadata`
inline Commit CommitAttach(Repo repo, Commit parent, Tree tree, Commit metadata) {
    git_oid commitId;
    const git_commit* parents[] = {*parent};
    int ir = git_commit_create(
        &commitId,
        *repo,
        nullptr,
        git_commit_author(*metadata),
        git_commit_committer(*metadata),
        git_commit_message_encoding(*metadata),
        git_commit_message(*metadata),
        *tree,
        std::size(parents),
        parents
    );
    if (ir) throw RuntimeError("git_commit_create failed: %s", git_error_last()->message);
    
    return CommitLookup(repo, commitId);
}

inline Commit CherryPick(Repo repo, Commit dst, Commit src) {
    const git_oid* dstId = git_commit_id(*dst);
    const git_oid* srcId = git_commit_id(*src);
//    const git_oid* dstTargetId = git_reference_target(basket);
//    if (!dstTargetId) throw RuntimeError("git_reference_target returned null failed: %s", git_error_last()->message);
    
    Tree srcTree = TreeGet(src);
    Tree dstTree = TreeGet(dst);
    
    git_oid baseId;
    int ir = git_merge_base(&baseId, *repo, srcId, dstId);
    if (ir) throw RuntimeError("git_merge_base failed: %s", git_error_last()->message);
    
    Tree ancestorTree = TreeGet(ParentGet(src));
    Index mergedTreesIndex = TreesMerge(repo, ancestorTree, dstTree, srcTree);
    Tree newTree = IndexWrite(repo, mergedTreesIndex);
    Commit newCommit = CommitAttach(repo, dst, newTree, src);
//    printf("New commit: %s\n", git_oid_tostr_s(git_commit_id(*newCommit)));
    return newCommit;
}

} // namespace Git
