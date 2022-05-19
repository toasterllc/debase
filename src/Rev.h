#pragma once
#include "git/Git.h"

class Rev : public Git::Rev {
public:
    using Git::Rev::Rev;
    Rev(const Git::Rev& x) : Git::Rev(x) {}
    
    // Methods
    operator bool() const { return Git::Rev::operator bool(); }
    
    bool operator ==(const Rev& x) const {
        if (!(Git::Rev::operator ==(x))) return false;
        if (skip != x.skip) return false;
        if (mutationAllowed != x.mutationAllowed) return false;
        return true;
    }
    
    bool operator !=(const Rev& x) const { return !(*this==x); }
    
    bool operator <(const Rev& x) const {
        if (Git::Rev::operator !=(x)) return Git::Rev::operator <(x);
        if (skip != x.skip) return skip<x.skip;
        if (mutationAllowed != x.mutationAllowed) return mutationAllowed<x.mutationAllowed;
        return false;
    }
    
    std::string displayName() const {
        constexpr size_t Len = 7;
        if (ref) {
            return ref.name() + (skip ? "~" + std::to_string(skip) : "");
        }
        return Git::DisplayStringForId(commit.id(), Len);
    }
    
    // displayHead(): returns the head commit considering `skip`
    // skip==0 -> return `commit`
    // skip>0  -> returns the `skip` parent of `commit`
    Git::Commit displayHead() const {
        Git::Commit c = commit;
        size_t s = skip;
        while (c && s) {
            c = c.parent();
            s--;
        }
        return c;
    }
    
//    bool isMutable() const {
//        return true;
//    }
    
    bool isMutable() const {
        // Check if mutation has been explicitly disabled
        if (!mutationAllowed) return false;
        return Git::Rev::isMutable();
    }
    
    size_t skip = 0;
    bool mutationAllowed = true;
};











//class _Rev {
//public:
//    // Subclass Implementation
//    virtual Commit commit() const = 0;
//    virtual Ref ref() const = 0;
//    virtual size_t skip() const = 0;
//    
//    // Methods
//    virtual operator bool() const { return (bool)commit(); }
//    
//    virtual bool operator ==(const _Rev& x) const {
//        if (commit() != x.commit()) return false;
//        if (ref() != x.ref()) return false;
//        if (skip() != x.skip()) return false;
//        return true;
//    }
//    
//    virtual bool operator !=(const _Rev& x) const { return !(*this==x); }
//    
//    virtual bool operator <(const _Rev& x) const {
//        if (commit() != x.commit()) return commit()<x.commit();
//        if (ref() != x.ref()) return ref()<x.ref();
//        if (skip() != x.skip()) return skip()<x.skip();
//        return false;
//    }
//    
//    virtual std::string displayName() const {
//        constexpr size_t Len = 7;
//        if (const Ref r = ref()) {
//            const size_t s = skip();
//            return r.name() + (s ? "~" + std::to_string(s) : "");
//        }
//        return DisplayStringForId(commit().id(), Len);
//    }
//    
//    // displayHead(): returns the head commit considering `skip`
//    // skip==0 -> return `commit`
//    // skip>0  -> returns the `skip` parent of `commit`
//    virtual Commit displayHead() const {
//        Commit c = commit();
//        size_t s = skip();
//        while (c && s) {
//            c = c.parent();
//            s--;
//        }
//        return c;
//    }
//    
//    virtual std::string fullName() const {
//        if (Ref r = ref()) return r.fullName();
//        return StringForId(commit().id());
//    }
//    
//    virtual bool isMutable() const {
//        assert(*this);
//        // Only ref-backed revs are mutable
//        if (const Ref r = ref()) {
//            return r.isBranch() || r.isTag();
//        }
//        return false;
//    }
//};
//using Rev = std::shared_ptr<_Rev>;
//
//template <typename T, typename ...T_Args>
//std::shared_ptr<T> RevCreate(T_Args&&... args) {
//    return std::make_shared<T>(std::forward<T_Args>(args)...);
//}
//
//class RevBasic : public _Rev {
//public:
//    // Invalid Rev
//    RevBasic() {}
//    
//    RevBasic(Commit commit) : _commit(commit) {}
//    
//    RevBasic(Ref ref, size_t skip) : _ref(ref), _skip(skip) {
//        _commit = ref.commit();
//        assert(_commit); // Verify assumption: if we have a ref, it points to a valid commit
//    }
//    
//    Commit commit() const override { return _commit; }
//    Ref ref() const override { return _ref; }
//    size_t skip() const override { return _skip; }
//    
//private:
//    Git::Rev _rev;
//
//};
