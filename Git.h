#pragma once
#include "RefCounted.h"
#include "RuntimeError.h"
#include "lib/libgit2/include/git2.h"

using Repo = RefCounted<git_repository*, git_repository_free>;
using RevWalk = RefCounted<git_revwalk*, git_revwalk_free>;
using Commit = RefCounted<git_commit*, git_commit_free>;

inline std::string StrFromGitOid(const git_oid& oid) {
    char str[16];
    sprintf(str, "%02x%02x%02x%02x", oid.id[0], oid.id[1], oid.id[2], oid.id[3]);
    str[7] = 0;
    return str;
}

inline std::string StrFromGitTime(git_time_t t) {
    const std::time_t tmp = t;
    std::tm tm;
    localtime_r(&tmp, &tm);
    
    std::stringstream ss;
    ss << std::put_time(&tm, "%a %b %d %R");
    return ss.str();
}

inline Repo RepoOpen() {
    git_repository* r = nullptr;
    int ir = git_repository_open(&r, ".");
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
