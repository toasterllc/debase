#pragma once
#include "RefCounted.h"
#include "RuntimeError.h"
#include "lib/libgit2/include/git2.h"

#warning TODO: formalize what types these APIs accept and return. can we only accept the wrapper objects? do we need to worry about implicit creation of RefCounted objects if a non-wrapper is supplied?

namespace Git {

using Id = git_oid;
using Tree = RefCounted<git_tree*, git_tree_free>;
using Index = RefCounted<git_index*, git_index_free>;
using Buf = RefCounted<git_buf, git_buf_dispose>;

struct Object : RefCounted<git_object*, git_object_free> {
    using RefCounted::RefCounted;
    const Id& id() const { return *git_object_id(*get()); }
    bool operator==(const Object& x) const { return git_oid_cmp(&id(), &x.id())==0; }
    bool operator!=(const Object& x) const { return !(*this==x); }
    bool operator<(const Object& x) const { return git_oid_cmp(&id(), &x.id())<0; }
//    operator const git_object*() const { return *get(); }
};

struct Commit : RefCounted<git_commit*, git_commit_free> {
    using RefCounted::RefCounted;
    const Id& id() const { return *git_commit_id(*get()); }
    bool operator==(const Commit& x) const { return git_oid_cmp(&id(), &x.id())==0; }
    bool operator!=(const Commit& x) const { return !(*this==x); }
    bool operator<(const Commit& x) const { return git_oid_cmp(&id(), &x.id())<0; }
//    operator git_commit*() { return *get(); }
//    operator const git_commit*() const { return *get(); }
    
    static Commit Lookup(git_repository* repo, const Id& commitId) {
        git_commit* x = nullptr;
        int ir = git_commit_lookup(&x, repo, &commitId);
        if (ir) throw RuntimeError("git_commit_lookup failed: %s", git_error_last()->message);
        return x;
    }
    
    static Commit Lookup(git_repository* repo, std::string_view commitIdStr) {
        Id commitId;
        int ir = git_oid_fromstr(&commitId, commitIdStr.data());
        if (ir) throw RuntimeError("git_oid_fromstr failed: %s", git_error_last()->message);
        return Lookup(repo, commitId);
    }
    
    static Commit FromObject(Object obj) {
        git_commit* x = nullptr;
        int ir = git_object_peel((git_object**)&x, *obj, GIT_OBJECT_COMMIT);
        if (ir) throw RuntimeError("git_object_peel failed: %s", git_error_last()->message);
        return x;
    }
    
    Tree tree() const {
        git_tree* x = nullptr;
        int ir = git_commit_tree(&x, *get());
        if (ir) throw RuntimeError("git_commit_tree failed: %s", git_error_last()->message);
        return x;
    }
    
    Commit parent(unsigned int n=0) const {
        git_commit* x = nullptr;
        int ir = git_commit_parent(&x, *get(), n);
        if (ir) throw RuntimeError("git_commit_tree failed: %s", git_error_last()->message);
        return x;
    }
};

struct Ref : RefCounted<git_reference*, git_reference_free> {
    using RefCounted::RefCounted;
    bool operator==(const Ref& x) const { return git_reference_cmp(*get(), *x)==0; }
    bool operator!=(const Ref& x) const { return !(*this==x); }
    bool operator<(const Ref& x) const { return git_reference_cmp(*get(), *x)<0; }
    
    static Ref Lookup(git_repository* repo, std::string_view name) {
        git_reference* x = nullptr;
        int ir = git_reference_dwim(&x, repo, name.data());
        if (ir) throw RuntimeError("git_reference_dwim failed: %s", git_error_last()->message);
        return x;
    }
    
    static Ref LookupFullName(git_repository* repo, std::string_view name) {
        git_reference* x = nullptr;
        int ir = git_reference_lookup(&x, repo, name.data());
        if (ir) throw RuntimeError("git_reference_dwim failed: %s", git_error_last()->message);
        return x;
    }
    
//    static Ref Lookup(git_repository* repo, const Id& id) {
//        git_reference* x = nullptr;
//        int ir = git_reference_dwim(&x, repo, git_oid_tostr_s(&id));
//        if (ir) throw RuntimeError("git_branch_lookup failed: %s", git_error_last()->message);
//        return x;
//    }
    
    const char* name() const {
        return git_reference_shorthand(*get());
    }
    
    const char* fullName() const {
        return git_reference_name(*get());
    }
    
    bool isBranch() const {
        return git_reference_is_branch(*get());
    }
    
    bool isTag() const {
        return git_reference_is_tag(*get());
    }
    
//    bool isHead() const {
//        int ir = git_branch_is_head(*get());
//        if (ir < 0) throw RuntimeError("git_branch_is_head failed: %s", git_error_last()->message);
//        return ir==1;
//    }
    
//    bool isSymbolic() const {
//        return git_reference_type(*this)==GIT_REFERENCE_SYMBOLIC;
//    }
//    
//    bool isDirect() const {
//        return git_reference_type(*this)==GIT_REFERENCE_DIRECT;
//    }
    
