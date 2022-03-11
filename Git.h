#pragma once
#include "RefCounted.h"
#include "RuntimeError.h"
#include "lib/libgit2/include/git2.h"

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
};

struct Commit : RefCounted<git_commit*, git_commit_free> {
    using RefCounted::RefCounted;
    const Id& id() const { return *git_commit_id(*get()); }
    bool operator==(const Commit& x) const { return git_oid_cmp(&id(), &x.id())==0; }
    bool operator!=(const Commit& x) const { return !(*this==x); }
    bool operator<(const Commit& x) const { return git_oid_cmp(&id(), &x.id())<0; }
    
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
    
    void printId() const {
        printf("%s\n", git_oid_tostr_s(&id()));
    }
};

struct Ref : RefCounted<git_reference*, git_reference_free> {
    using RefCounted::RefCounted;
    bool operator==(const Ref& x) const { return strcmp(fullName(), x.fullName())==0; }
    bool operator!=(const Ref& x) const { return !(*this==x); }
    bool operator<(const Ref& x) const { return strcmp(fullName(), x.fullName())<0; }
    
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

struct AnnotatedCommit : RefCounted<git_annotated_commit*, git_annotated_commit_free> {
    using RefCounted::RefCounted;
    
    static AnnotatedCommit FromCommit(Commit commit) {
        git_annotated_commit* x = nullptr;
        int ir = git_annotated_commit_lookup(&x, git_commit_owner(*commit), &commit.id());
        if (ir) throw RuntimeError("git_annotated_commit_from_ref failed: %s", git_error_last()->message);
        return x;
    }
    
