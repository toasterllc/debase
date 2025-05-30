#pragma once
#include <filesystem>
#include <optional>
#include <vector>
#include <cassert>
#include <cstring>
#include "Debase.h"
#include "RefCounted.h"
#include "lib/toastbox/RuntimeError.h"
#include "lib/toastbox/Defer.h"
#include "lib/toastbox/String.h"
#include "lib/libgit2/include/git2.h"

namespace Git {
using namespace Toastbox;

// Macros to handle checking for nullptr before comparing underlying objects
#define _Equal(a, b, cmp) ({                \
    bool r = ((bool)(a) == (bool)(b));      \
    if ((bool)(a) && (bool)(b)) r = (cmp);  \
    r;                                      \
})

#define _Less(a, b, cmp) ({                 \
    bool r = ((bool)(a) < (bool)(b));       \
    if ((bool)(a) && (bool)(b)) r = (cmp);  \
    r;                                      \
})

class Error : public std::runtime_error {
public:
    Error(int error, const char* msg=nullptr) :
    std::runtime_error(_ErrorMsg(msg)), error((git_error_code)error) {}
    
    git_error_code error = GIT_OK;
    
protected:
    static std::string _ErrorMsg(const char* msg) {
        if (!msg) return git_error_last()->message;
        
        char buf[256];
        const git_error* gitErr = git_error_last();
        const char* gitErrMsg = (gitErr ? gitErr->message : nullptr);
        if (gitErrMsg) {
            snprintf(buf, sizeof(buf), "%s: %s", msg, gitErrMsg);
        } else {
            snprintf(buf, sizeof(buf), "%s", msg);
        }
        return buf;
    }
};

class ConflictError : public Error {
public:
    ConflictError(int error, const std::vector<std::filesystem::path>& paths) : Error(error), paths(paths) {}
    std::vector<std::filesystem::path> paths;
};

using Id = git_oid;

inline std::string StringFromId(const git_oid& oid) {
    return git_oid_tostr_s(&oid);
}

inline Id IdFromString(const std::string& str) {
    Id id;
    int ir = git_oid_fromstr(&id, str.c_str());
    if (ir) throw Error(ir, "git_oid_fromstr failed");
    return id;
}

inline std::string DisplayStringForId(const git_oid& oid, size_t len) {
    assert(len <= GIT_OID_HEXSZ);
    char str[GIT_OID_HEXSZ+1];
    git_oid_tostr(str, std::min(sizeof(str), len+1), &oid);
    return str;
}