    Commit commit() const {
        git_commit* x = nullptr;
        int ir = git_reference_peel((git_object**)&x, *get(), GIT_OBJECT_COMMIT);
        if (ir) throw RuntimeError("git_reference_peel failed: %s", git_error_last()->message);
        return x;
    }
    
    Tree tree() const {
        git_tree* x = nullptr;
        int ir = git_reference_peel((git_object**)&x, *get(), GIT_OBJECT_TREE);
        if (ir) throw RuntimeError("git_reference_peel failed: %s", git_error_last()->message);
        return x;
    }
};

struct Branch : Ref {
    using Ref::Ref;
    
    static Branch FromRef(Ref ref) {
        if (!ref.isBranch()) return nullptr;
        git_reference* x = nullptr;
        int ir = git_reference_dup(&x, *ref);
        if (ir) throw RuntimeError("git_reference_dup failed: %s", git_error_last()->message);
        return x;
    }
    
    static Branch Lookup(git_repository* repo, std::string_view name) {
        git_reference* x = nullptr;
        int ir = git_branch_lookup(&x, repo, name.data(), GIT_BRANCH_LOCAL);
        if (ir) throw RuntimeError("git_branch_lookup failed: %s", git_error_last()->message);
        return x;
    }
    
    static Branch Create(git_repository* repo, std::string_view name, Commit commit, bool force=false) {
        git_reference* x = nullptr;
        int ir = git_branch_create(&x, repo, name.data(), *commit, force);
        if (ir) throw RuntimeError("git_branch_create failed: %s", git_error_last()->message);
        return x;
    }
    
    const char* name() const {
        const char* x = nullptr;
        int ir = git_branch_name(&x, *get());
        if (ir) throw RuntimeError("git_branch_name failed: %s", git_error_last()->message);
        return x;
    }
    
    Branch upstream() const {
        git_reference* x = nullptr;
        int ir = git_branch_upstream(&x, *get());
        switch (ir) {
        case 0:             return x;
        case GIT_ENOTFOUND: return nullptr;
        default:            throw RuntimeError("git_branch_upstream failed: %s", git_error_last()->message);
        }
    }
    
    void setUpstream(Branch upstream) {
        int ir = git_branch_set_upstream(*get(), git_reference_shorthand(*upstream));
        if (ir) throw RuntimeError("git_branch_upstream_name failed: %s", git_error_last()->message);
    }
};

struct Repo : RefCounted<git_repository*, git_repository_free> {
    using RefCounted::RefCounted;
    
    static Repo Open(std::string_view path) {
        git_repository* x = nullptr;
        int ir = git_repository_open(&x, path.data());
        if (ir) throw RuntimeError("git_repository_open failed: %s", git_error_last()->message);
        return x;
    }
    
    Ref head() const {
        git_reference* x = nullptr;
        int ir = git_repository_head(&x, *get());
        if (ir) throw RuntimeError("git_repository_head failed: %s", git_error_last()->message);
        return x;
    }
    
//    void setHead(std::string_view head) const {
//        git_repository_set_head(*get(), head.data());
//    }
    
    void detachHead() const {
        int ir = git_repository_detach_head(*get());
        if (ir) throw RuntimeError("git_repository_detach_head failed: %s", git_error_last()->message);
    }
    
    void checkout(std::string_view name) const {
        Ref ref = Ref::LookupFullName(*get(), name);
        const git_checkout_options opts = GIT_CHECKOUT_OPTIONS_INIT;
        int ir = git_checkout_tree(*get(), (git_object*)*ref.tree(), &opts);
        if (ir) throw RuntimeError("git_checkout_tree failed: %s", git_error_last()->message);
        ir = git_repository_set_head(*get(), name.data());
        if (ir) throw RuntimeError("git_repository_set_head failed: %s", git_error_last()->message);
    }
    
    Index mergeTrees(Tree ancestorTree, Tree dstTree, Tree srcTree) const {
        git_merge_options mergeOpts = GIT_MERGE_OPTIONS_INIT;
        git_index* x = nullptr;
        int ir = git_merge_trees(&x, *get(), *ancestorTree, *dstTree, *srcTree, &mergeOpts);
        if (ir) throw RuntimeError("git_merge_trees failed: %s", git_error_last()->message);
        return x;
    }
    
    Tree writeIndex(Index index) const {
        git_oid treeId;
        int ir = git_index_write_tree_to(&treeId, *index, *get());
        if (ir) throw RuntimeError("git_index_write_tree_to failed: %s", git_error_last()->message);
        
        git_tree* x = nullptr;
        ir = git_tree_lookup(&x, *get(), &treeId);
        if (ir) throw RuntimeError("git_tree_lookup failed: %s", git_error_last()->message);
        return x;
    }
    