    static AnnotatedCommit FromRef(Ref ref) {
        git_annotated_commit* x = nullptr;
        int ir = git_annotated_commit_from_ref(&x, git_reference_owner(*ref), *ref);
        if (ir) throw RuntimeError("git_annotated_commit_from_ref failed: %s", git_error_last()->message);
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

// A Rev holds a mandatory commit, along with an optional reference (ie a branch/tag)
// that targets that commit
class Rev {
public:
    // Invalid Rev
    Rev() {}
    
    Rev(Commit c) : commit(c) {}
    
    Rev(Ref r) : ref(r) {
        commit = ref.commit();
        assert(commit); // Verify assumption: if we have a ref, it points to a valid commit
    }
    
    AnnotatedCommit annotatedCommit() const {
        if (ref) return AnnotatedCommit::FromRef(ref);
        return AnnotatedCommit::FromCommit(commit);
    }
    
    operator bool() const { return (bool)commit; }
    
    bool operator==(const Rev& x) const {
        if ((bool)commit != (bool)x.commit) return false;
        if (commit && (commit != x.commit)) return false;
        if ((bool)ref != (bool)x.ref) return false;
        if (ref && (ref != x.ref)) return false;
        return true;
    }
    
    bool operator!=(const Rev& x) const { return !(*this==x); }
    
    bool operator<(const Rev& x) const {
        // Stage 1: compare commits
        if ((bool)commit != (bool)x.commit) return (bool)commit<(bool)x.commit;
        if (!commit) return false; // Handle both commits==nullptr
        if (commit != x.commit) return commit<x.commit;
        // Stage 2: commits are the same; compare the refs
        if ((bool)ref != (bool)x.ref) return (bool)ref<(bool)x.ref;
        if (!ref) return false; // Handle both refs==nullptr
        return ref < x.ref;
    }
    
    Commit commit; // Mandatory
    Ref ref;       // Optional
};

struct Repo : RefCounted<git_repository*, git_repository_free> {
    using RefCounted::RefCounted;
    
    static Repo Open(std::string_view path) {
        #warning TODO: figure out whether/where/when to call git_libgit2_shutdown()
        git_libgit2_init();
        
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
    
    void headDetach() const {
        int ir = git_repository_detach_head(*get());
        if (ir) throw RuntimeError("git_repository_detach_head failed: %s", git_error_last()->message);
    }
    
    void checkout(std::string_view name) const {
        Ref ref = refFullNameLookup(name);
        const git_checkout_options opts = GIT_CHECKOUT_OPTIONS_INIT;
        int ir = git_checkout_tree(*get(), (git_object*)*ref.tree(), &opts);
        if (ir) throw RuntimeError("git_checkout_tree failed: %s", git_error_last()->message);
        ir = git_repository_set_head(*get(), name.data());
        if (ir) throw RuntimeError("git_repository_set_head failed: %s", git_error_last()->message);
    }
    
    Index treesMerge(Tree ancestorTree, Tree dstTree, Tree srcTree) const {
        git_merge_options mergeOpts = GIT_MERGE_OPTIONS_INIT;
        git_index* x = nullptr;
        int ir = git_merge_trees(&x, *get(), *ancestorTree, *dstTree, *srcTree, &mergeOpts);
        if (ir) throw RuntimeError("git_merge_trees failed: %s", git_error_last()->message);
        return x;
    }
    
    Tree indexWrite(Index index) const {
        git_oid treeId;
        int ir = git_index_write_tree_to(&treeId, *index, *get());
        if (ir) throw RuntimeError("git_index_write_tree_to failed: %s", git_error_last()->message);
        
        git_tree* x = nullptr;
        ir = git_tree_lookup(&x, *get(), &treeId);
        if (ir) throw RuntimeError("git_tree_lookup failed: %s", git_error_last()->message);
        return x;
    }
    
    // Creates a new child commit of `parent` with the tree `tree`, using the metadata from `metadata`
    Commit commitAttach(Commit parent, Tree tree, Commit metadata) const {
        git_oid id;
        const git_commit* parents[] = {*parent};
        int ir = git_commit_create(
            &id,
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
        return commitLookup(id);
    }
    
    // commitAttach: attaches (cherry-picks) `src` onto `dst` and returns the result
    Commit commitAttach(Commit dst, Commit src) const {
        Tree srcTree = src.tree();
        Tree dstTree = dst.tree();
        Tree ancestorTree = src.parent().tree();
        Index mergedTreesIndex = treesMerge(ancestorTree, dstTree, srcTree);
        Tree newTree = indexWrite(mergedTreesIndex);
        return commitAttach(dst, newTree, src);
    }
    
    // commitIntegrate: adds the content of `src` into `dst` and returns the result
    Commit commitIntegrate(Commit dst, Commit src) const {
        Tree srcTree = src.tree();
        Tree dstTree = dst.tree();
        Tree ancestorTree = src.parent().tree();
        Index mergedTreesIndex = treesMerge(ancestorTree, dstTree, srcTree);
        Tree newTree = indexWrite(mergedTreesIndex);
        git_oid id;
        
        // Combine the commit messages
        std::stringstream msg;
        msg << git_commit_message(*dst);
        msg << "\n";
        msg << git_commit_message(*src);
        
        int ir = git_commit_amend(&id, *dst, nullptr, nullptr, nullptr, git_commit_message_encoding(*dst), msg.str().c_str(), *newTree);
        if (ir) throw RuntimeError("git_commit_amend failed: %s", git_error_last()->message);
        return commitLookup(id);
    }
    
    Ref refReplace(Ref ref, Commit commit) const {
        if (Branch branch = Branch::FromRef(ref)) {
            return branchReplace(branch, commit);
        
        } else if (false) {
            #warning TODO: handle tags
        
        } else {
            throw RuntimeError("unknown ref type");
        }
    }
    
    Branch branchReplace(Branch branch, Commit commit) const {
        Branch upstream = branch.upstream();
        Branch newBranch = branchCreate(branch.name(), commit, true);
        if (upstream) newBranch.setUpstream(upstream);
        return newBranch;
    }
    
    Ref refLookup(std::string_view name) const {
        git_reference* x = nullptr;
        int ir = git_reference_dwim(&x, *get(), name.data());
        if (ir) throw RuntimeError("git_reference_dwim failed: %s", git_error_last()->message);
        return x;
    }
    
    Ref refFullNameLookup(std::string_view name) const {
        git_reference* x = nullptr;
        int ir = git_reference_lookup(&x, *get(), name.data());
        if (ir) throw RuntimeError("git_reference_dwim failed: %s", git_error_last()->message);
        return x;
    }
    
    Ref refReload(Ref ref) const {
        return refFullNameLookup(ref.fullName());
    }
    
    Rev revLookup(std::string_view str) {
        Object obj;
        Ref ref;
        
        // Determine whether `str` is a ref or commit
        {
            git_object* po = nullptr;
            git_reference* pr = nullptr;
            int ir = git_revparse_ext(&po, &pr, *get(), str.data());
            if (ir) throw RuntimeError("git_revparse_ext failed: %s", git_error_last()->message);
            obj = po;
            ref = pr;
        }
        
        // If we have a ref, construct this Rev as a ref
        if (ref) return Rev(ref);
        return Rev(Commit::FromObject(obj));
    }
    
    Commit commitLookup(const Id& id) const {
        git_commit* x = nullptr;
        int ir = git_commit_lookup(&x, *get(), &id);
        if (ir) throw RuntimeError("git_commit_lookup failed: %s", git_error_last()->message);
        return x;
    }
    
    Commit commitLookup(std::string_view idStr) const {
        Id id;
        int ir = git_oid_fromstr(&id, idStr.data());
        if (ir) throw RuntimeError("git_oid_fromstr failed: %s", git_error_last()->message);
        return commitLookup(id);
    }
    
//    AnnotatedCommit annotatedCommitForRef(const Id& id) const {
//        git_annotated_commit* x = nullptr;
//        int ir = git_annotated_commit_lookup(&x, *get(), &id);
//        if (ir) throw RuntimeError("git_commit_lookup failed: %s", git_error_last()->message);
//        return x;
//    }
    
//    AnnotatedCommit annotatedCommitLookup(const Id& id) const {
//        git_annotated_commit* x = nullptr;
//        int ir = git_annotated_commit_lookup(&x, *get(), &id);
//        if (ir) throw RuntimeError("git_commit_lookup failed: %s", git_error_last()->message);
//        return x;
//    }
//    
//    AnnotatedCommit annotatedCommitLookup(std::string_view idStr) const {
//        Id id;
//        int ir = git_oid_fromstr(&id, idStr.data());
//        if (ir) throw RuntimeError("git_oid_fromstr failed: %s", git_error_last()->message);
//        return annotatedCommitLookup(id);
//    }
    
    Branch branchLookup(std::string_view name) const {
        git_reference* x = nullptr;
        int ir = git_branch_lookup(&x, *get(), name.data(), GIT_BRANCH_LOCAL);
        if (ir) throw RuntimeError("git_branch_lookup failed: %s", git_error_last()->message);
        return x;
    }
    
    Branch branchCreate(std::string_view name, Commit commit, bool force=false) const {
        git_reference* x = nullptr;
        int ir = git_branch_create(&x, *get(), name.data(), *commit, force);
        if (ir) throw RuntimeError("git_branch_create failed: %s", git_error_last()->message);
        return x;
    }
};

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

} // namespace Git