static const char* _Weekdays[] = { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat", };
static const char* _Months[]   = { "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec", };

struct Time {
    time_t time = 0;
    int offset = 0;
    
    bool operator ==(const Time& x) const {
        return time==x.time && offset==x.offset;
    }
};

inline Time TimeForGitTime(const git_time& t) {
    return {
        .time = t.time,
        .offset = t.offset,
    };
}

inline std::string ShortStringForTime(const Time& t) {
    time_t tmp = t.time + t.offset*60;
    
    struct tm tm = {};
    struct tm* tr = gmtime_r(&tmp, &tm);
    if (!tr) throw RuntimeError("gmtime_r failed: %s", strerror(errno));
    
    char buf[64];
    snprintf(buf, sizeof(buf), "%s %s %u %02u:%02u",
        _Weekdays[tm.tm_wday], _Months[tm.tm_mon], tm.tm_mday, tm.tm_hour, tm.tm_min);
    return buf;
}

inline std::string StringFromTime(const Time& t) {
    time_t tmp = t.time + t.offset*60;
    
    struct tm tm = {};
    struct tm* tr = gmtime_r(&tmp, &tm);
    if (!tr) throw RuntimeError("gmtime_r failed: %s", strerror(errno));
    
    char buf[64];
    snprintf(buf, sizeof(buf), "%s %s %d %02d:%02d:%02d %04d %+03d%02d",
        _Weekdays[tm.tm_wday], _Months[tm.tm_mon], tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec, tm.tm_year+1900, t.offset/60, t.offset%60);
    return buf;
}

inline int _WeekdayParse(std::string_view str) {
    for (size_t i=0; i<std::size(_Weekdays); i++) {
        if (str == _Weekdays[i]) {
            return (int)i;
        }
    }
    throw RuntimeError("failed to parse weekday: %s", str);
}

inline int _MonthParse(std::string_view str) {
    for (size_t i=0; i<std::size(_Months); i++) {
        if (str == _Months[i]) {
            return (int)i;
        }
    }
    throw RuntimeError("failed to parse weekday: %s", str);
}

inline Time TimeFromString(const std::string& str) {
    struct tm tm = {};
    
    char weekday[16];
    char month[16];
    int offset = 0;
    int ir = sscanf(str.c_str(), "%15s %15s %d %d:%d:%d %d %d",
        weekday, month, &tm.tm_mday, &tm.tm_hour, &tm.tm_min, &tm.tm_sec, &tm.tm_year, &offset
    );
    if (ir != 8) throw RuntimeError("sscanf failed");
    
    // Cap string fields to 3 characters
    weekday[3] = 0;
    month[3] = 0;
    
    tm.tm_wday = _WeekdayParse(weekday);
    tm.tm_mon = _MonthParse(month);
    tm.tm_year -= 1900;
    
    int offs = (offset >= 0 ? 1 : -1);
    int offh = std::abs(offset)/100;
    int offm = std::abs(offset)%100;
    offset = offs * offh*60 + offm;
    
    time_t t = timegm(&tm);
    if (t == -1) throw RuntimeError("timegm failed");
    t -= offset*60;
    
    return {
        .time = t,
        .offset = offset,
    };
}

using Tree = RefCounted<git_tree*, git_tree_free>;

static void _MergeFileResultFree(git_merge_file_result& x) {
    git_merge_file_result_free(&x);
}

using MergeFileResult = RefCounted<git_merge_file_result, _MergeFileResultFree>;

struct Index : RefCounted<git_index*, git_index_free> {
    using RefCounted::RefCounted;
    
    bool conflicts() const { return git_index_has_conflicts(*get()); }
    
    const git_index_entry* operator [](size_t i) const {
        return git_index_get_byindex(*get(), i);
    }
    
    const git_index_entry* find(const std::filesystem::path& path, git_index_stage_t stage=GIT_INDEX_STAGE_ANY) const {
        return git_index_get_bypath(*get(), path.c_str(), stage);
    }
    
    void add(const git_index_entry& entry) const {
        int ir = git_index_add(*get(), &entry);
        if (ir) throw Error(ir, "git_index_add failed");
    }
    
    void remove(const std::filesystem::path& path, git_index_stage_t stage=GIT_INDEX_STAGE_ANY) const {
        int ir = git_index_remove(*get(), path.c_str(), stage);
        if (ir) throw Error(ir, "git_index_remove failed");
    }
    
    void conflictClear(const std::filesystem::path& path) const {
        int ir = git_index_conflict_remove(*get(), path.c_str());
        if (ir) throw Error(ir, "git_index_conflict_remove failed");
    }
};

struct StatusList : RefCounted<git_status_list*, git_status_list_free> {
    using RefCounted::RefCounted;
    
    const git_status_entry* operator [](size_t i) {
        return git_status_byindex(*get(), i);
    }
};

struct Signature : RefCounted<git_signature*, git_signature_free> {
    using RefCounted::RefCounted;
    static Signature Create(const std::string& name, const std::string& email, git_time_t time, int offset) {
        git_signature* x = nullptr;
        int ir = git_signature_new(&x, name.c_str(), email.c_str(), time, offset);
        if (ir) throw Error(ir, "git_signature_new failed");
        return x;
    }
    
    std::string name() const { return (*get())->name; }
    std::string email() const { return (*get())->email; }
    git_time time() const { return (*get())->when; }
};

static void _BufDelete(git_buf& buf) { git_buf_dispose(&buf); }
using Buf = RefCounted<git_buf, _BufDelete>;

struct Object : RefCounted<git_object*, git_object_free> {
    using RefCounted::RefCounted;
    const Id& id() const { return *git_object_id(*get()); }
    bool operator ==(const Object& x) const { return _Equal(*this, x, git_oid_cmp(&id(), &x.id())==0); }
    bool operator !=(const Object& x) const { return !(*this==x); }
    bool operator <(const Object& x) const { return _Less(*this, x, git_oid_cmp(&id(), &x.id())<0); }
};

struct Config : RefCounted<git_config*, git_config_free> {
    using RefCounted::RefCounted;
    std::optional<std::string> stringGet(const std::string& key) {
        Buf buf;
        {
            git_buf x = GIT_BUF_INIT;
            int ir = git_config_get_string_buf(&x, *get(), key.c_str());
            if (ir == GIT_ENOTFOUND) return std::nullopt;
            if (ir) throw Error(ir, "git_config_get_string_buf failed");
            buf = x;
        }
        return buf->ptr;
    }
};

struct Submodule : RefCounted<git_submodule*, git_submodule_free> {
    using RefCounted::RefCounted;
    
    std::filesystem::path path() const {
        return git_submodule_path(*get());
    }
    
    void update() {
        int ir = git_submodule_update(*get(), false, nullptr);
        if (ir) throw Error(ir, "git_submodule_update failed");
    }
    
    const Id* indexId() const {
        return git_submodule_index_id(*get());
    }
};

struct Commit : Object {
    using Object::Object;
    Commit(const git_commit* x) : Object((git_object*)x) {}
    const git_commit** get() const { return (const git_commit**)Object::get(); }
    const git_commit*& operator *() const { return *get(); }
//    git_commit** get() { return (git_commit**)Object::get(); }
//    git_commit*& operator *() { return *get(); }
    
    static Commit ForObject(Object obj) {
        git_commit* x = nullptr;
        int ir = git_object_peel((git_object**)&x, *obj, GIT_OBJECT_COMMIT);
        if (ir) throw Error(ir, "git_object_peel failed");
        return x;
    }
    
    Tree tree() const {
        git_tree* x = nullptr;
        int ir = git_commit_tree(&x, *get());
        if (ir) throw Error(ir, "git_commit_tree failed");
        return x;
    }
    
    Commit parent(size_t n=0) const {
        git_commit* x = nullptr;
        int ir = git_commit_parent(&x, *get(), (unsigned int)n);
        if (ir == GIT_ENOTFOUND) return nullptr; // `this` is the root commit -> no parent exists
        if (ir) throw Error(ir, "git_commit_tree failed");
        return x;
    }
    
    Signature author() const {
        const git_signature* author = git_commit_author(*get());
        if (!author) return nullptr;
        git_signature* x = nullptr;
        int ir = git_signature_dup(&x, author);
        if (ir) throw Error(ir, "git_signature_dup failed");
        return x;
    }
    
    std::string message() const {
        return git_commit_message(*get());
    }
    
    std::vector<Commit> parents() const {
        std::vector<Commit> p;
        size_t n = git_commit_parentcount(*get());
        for (size_t i=0; i<n; i++) p.push_back(parent(i));
        return p;
    }
    
    bool isMerge() const {
        return git_commit_parentcount(*get()) > 1;
    }
    
    std::string idStr() const {
        return git_oid_tostr_s(&id());
    }
    
    // To allow json serialization
    operator std::string() const {
        return idStr();
    }
};

struct Blob : Object {
    using Object::Object;
    Blob(const git_blob* x) : Object((git_object*)x) {}
    const git_blob** get() const { return (const git_blob**)Object::get(); }
    const git_blob*& operator *() const { return *get(); }
    
    const void* data() const { return git_blob_rawcontent(*get()); }
    size_t size() const { return git_blob_rawsize(*get()); }
};

struct TagAnnotation : Object {
    using Object::Object;
    TagAnnotation(const git_tag* x) : Object((git_object*)x) {}
    const git_tag** get() const { return (const git_tag**)Object::get(); }
    const git_tag*& operator *() const { return *get(); }
    
    Signature author() const {
        const git_signature* author = git_tag_tagger(*get());
        if (!author) return nullptr;
        git_signature* x = nullptr;
        int ir = git_signature_dup(&x, author);
        if (ir) throw Error(ir, "git_signature_dup failed");
        return x;
    }
    
    std::string name() const {
        return git_tag_name(*get());
    }
    
    std::string message() const {
        return git_tag_message(*get());
    }
};

struct Ref : RefCounted<git_reference*, git_reference_free> {
    using RefCounted::RefCounted;
    bool operator ==(const Ref& x) const { return _Equal(*this, x, fullName()==x.fullName()); }
    bool operator !=(const Ref& x) const { return !(*this==x); }
    bool operator <(const Ref& x) const { return _Less(*this, x, fullName()<x.fullName()); }
    
    std::string name() const {
        return git_reference_shorthand(*get());
    }
    
    std::string fullName() const {
        return git_reference_name(*get());
    }
    
    bool isBranch() const {
        return isLocalBranch() || isRemoteBranch();
    }
    
    bool isLocalBranch() const {
        return git_reference_is_branch(*get());
    }
    
    bool isRemoteBranch() const {
        return git_reference_is_remote(*get());
    }
    
    bool isTag() const {
        return git_reference_is_tag(*get());
    }
    
    Commit commit() const {
        git_commit* x = nullptr;
        int ir = git_reference_peel((git_object**)&x, *get(), GIT_OBJECT_COMMIT);
        if (ir) throw Error(ir, "git_reference_peel failed");
        return x;
    }
    
    Tree tree() const {
        git_tree* x = nullptr;
        int ir = git_reference_peel((git_object**)&x, *get(), GIT_OBJECT_TREE);
        if (ir) throw Error(ir, "git_reference_peel failed");
        return x;
    }
    
//    void remove() const {
//        int ir = git_reference_delete(*get());
//        if (ir) throw Error(ir, "git_reference_delete failed");
//    }
    
    // To allow json serialization
    operator std::string() const {
        return fullName();
    }
};

struct Branch : Ref {
    using Ref::Ref;
    
    static Branch ForRef(const Ref& ref) {
        assert(ref.isBranch());
        git_reference* x = nullptr;
        int ir = git_reference_dup(&x, *ref);
        if (ir) throw Error(ir, "git_reference_dup failed");
        return x;
    }
    
    // local(): returns true if this is a local branch, false if this a remote tracking branch
    bool local() const {
        return git_reference_is_branch(*get());
    }
    
    std::string name() const {
        const char* x = nullptr;
        int ir = git_branch_name(&x, *get());
        if (ir) throw Error(ir, "git_branch_name failed");
        return x;
    }
    
//    std::string nameLocal() const {
//        std::string n = name();
//        if (local()) return n;
//        
//        Buf buf;
//        {
//            git_buf x = GIT_BUF_INIT;
//            int ir = git_branch_remote_name(&x, *get(), key.c_str());
//            if (ir == GIT_ENOTFOUND) return std::nullopt;
//            if (ir) throw Error(ir, "git_config_get_string_buf failed");
//            buf = x;
//        }
//        
//        
//        
//        n.
//        git_reference_is_branch(*get()) || git_reference_is_remote(*get())
//    }
    
    Branch upstream() const {
        git_reference* x = nullptr;
        int ir = git_branch_upstream(&x, *get());
        switch (ir) {
        case 0:             return x;
        case GIT_ENOTFOUND: return nullptr;
        default:            throw Error(ir, "git_branch_upstream failed");
        }
    }
    
    void upstreamSet(Branch upstream) {
        int ir = git_branch_set_upstream(*get(), git_reference_shorthand(*upstream));
        if (ir) throw Error(ir, "git_branch_upstream_name failed");
    }
};

struct Tag : Ref {
    using Ref::Ref;
    
    static Tag ForRef(const Ref& ref) {
        assert(ref.isTag());
        git_reference* x = nullptr;
        int ir = git_reference_dup(&x, *ref);
        if (ir) throw Error(ir, "git_reference_dup failed");
        return x;
    }
    
    TagAnnotation annotation() const {
        git_tag* x = nullptr;
        int ir = git_reference_peel((git_object**)&x, *get(), GIT_OBJECT_TAG);
        // Don't throw on failures, since we use this method to determine whether the tag is an annotated tag or not
        if (ir) return nullptr;
        return x;
    }
};

// A Rev holds a mandatory commit, along with an optional reference (ie a branch/tag)
// that targets that commit
class Rev {
public:
    // Invalid Rev
    Rev() {}
    
    Rev(Commit commit) : commit(commit) {}
    
    Rev(Ref ref) : ref(ref) {
        commit = ref.commit();
        assert(commit); // Verify assumption: if we have a ref, it points to a valid commit
    }
    
    operator bool() const { return (bool)commit; }
    
    bool operator ==(const Rev& x) const {
        if (commit != x.commit) return false;
        if (ref != x.ref) return false;
        return true;
    }
    
    bool operator !=(const Rev& x) const { return !(*this==x); }
    
    bool operator <(const Rev& x) const {
        if (commit != x.commit) return commit<x.commit;
        if (ref != x.ref) return ref<x.ref;
        return false;
    }
    
//    std::string name() const {
//        constexpr size_t Len = 7;
//        if (ref) return ref.name();
//        return Git::DisplayStringForId(commit.id(), Len);
//    }
    
    std::string fullName() const {
        if (ref) return ref.fullName();
        return StringFromId(commit.id());
    }
    
    bool isMutable() const {
        assert(*this);
        // Only ref-backed revs are mutable
        if (!ref) return false;
        return ref.isLocalBranch() || ref.isTag();
    }
    
//    operator bool() const { return (bool)commit; }
//    
//    bool operator ==(const Rev& x) const {
//        if (commit != x.commit) return false;
//        if (ref != x.ref) return false;
//        return true;
//    }
//    
//    bool operator !=(const Rev& x) const { return !(*this==x); }
//    
//    bool operator <(const Rev& x) const {
//        if (commit != x.commit) return commit<x.commit;
//        if (ref != x.ref) return ref<x.ref;
//        return false;
//    }
//    
//    std::string displayName() const {
//        constexpr size_t Len = 7;
//        if (ref) return ref.name();
//        return DisplayStringForId(commit.id(), Len);
//    }
//    
//    std::string fullName() const {
//        if (ref) return ref.fullName();
//        return StringFromId(commit.id());
//    }
    
    Commit commit;  // Mandatory
    Ref ref;        // Optional
};

struct Reflog : RefCounted<git_reflog*, git_reflog_free> {
    using RefCounted::RefCounted;
    
    void append(const Signature& sig, const Rev& curr, const Rev& next) const {
        const std::string currName = (curr.ref ? curr.ref.name() : curr.commit.idStr());
        const std::string nextName = (next.ref ? next.ref.name() : next.commit.idStr());
        const std::string msg = "checkout: moving from " + currName + " to " + nextName;
        int ir = git_reflog_append(*get(), &next.commit.id(), *sig, msg.c_str());
        if (ir) throw Error(ir, "git_reflog_append failed");
        ir = git_reflog_write(*get());
        if (ir) throw Error(ir, "git_reflog_write failed");
    }
    
    void drop(size_t idx) const {
        int ir = git_reflog_drop(*get(), idx, 1);
        if (ir) throw Error(ir, "git_reflog_drop failed");
        ir = git_reflog_write(*get());
        if (ir) throw Error(ir, "git_reflog_write failed");
    }
    
    const git_reflog_entry* operator [](size_t idx) const {
        return git_reflog_entry_byindex(*get(), idx);
    }
    
    const git_reflog_entry* at(size_t idx) const {
        if (idx >= git_reflog_entrycount(*get())) {
            throw std::out_of_range("index out of range");
        }
        return git_reflog_entry_byindex(*get(), idx);
    }
    
    size_t len() const {
        return git_reflog_entrycount(*get());
    }
};

inline void _RepoFree(git_repository* repo) {
    git_repository_free(repo);
    git_libgit2_shutdown(); // Balance call in Repo::Open()
}

class Repo : public RefCounted<git_repository*, _RepoFree> {
public:
    using RefCounted::RefCounted;
    
    static std::filesystem::path _RepoDir(const std::filesystem::path& path) {
        Buf buf;
        {
            git_buf x = GIT_BUF_INIT;
            int ir = git_repository_discover(&x, path.c_str(), false, nullptr);
            if (ir) throw Error(ir, "git_repository_discover failed");
            buf = x;
        }
        
        // Add a trailing slash to ensure we're in the form /a/b/.git///, with at least one trailing slash.
        // This is necessary because parent_path("/a/b/.git///") returns /a/b/.git, not /a/b.
        // So by ensuring there's at least one trailing slash, we ensure that we can call parent_path()
        // twice to get the actual parent directory:
        //   parent_path(parent_path("/a/b/.git///")) returns "/a/b"
        const std::filesystem::path dir = std::filesystem::path(buf->ptr) / "";
        return dir.parent_path().parent_path();
    }
    
    static Repo Open(const std::filesystem::path& path) {
        namespace fs = std::filesystem;
        bool shutdown = true;
        git_libgit2_init();
        Defer( if (shutdown) git_libgit2_shutdown() );
        
        const fs::path dir = _RepoDir(path);
        git_repository* x = nullptr;
        int ir = git_repository_open(&x, dir.c_str());
        if (ir) throw Error(ir, "git_repository_open failed");
        
        // We succeeded -- don't call shutdown!
        shutdown = false;
        return x;
    }
    
    static Repo Open(const Submodule& sm) {
        bool shutdown = true;
        git_libgit2_init();
        Defer( if (shutdown) git_libgit2_shutdown() );
        
        git_repository* x = nullptr;
        int ir = git_submodule_open(&x, *sm);
        if (ir) throw Error(ir, "git_submodule_open failed");
        
        // We succeeded -- don't call shutdown!
        shutdown = false;
        return x;
    }
    
    std::filesystem::path path() const {
        return git_repository_workdir(*get());
    }
    
    Ref head() const {
        return refFullNameLookup("HEAD");
    }
    
    Rev headResolved() const {
        Ref ref;
        {
            git_reference* x = nullptr;
            int ir = git_repository_head(&x, *get());
            if (ir) throw Error(ir, "git_repository_head failed");
            ref = x;
        }
        
        if (ref.isBranch()) return ref;
        if (ref.isTag()) return ref; // HEAD can't point to tags currently, but checking anyway for completeness
        
        // The ref isn't a branch or tag, so it's just the detached "HEAD" pointer.
        // Try to derive the ref by looking at the last reflog entry.
        // This heuristic allows us to get the ref in the case where the repo has a
        // tag checked out, in which case the `ref.isTag()` check (above) returns
        // false (because git doesn't allow HEAD to point to tags currently --
        // instead it's a detached HEAD).
        try {
            Reflog reflog = reflogForRef(head());
            const git_reflog_entry*const entry = reflog.at(0);
            const Rev rev = reflogRevForCheckoutEntry(entry);
            if (rev.ref && rev.commit==ref.commit()) {
                return rev;
            }
        } catch (...) {}
        
        return ref.commit();
    }
    
    void checkout(const Rev& rev) const {
        struct Ctx {
            std::vector<std::filesystem::path> conflicts;
        };
        Ctx ctx;
        
        git_checkout_options opts = GIT_CHECKOUT_OPTIONS_INIT;
        opts.notify_flags |= GIT_CHECKOUT_NOTIFY_CONFLICT;
        opts.notify_payload = &ctx;
        opts.notify_cb = [] (
            git_checkout_notify_t why,
            const char* path,
            const git_diff_file* baseline,
            const git_diff_file* target,
            const git_diff_file* workdir,
            void* ctxp
        ) -> int {
            Ctx& ctx = *(Ctx*)ctxp;
            if (why == GIT_CHECKOUT_NOTIFY_CONFLICT) {
                ctx.conflicts.push_back(workdir->path);
            }
            return 0;
        };
        
        int ir = git_checkout_tree(*get(), (git_object*)*rev.commit.tree(), &opts);
        if (ir == GIT_ECONFLICT) throw ConflictError(ir, ctx.conflicts);
        else if (ir)             throw Error(ir, "git_checkout_tree failed");
        
        if (rev.ref) {
            const std::string fullName = rev.ref.fullName();
            ir = git_repository_set_head(*get(), fullName.c_str());
            if (ir) throw Error(ir, "git_repository_set_head failed");
        
        } else {
            ir = git_repository_set_head_detached(*get(), &rev.commit.id());
            if (ir) throw Error(ir, "git_repository_set_head_detached failed");
        }
        
        submodulesUpdate(true);
    }
    
    
    void headDetach() const {
        int ir = git_repository_detach_head(*get());
        if (ir) throw Error(ir, "git_repository_detach_head failed");
    }
    
    void headAttach(const Rev& rev) const {
        checkout(rev);
    }
    
//    void headDetach() const {
//        int ir = git_repository_head_detached(*get());
//        if (ir < 0) throw Error(ir, "git_repository_head_detached failed");
//        
//        // Short-circuit if we're already detached
//        // This is important so that we don't erroneously drop entries from the reflog when
//        // we didn't actually detach, which we would otherwise do without this check
//        if (ir == 1) return;
//        
//        ir = git_repository_detach_head(*get());
//        if (ir) throw Error(ir, "git_repository_detach_head failed");
//        
//        // Forget that we detached head
//        const Reflog reflog = reflogForRef(head());
//        reflog.drop(0);
//    }
//    
//    void headAttach(const Rev& rev) const {
//        const Reflog reflogPrev = reflogForRef(head());
//        
//        checkout(rev);
//        
//        // Forget that we attached head, if a new reflog entry was created
//        const Reflog reflog = reflogForRef(head());
//        if (reflogPrev.len() != reflog.len()) {
//            reflog.drop(0);
//        }
//    }
    
    
//    void headAttach(const Rev& rev) const {
//        const Reflog reflogPrev = reflogForRef(head());
//        const git_reflog_entry* const entryPrev = reflogPrev[0];
//        const char*const msgPrev = git_reflog_entry_message(entryPrev);
//        
//        checkout(rev);
//        
//        // Forget that we attached head, but only if a reflog entry was pushed as a result
//        // of the checkout.
//        // This check is necessary because headAttach() could be called twice for the same
//        // rev, and the second call wouldn't affect the reflog, so we shouldn't drop the
//        // reflog entry in that case.
//        const Reflog reflog = reflogForRef(head());
//        const git_reflog_entry* const entry = reflog[0];
//        const char*const msg = git_reflog_entry_message(entry);
//        if (msgPrev && msg) {
//            if (strcmp(msgPrev, msg)) {
//                reflog.drop(0);
//            }
//        }
//    }
    
//    void headDetach() const {
//        const Reflog reflog = reflogForRef(head());
//        const git_reflog_entry*const entryPrev = reflog[0];
//        
//        int ir = git_repository_detach_head(*get());
//        if (ir) throw Error(ir, "git_repository_detach_head failed");
//        
//        // Forget that we detached head
//        const git_reflog_entry*const entry = reflog[0];
//        if (entry != entryPrev) reflog.drop(0);
//    }
//    
//    void headAttach(const Rev& rev) const {
//        const Reflog reflog = reflogForRef(head());
//        const git_reflog_entry*const entryPrev = reflog[0];
//        
//        checkout(rev);
//        
//        // Forget that we attached head
//        const git_reflog_entry*const entry = reflog[0];
//        if (entry != entryPrev) reflog.drop(0);
//    }
    
    Index treesMerge(git_merge_file_favor_t fileFavor, const Tree& ancestorTree, const Tree& dstTree, const Tree& srcTree) const {
        git_merge_options opts = GIT_MERGE_OPTIONS_INIT;
        opts.file_favor = fileFavor;
        
        git_index* x = nullptr;
        int ir = git_merge_trees(
            &x,
            *get(),
            (ancestorTree ? *ancestorTree : nullptr),
            (dstTree ? *dstTree : nullptr),
            (srcTree ? *srcTree : nullptr),
            &opts
        );
        if (ir) throw Error(ir, "git_merge_trees failed");
        return x;
    }
    
    Tree indexWrite(const Index& index) const {
        Id treeId;
        int ir = git_index_write_tree_to(&treeId, *index, *get());
        if (ir) throw Error(ir, "git_index_write_tree_to failed");
        
        git_tree* x = nullptr;
        ir = git_tree_lookup(&x, *get(), &treeId);
        if (ir) throw Error(ir, "git_tree_lookup failed");
        return x;
    }
    
    // commitParentSet(): commit.parent[0] = parent
    Index commitParentSet(git_merge_file_favor_t fileFavor, const Commit& commit, const Commit& parent) const {
        assert(commit);
        
        if (parent) {
            git_merge_options opts = GIT_MERGE_OPTIONS_INIT;
            opts.file_favor = fileFavor;
            
            const unsigned int mainline = (commit.isMerge() ? 1 : 0);
            git_index* x = nullptr;
            int ir = git_cherrypick_commit(&x, *get(), (git_commit*)*commit, (git_commit*)*parent, mainline, &opts);
            if (ir) throw Error(ir, "git_cherrypick_commit failed");
            return x;
        
        } else {
            Commit oldParent = commit.parent();
            Tree oldParentTree = (oldParent ? oldParent.tree() : nullptr);
            return treesMerge(fileFavor, oldParentTree, nullptr, commit.tree());
        }
    }
    
    Commit commitParentSetFinish(const Index& index, const Commit& commit, const Commit& parent) const {
        assert(commit);
        
        Tree tree = indexWrite(index);
        std::vector<Commit> parents = commit.parents();
        if (!parents.empty()) parents.erase(parents.begin());
        if (parent) parents.insert(parents.begin(), parent);
        return commitAmend(commit, parents, tree);
    }
    
    // commitIntegrate: adds the content of `src` into `dst` and returns the result
    Index commitIntegrate(git_merge_file_favor_t fileFavor, const Commit& dst, const Commit& src) const {
        Tree srcTree = src.tree();
        Tree dstTree = dst.tree();
        Tree ancestorTree = src.parent().tree();
        return treesMerge(fileFavor, ancestorTree, dstTree, srcTree);
    }
    
    // commitIntegrate: adds the content of `src` into `dst` and returns the result
    Commit commitIntegrateFinish(const Index& index, const Commit& dst, const Commit& src) const {
        Tree newTree = indexWrite(index);
        
        // Combine the commit messages
        std::stringstream msg;
        msg << git_commit_message(*dst);
        msg << "\n";
        msg << git_commit_message(*src);
        
        Id id;
        int ir = git_commit_amend(&id, *dst, nullptr, nullptr, nullptr, git_commit_message_encoding(*dst), msg.str().c_str(), *newTree);
        if (ir) throw Error(ir, "git_commit_amend failed");
        return commitLookup(id);
    }
    
    // commitAmend(): change parents/tree of a commit
    Commit commitAmend(const Commit& commit, const std::vector<Commit>& parents, const Tree& tree) const {
        Id id;
        
        assert(parents.size() <= 4); // Protect stack-allocated array from being too large
        const git_commit* stackParents[parents.size()];
        for (size_t i=0; i<parents.size(); i++) {
            stackParents[i] = *parents[i];
        }
        
        int ir = git_commit_create(
            &id,
            *get(),
            nullptr,
            git_commit_author(*commit),
            git_commit_committer(*commit),
            git_commit_message_encoding(*commit),
            git_commit_message(*commit),
            *tree,
            parents.size(),
            stackParents
        );
        if (ir) throw Error(ir, "git_commit_create failed");
        return commitLookup(id);
    }
    
    // commitAmend(): change the author/message of a commit
    Commit commitAmend(const Commit& commit, const Signature& author, const std::string& msg) const {
        Id id;
        int ir = git_commit_amend(&id, *commit, nullptr, *author, *author,
            nullptr, (!msg.empty() ? msg.c_str() : nullptr), nullptr);
        if (ir) throw Error(ir, "git_commit_amend failed");
        return commitLookup(id);
    }
    
    Commit commitLookup(const Id& id) const {
        git_commit* x = nullptr;
        int ir = git_commit_lookup(&x, *get(), &id);
        if (ir) throw Error(ir, "git_commit_lookup failed");
        return x;
    }
    
//    Commit commitLookup(const std::string& idStr) const {
//        Id id;
//        int ir = git_oid_fromstr(&id, idStr.c_str());
//        if (ir) throw Error(ir, "git_oid_fromstr failed");
//        return commitLookup(id);
//    }
    
    Ref refReplace(const Ref& ref, const Commit& commit) const {
        if (ref.isLocalBranch()) {
            return branchReplace(Branch::ForRef(ref), commit);
        
        } else if (ref.isTag()) {
            return tagReplace(Tag::ForRef(ref), commit);
        
        } else {
            // Unknown ref type
            abort();
        }
    }
    
//    Rev revReplace(const Rev& rev, const Commit& commit) const {
//        assert(rev.ref);
//        return refReplace(rev.ref, commit);
//    }
//    
//    Rev revReplace(Rev rev, Commit commit) const {
//        assert(rev.isMutable());
//        return Rev(refReplace(rev.ref, commit), rev.refSkip);
//    }
    
    Tag tagReplace(const Tag& tag, const Commit& commit) const {
        if (TagAnnotation ann = tag.annotation()) {
            return tagCreateAnnotated(tag.name(), commit, ann.author(), ann.message(), true);
        }
        return tagCreate(tag.name(), commit, true);
    }
    
    Ref refLookup(const std::string& name) const {
        git_reference* x = nullptr;
        int ir = git_reference_dwim(&x, *get(), name.c_str());
        if (ir) throw Error(ir, "git_reference_dwim failed");
        return x;
    }
    
    Ref refFullNameLookup(const std::string& name) const {
        git_reference* x = nullptr;
        int ir = git_reference_lookup(&x, *get(), name.c_str());
        if (ir) throw Error(ir, "git_reference_dwim failed");
        return x;
    }
    
    Ref refReload(const Ref& ref) const {
        return refFullNameLookup(ref.fullName());
    }
    
    Ref refCopy(const Ref& ref, const std::string& name, Commit commit=nullptr) const {
        if (!commit) commit = ref.commit();
        
        if (ref.isBranch()) {
            return branchCreate(name, commit, false);
        
        } else if (ref.isTag()) {
            const Tag tag = Tag::ForRef(ref);
            if (const TagAnnotation ann = tag.annotation()) {
                return tagCreateAnnotated(name, commit, ann.author(), ann.message());
            } else {
                return tagCreate(name, commit);
            }
        
        } else {
            // Unsupported ref type
            throw Toastbox::RuntimeError("unsupported ref type");
        }
    }
    
    void refDelete(const Ref& ref) {
        if (ref.isLocalBranch() || ref.isTag()) {
            int ir = git_reference_delete(*ref);
            if (ir) throw Error(ir, "git_reference_delete failed");
            
        } else {
            // Unsupported ref type
            throw Toastbox::RuntimeError("unsupported ref type");
        }
    }
    
//    Ref revReload(Rev rev) const {
//        assert(rev.ref);
//        return refFullNameLookup(rev.ref.fullName());
//    }
    
    Rev revReload(const Rev& rev) const {
        if (rev.ref) return refReload(rev.ref);
        return rev;
    }
    
    Rev revLookup(const std::string& str) const {
        Ref ref;
        Object obj;
        {
            git_reference* pr = nullptr;
            git_object* po = nullptr;
            int ir = git_revparse_ext(&po, &pr, *get(), str.c_str());
            if (ir) throw Error(ir, "git_revparse_ext failed");
            obj = po;
            ref = pr;
        }
        
        if (ref) return ref;
        return Commit::ForObject(obj);
    }
    
    Branch branchLookup(const std::string& name) const {
        git_reference* x = nullptr;
        int ir = git_branch_lookup(&x, *get(), name.c_str(), GIT_BRANCH_LOCAL);
        if (ir) throw Error(ir, "git_branch_lookup failed");
        return x;
    }
    
    Branch branchCreate(const std::string& name, const Commit& commit, bool force=false) const {
        git_reference* x = nullptr;
        int ir = git_branch_create(&x, *get(), name.c_str(), (commit ? *commit : nullptr), force);
        if (ir) throw Error(ir, "git_branch_create failed");
        return x;
    }
    
    Branch branchReplace(const Branch& branch, const Commit& commit) const {
        Branch upstream = branch.upstream();
        Branch newBranch = branchCreate(branch.name(), commit, true);
        Branch newBranchUpstream = newBranch.upstream();
        
        if (upstream && !newBranchUpstream) {
            newBranch.upstreamSet(upstream);
        }
        
        return newBranch;
    }
    
    // branchNameLocal(): returns the local branch name for a branch:
    //   master         -> master
    //   origin/master  -> master
    std::string branchNameLocal(const Branch& branch) {
        std::string n = branch.name();
        if (branch.local()) return n;
        
        Buf buf;
        {
            git_buf x = GIT_BUF_INIT;
            int ir = git_branch_remote_name(&x, *get(), branch.fullName().c_str());
            if (ir) throw Error(ir, "git_branch_remote_name failed");
            buf = x;
        }
        
        const std::string prefix = std::string(buf->ptr)+"/";
        if (!String::StartsWith(prefix, n)) {
            throw RuntimeError("%s doesn't begin with %s as expected", n.c_str(), prefix.c_str());
        }
        
        return n.substr(prefix.size());
    }
    
    Tag tagLookup(const std::string& name) const {
        return Tag::ForRef(refLookup(name));
    }
    
    Tag tagCreate(const std::string& name, const Commit& commit, bool force=false) const {
        Id id;
        int ir = git_tag_create_lightweight(&id, *get(), name.c_str(), *(Object)commit, force);
        if (ir) throw Error(ir, "git_tag_create failed");
        return tagLookup(name);
    }
    
    Tag tagCreateAnnotated(const std::string& name, const Commit& commit, const Signature& author, const std::string& message, bool force=false) const {
        Id id;
        int ir = git_tag_create(&id, *get(), name.c_str(), *((Object)commit), *author, message.c_str(), force);
        if (ir) throw Error(ir, "git_tag_create failed");
        return tagLookup(name);
    }
    
    Config config() const {
        git_config* x = nullptr;
        int ir = git_repository_config(&x, *get());
        if (ir) throw Error(ir, "git_repository_config failed");
        return x;
    }
    
    StatusList status() const {
        git_status_list* x = nullptr;
        
        const git_status_options opts = GIT_STATUS_OPTIONS_INIT;
        int ir = git_status_list_new(&x, *get(), &opts);
        if (ir) throw Error(ir, "git_status_list_new failed");
        
        return x;
    }
    
    bool dirty() const {
        StatusList s = status();
        for (size_t i=0;; i++) {
            const git_status_entry* e = s[i];
            if (!e) break;
            
            constexpr int Dirty =
                GIT_STATUS_INDEX_MODIFIED   |
                GIT_STATUS_INDEX_DELETED    |
                GIT_STATUS_INDEX_RENAMED    |
                GIT_STATUS_INDEX_TYPECHANGE |
                GIT_STATUS_WT_MODIFIED      |
                GIT_STATUS_WT_DELETED       |
                GIT_STATUS_WT_RENAMED       |
                GIT_STATUS_WT_TYPECHANGE    ;
            
            if (Dirty & e->status) {
                return true;
            }
        }
        return false;
    }
    
    std::vector<Submodule> submodules() const {
        struct Ctx {
            std::filesystem::path repoPath;
            std::vector<Submodule> submodules;
        };
        Ctx ctx = { .repoPath = path() };
        
        auto callback = [] (
            git_submodule* sm,
            const char* name,
            void* ctxp
        ) -> int {
            Ctx& ctx = *(Ctx*)ctxp;
            Submodule submodule;
            {
                git_submodule* x = nullptr;
                int ir = git_submodule_dup(&x, sm);
                if (ir) return ir;
                submodule = x;
            }
            
            // Ignore submodules for which we don't have an indexId.
            // On macOS, `git_submodule_foreach` supplies the same submodule multiple
            // times, with different-case paths (eg lib/toastbox and Lib/Toastbox).
            // Empirically, only one of them returns a valid indexId, which is
            // required for Submodule::update() to work. So ignore submodules that
            // have a null indexId.
            if (!submodule.indexId()) return 0;
            
            std::filesystem::path path = std::filesystem::canonical(ctx.repoPath / submodule.path());
            ctx.submodules.push_back(submodule);
            return 0;
        };
        
        int ir = git_submodule_foreach(*get(), callback, &ctx);
        if (ir) throw Error(ir, "git_submodule_foreach failed");
        
        return ctx.submodules;
    }
    
    void submodulesUpdate(bool recurse=false) const {
        std::vector<Submodule> sms = submodules();
        for (Submodule sm : sms) {
            sm.update();
            if (recurse) {
                Repo repo = Repo::Open(sm);
                repo.submodulesUpdate(recurse);
            }
        }
    }
    
    Reflog reflogForRef(const Ref& ref) const {
        git_reflog* x = nullptr;
        int ir = git_reflog_read(&x, *get(), ref.fullName().c_str());
        if (ir) throw Error(ir, "git_reflog_read failed");
        return x;
    }
    
    static constexpr const char MergeMarkerBareStart[]      = "<<<<<<<";
    static constexpr const char MergeMarkerBareSeparator[]  = "=======";
    static constexpr const char MergeMarkerBareEnd[]        = ">>>>>>>";
    
    static constexpr const char MergeMarkerStart[]          = "<<<<<<< " _DebaseProductId;
    static constexpr const char MergeMarkerSeparator[]      = "=======";
    static constexpr const char MergeMarkerEnd[]            = ">>>>>>> " _DebaseProductId;
    
    MergeFileResult merge(
        const git_index_entry* ancestor,
        const git_index_entry* ours,
        const git_index_entry* theirs
    ) const {
        git_merge_file_options opts = {
            .version = GIT_MERGE_FILE_OPTIONS_VERSION,
            .our_label = _DebaseProductId,
            .their_label = _DebaseProductId,
            // Using the 'patience' algorithm because it seems to remove conflicts
            // where both sides contain the exact same code
            .flags = GIT_MERGE_FILE_DIFF_PATIENCE,
        };
        
        git_merge_file_result x;
        int ir = git_merge_file_from_index(&x, *get(), ancestor, ours, theirs, &opts);
        if (ir) throw Error(ir, "git_merge_file_from_index failed");
        return x;
    }
    
    Blob blobLookup(const Id& id) const {
        git_blob* x = nullptr;
        int ir = git_blob_lookup(&x, *get(), &id);
        if (ir) throw Error(ir, "git_blob_lookup failed");
        return x;
    }
    
    Blob blobCreate(const void* data, size_t len) const {
        Id id;
        int ir = git_blob_create_from_buffer(&id, *get(), data, len);
        if (ir) throw Error(ir, "git_blob_create_from_buffer failed");
        return blobLookup(id);
    }
    
    Signature signatureCreateDefault() {
        git_signature* x = nullptr;
        int ir = git_signature_default(&x, *get());
        if (ir) throw Error(ir, "git_signature_default failed");
        return x;
    }
    
    // reflogRevForCheckoutEntry(): parse and lookup the rev from the reflog
    // message string. For example, a checkout message in the reflog looks
    // like:
    //   checkout: moving from master to v1
    // And we want to extract `v1` and turn it into a Rev.
    //
    // This is of course an imperfect heuristic and ideally we wouldn't
    // be parsing log strings, but there doesn't seem to be a better option,
    // and this is how libgit2 does it to implement its handling of the
    // @{N} syntax.
    // 
    // We decided to implement this ourself instead of using libgit2's
    // implementation for 2 reasons:
    // 
    //   1. libgit2 parses the first rev in the string but we want the second,
    //      because git includes the symbolic name of revs in the second, but
    //      not the first. For example if you checkout a tag (master -> v1),
    //      the message will say:
    //      
    //          checkout: moving from master to v1
    //      
    //      but when moving from v1 -> master it'll say:
    //      
    //          checkout: moving from a561d03 to master
    //      
    //      So if we use libgit2's implementation, we can't get the tag name.
    //   
    //   2. We can implement reflog iteration much more efficiently. When
    //      parsing `@{-3}` for example, libgit2 has to perform a regex match
    //      on at least 3 reflog entries (to match the 'checkout: moving from
    //      X to Y' string), and then has to redo that same work when parsing
    //      `@{-4}`. Whereas with our implementation, we process each reflog
    //      entry only once.
    
    Rev reflogRevForCheckoutEntry(const git_reflog_entry* entry) const {
        assert(entry);
        
        const char* msg = git_reflog_entry_message(entry);
        if (!msg) throw std::runtime_error("invalid message");
        if (!Toastbox::String::StartsWith("checkout: moving from ", msg)) throw std::runtime_error("not a checkout entry");
        
        const std::string_view msgv = msg;
        const size_t lastSpaceIdx = msgv.find_last_of(' ');
        // This function should only be called for checkout reflog messages, so there must be a space.
        // If there's not, it's programmer error.
        assert(lastSpaceIdx != std::string::npos);
        std::string_view revName = msgv.substr(lastSpaceIdx+1);
        
        const size_t tildeIdx = revName.find_first_of('~');
        const size_t carrotIdx = revName.find_first_of('^');
        revName = revName.substr(0, std::min(tildeIdx, carrotIdx));
        // Ignore HEAD special pointers; eg: HEAD, ORIG_HEAD, FETCH_HEAD, REVERT_HEAD
        if (_HEADSpecialPointer(revName)) throw std::runtime_error("HEAD-based special pointer");
        return revLookup(std::string(revName));
    }
    
    void reflogRememberRef(const Ref& ref) {
        const Reflog reflog = reflogForRef(head());
        const Rev headRev = headResolved();
        const Git::Signature sig = signatureCreateDefault();
        reflog.append(sig, headRev, ref);
        reflog.append(sig, ref, headRev);
    }
    
private:
    static bool _HEADSpecialPointer(std::string_view name) {
        return Toastbox::String::EndsWith("HEAD", name);
    }
};

#undef _Equal
#undef _Less

} // namespace Git