    // Creates a new child commit of `parent` with the tree `tree`, using the metadata from `metadata`
    Commit attachCommit(Commit parent, Tree tree, Commit metadata) const {
        git_oid commitId;
        const git_commit* parents[] = {*parent};
        int ir = git_commit_create(
            &commitId,
            *get(),
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
        return Commit::Lookup(*get(), commitId);
    }

    Commit cherryPick(Commit dst, Commit src) const {
        const git_oid* dstId = git_commit_id(*dst);
        const git_oid* srcId = git_commit_id(*src);
    //    const git_oid* dstTargetId = git_reference_target(basket);
    //    if (!dstTargetId) throw RuntimeError("git_reference_target returned null failed: %s", git_error_last()->message);
        
        Tree srcTree = src.tree();
        Tree dstTree = dst.tree();
        
        git_oid baseId;
        int ir = git_merge_base(&baseId, *get(), srcId, dstId);
        if (ir) throw RuntimeError("git_merge_base failed: %s", git_error_last()->message);
        
        Tree ancestorTree = src.parent().tree();
        Index mergedTreesIndex = mergeTrees(ancestorTree, dstTree, srcTree);
        Tree newTree = writeIndex(mergedTreesIndex);
        Commit newCommit = attachCommit(dst, newTree, src);
    //    printf("New commit: %s\n", git_oid_tostr_s(git_commit_id(*newCommit)));
        return newCommit;
    }
    
    Ref replaceRef(Ref ref, Commit commit) const {
        if (Branch branch = Branch::FromRef(ref)) {
            return replaceBranch(branch, commit);
        
        } else if (false) {
            #warning TODO: handle tags
        
        } else {
            throw RuntimeError("unknown ref type");
        }
    }
    
    Branch replaceBranch(Branch branch, Commit commit) const {
        Branch upstream = branch.upstream();
        Branch newBranch = Branch::Create(*get(), branch.name(), commit, true);
        if (upstream) newBranch.setUpstream(upstream);
        return newBranch;
    }
    
//    std::string currentBranchName() {
//        Ref head;
//        {
//            git_reference* x = nullptr;
//            int ir = git_repository_head(&x, *this);
//            if (ir) return {};
//            head = x;
//        }
//        return git_reference_shorthand(*head);
//    }
};

//struct RevWalk : RefCounted<git_revwalk*, git_revwalk_free> {
//    using RefCounted::RefCounted;
//    
//    static RevWalk Create(git_repository* repo, std::string_view str) {
//        RevWalk walk;
//        
//        // Create a RevWalk
//        {
//            git_revwalk* x = nullptr;
//            int ir = git_revwalk_new(&x, repo);
//            if (ir) throw RuntimeError("git_revwalk_new failed: %s", git_error_last()->message);
//            walk = RevWalk(x);
//        }
//        
//        // Push the range specified by `str`
//        {
//            int ir = git_revwalk_push_range(*walk, str.data());
//            if (ir) throw RuntimeError("git_revwalk_push_range failed: %s", git_error_last()->message);
//        }
//        
//        return walk;
//    }
//    
//    Commit next() {
//        Id commitId;
//        int ir = git_revwalk_next(&commitId, *get());
//        if (ir) return nullptr;
//        return Git::Commit::Lookup(git_revwalk_repository(*get()), commitId);
//    }
//};

inline std::string Str(const git_oid& oid) {
    char str[8];
    git_oid_tostr(str, sizeof(str), &oid);
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

// A Rev holds a mandatory commit, along with an optional reference (ie a branch/tag)
// that targets that commit
class Rev {
public:
    Rev() {}
    
    Rev(Repo repo, std::string_view str) {
        Object o;
        Ref r;
        
        // Determine whether `str` is a ref or commit
        {
            git_object* po = nullptr;
            git_reference* pr = nullptr;
            int ir = git_revparse_ext(&po, &pr, *repo, str.data());
            if (ir) throw RuntimeError("git_revparse_ext failed: %s", git_error_last()->message);
            o = po;
            r = pr;
        }
        
        // If we have a ref, construct this Rev as a ref
        if (r) {
            ref = r;
            commit = ref.commit();
            assert(commit); // Verify assumption: if we have a ref, it points to a valid commit
        
        // Otherwise, construct this Rev as a commit
        } else if (o) {
            commit = Commit::FromObject(o);
        
        } else {
            throw RuntimeError("git_revparse_ext returned null");
        }
    }
    
    Rev(Commit c) : commit(c) {}
    
    Rev(Ref r) : ref(r) {
        commit = ref.commit();
    }
    
    Commit commit; // Mandatory
    Ref ref;       // Optional
};

//// A Rev holds a mandatory commit, along with an optional reference (ie a branch/tag)
//// that targets that commit
//struct Rev {
//    Commit commit; // Mandatory
//    Ref ref;       // Optional
//};

} // namespace Git
