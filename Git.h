#pragma once
#include "RefCounted.h"
#include "RuntimeError.h"
#include "lib/libgit2/include/git2.h"

namespace Git {

using Repo = RefCounted<git_repository*, git_repository_free>;
using RevWalk = RefCounted<git_revwalk*, git_revwalk_free>;
using Commit = RefCounted<git_commit*, git_commit_free>;
using Object = RefCounted<git_object*, git_object_free>;
using Reference = RefCounted<git_reference*, git_reference_free>;

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

inline Repo RepoOpen(const char* path) {
    git_repository* r = nullptr;
    int ir = git_repository_open(&r, path);
    if (ir) throw RuntimeError("git_repository_open failed: %s", git_error_last()->message);
    return r;
}

inline RevWalk RevWalkCreate(Repo repo) {
    git_revwalk* w = nullptr;
    int ir = git_revwalk_new(&w, *repo);
    if (ir) throw RuntimeError("git_revwalk_new failed: %s", git_error_last()->message);
    return w;
}

inline Commit CommitCreate(Repo repo, const git_oid& oid) {
    git_commit* c = nullptr;
    int ir = git_commit_lookup(&c, *repo, &oid);
    if (ir) throw RuntimeError("git_commit_lookup failed: %s", git_error_last()->message);
    return c;
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

inline Commit CherryPick(Commit dst, Commit src) {
    git_object* cherryObj = nullptr;
    ir = git_revparse_single(&cherryObj, *repo, "36cc93379d04bbee75b8236fa62c47e4320e2b73");
    assert(!ir);
    
    git_commit* cherry = nullptr;
    ir = git_object_peel((git_object**)&cherry, cherryObj, GIT_OBJECT_COMMIT);
    assert(!ir);
    git_tree* cherryTree = nullptr;
    ir = git_commit_tree(&cherryTree, cherry);
    assert(!ir);
    
    git_reference* basket = nullptr;
    ir = git_branch_lookup(&basket, *repo, "basket", GIT_BRANCH_LOCAL);
    assert(!ir);
    git_tree* basketTree = nullptr;
    ir = git_reference_peel((git_object**)&basketTree, basket, GIT_OBJECT_TREE);
    assert(!ir);
    
    const git_oid* cherryId = git_object_id(cherryObj);
    const git_oid* basketTargetId = git_reference_target(basket);
    assert(basketTargetId);
    git_oid baseId;
    ir = git_merge_base(&baseId, *repo, cherryId, basketTargetId);
    assert(!ir);
    git_commit* basketTargetCommit = nullptr;
    ir = git_commit_lookup(&basketTargetCommit, *repo, basketTargetId);
    assert(!ir);
    
    git_commit* cherryParent = nullptr;
    ir = git_commit_parent(&cherryParent, cherry, 0);
    assert(!ir);
    
    git_tree* base_tree = nullptr;
    ir = git_commit_tree(&base_tree, cherryParent);
    assert(!ir);
    
    git_merge_options mergeOpts = GIT_MERGE_OPTIONS_INIT;
    git_index* index = nullptr;
    ir = git_merge_trees(&index, *repo, base_tree, basketTree, cherryTree, &mergeOpts);
    assert(!ir);
    
    git_oid tree_id;
//    ir = git_index_write_tree(&tree_id, index); // fails: "Failed to write tree. the index file is not backed up by an existing repository"
    ir = git_index_write_tree_to(&tree_id, index, *repo);
    assert(!ir);
    
    git_tree* newTree = nullptr;
    ir = git_tree_lookup(&newTree, *repo, &tree_id);
    assert(!ir);
    
    git_oid newCommitId;
    ir = git_commit_create(
        &newCommitId,
        *repo,
        nullptr,
        git_commit_author(cherry),
        git_commit_committer(cherry),
        git_commit_message_encoding(cherry),
        git_commit_message(cherry),
        newTree,
        1,
        (const git_commit**)&basketTargetCommit
    );
    
    printf("New commit: %s\n", git_oid_tostr_s(&newCommitId));
}

} // namespace Git
