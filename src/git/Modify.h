#pragma once
#include <fstream>
#include <deque>
#include "Git.h"
#include "Conflict.h"
#include "Editor.h"
#include "lib/toastbox/Defer.h"
#include "lib/toastbox/String.h"

namespace Git {

template <typename T_Rev>
class Modify {
public:
    struct Ctx {
        Repo repo;
        std::function<Ref(const Ref&, const Commit&)> refReplace;
        std::function<void(const char*const*)> spawn;
        std::function<void(const Index&, const std::vector<FileConflict>&)> conflictsResolve;
    };
    
    struct Op {
        enum class Type {
            None,
            Move,
            Copy,
            Delete,
            Combine,
            Edit,
        };
        
        Type type = Type::None;
        
        struct {
            T_Rev rev;
            std::set<Commit> commits; // Commits to be operated on
        } src;
        
        struct {
            T_Rev rev;
            Commit position; // Position in destination after which the commits will be added
        } dst;
    };
    
    struct OpResult {
        struct Res {
            T_Rev rev;
            std::set<Commit> selection;
            std::set<Commit> selectionPrev;
        };
        
        Res src;
        Res dst;
    };
    
    class ConflictResolveCanceled : public std::exception {};
    
private:
    // _Sorted: sorts a set of commits according to the order that they appear via `c`
    static std::vector<Commit> _Sorted(Commit head, const std::set<Commit>& commits) {
        std::vector<Commit> sorted;
        std::set<Commit> rem = commits;
        while (!rem.empty()) {
            if (rem.erase(head)) sorted.push_back(head);
            head = head.parent();
        }
        
        std::reverse(sorted.begin(), sorted.end());
        return sorted;
    }
    
    static Commit _FindEarliestCommit(Commit head, const std::set<Commit>& commits) {
        assert(!commits.empty());
        std::set<Commit> rem = commits;
        while (rem.size() > 1) {
            rem.erase(head);
            head = head.parent();
        }
        return *rem.begin();
    }
    
    static void _ConflictsHandle(const Ctx& ctx, git_merge_file_favor_t fileFavor, const Index& index) {
        if (!index.conflicts()) return;
        
        // We explicitly handle merge conflicts here.
        // Conflicts can still occur even when fileFavor==GIT_MERGE_FILE_FAVOR_THEIRS and we've provided
        // that to, eg, commitParentSet().
        // In that case, we want to iterate over all the conflicts and choose the 'theirs' side.
        // If fileFavor==GIT_MERGE_FILE_FAVOR_NORMAL, we need to let the user decide how to solve the
        // merge conflict.
        const std::vector<FileConflict> fcs = ConflictsGet(ctx.repo, index);
        switch (fileFavor) {
        case GIT_MERGE_FILE_FAVOR_NORMAL:
            ctx.conflictsResolve(index, fcs);
            return;
        case GIT_MERGE_FILE_FAVOR_THEIRS:
            for (const FileConflict& fc : fcs) {
                ConflictResolve(ctx.repo, index, fc, fc.content(FileConflict::Side::Theirs));
            }
            return;
        // Unsupported git_merge_file_favor_t
        default:
            abort();
        }
    }
    
    static Commit _CommitParentSet(const Ctx& ctx, git_merge_file_favor_t fileFavor, const Commit& commit, const Commit& parent) {
        Index index = ctx.repo.commitParentSet(fileFavor, commit, parent);
        _ConflictsHandle(ctx, fileFavor, index);
        return ctx.repo.commitParentSetFinish(index, commit, parent);
    }
    
    static Commit _CommitIntegrate(const Ctx& ctx, git_merge_file_favor_t fileFavor, const Commit& dst, const Commit& src) {
        Index index = ctx.repo.commitIntegrate(fileFavor, dst, src);
        _ConflictsHandle(ctx, fileFavor, index);
        return ctx.repo.commitIntegrateFinish(index, dst, src);
    }
    
    struct _AddRemoveResult {
        Commit commit;
        std::set<Commit> added;
    };
    
    static _AddRemoveResult _AddRemoveCommits(
        const Ctx& ctx,
        git_merge_file_favor_t fileFavor,
        const Commit& dst,
        const std::set<Commit>& add,
        const Commit& addSrc, // Source of `add` commits (to derive their order)
        const Commit& addPosition, // In `dst`
        const std::set<Commit>& remove
    ) {
        // CommitAdded: wraps a Commit with an additional `added` flag, which tracks whether
        // this is one of the added commits, so that we can put the commit in our returned
        // _AddRemoveResult.added set
        struct CommitAdded {
            CommitAdded(Commit c) : commit(c) {}
            Commit commit;
            bool added = false;
        };
        
        assert(dst);
        
        std::vector<Commit> addv = _Sorted(addSrc, add);
        
        // Construct `combined` and find `head`
        // `head` is the earliest commit from `dst` that we'll apply the commits on top of.
        // `combined` is the ordered set of commits that need to be applied on top of `head`.
        std::deque<CommitAdded> combined;
        Commit head;
        {
            const bool adding = !addv.empty();
            std::set<Commit> r = remove;
            Commit c = dst;
            bool foundAddPoint = false;
            for (;;) {
                if (adding && c==addPosition) {
                    assert(!foundAddPoint);
                    combined.insert(combined.begin(), addv.begin(), addv.end());
                    for (size_t i=0; i<addv.size(); i++) combined[i].added = true;
                    foundAddPoint = true;
                }
                
                if (r.empty() && (!adding || foundAddPoint)) break;
                // The location of this c!=null assertion is important:
                // We explicitly allow adding commits before the root commit, and also removing
                // the root commit itself, which requires us to allow c==null for a single
                // iteration of this loop.
                // Therefore this check needs to occur after our break (above). So if we get to
                // this point and we didn't break, but c==null, then we exhausted our one
                // c==null iteration and have a problem.
                assert(c);
                if (!r.erase(c)) combined.push_front(c);
                c = c.parent(); // TODO:MERGE
            }
            assert(!adding || foundAddPoint);
            assert(r.empty());
            
            head = c;
        }
        
        // Apply `combined` on top of `head`, and keep track of the added commits
        std::set<Commit> added;
        for (const CommitAdded& commit : combined) {
            // TODO:MERGE
            head = _CommitParentSet(ctx, fileFavor, commit.commit, head);
            
            if (commit.added) {
                added.insert(head);
            }
        }
        
        return {
            .commit = head,
            .added = added,
        };
    }
    
    static bool _CommitsHasGap(const Commit& head, const std::set<Commit>& commits) {
        Commit h = head;
        std::set<Commit> rem = commits;
        bool started = false;
        while (h && !rem.empty()) {
            bool found = rem.erase(h);
            if (started && !found) return true;
            started |= found;
            h = h.parent();
        }
        // If `rem` still has elements, then it's because `commits` contains
        // elements that don't exist in the `head` tree
        assert(rem.empty());
        return false;
    }
    
    static bool _CommitsHasMerge(const std::set<Commit>& commits) {
        for (const Commit& commit : commits) {
            if (commit.isMerge()) return true;
        }
        return false;
    }
    
    static bool _InsertionIsNop(const Commit& head, const Commit& position, const std::set<Commit>& commits) {
        std::set<Commit> positions = commits;
        const Commit tail = _FindEarliestCommit(head, positions);
        // tail.parent()==nullptr is OK and desired, because it represents insertion as the root commit
        positions.insert(tail.parent());
        return positions.find(position) != positions.end();
    }
    
    static bool _MoveIsNop(const Op& op) {
        assert(op.src.rev.ref);
        assert(op.dst.rev.ref);
        
        // Not a nop if the source and destination are different
        if (op.src.rev.ref != op.dst.rev.ref) return false;
        // If there's a gap in the commits that are being moved, then it's
        // never a nop, because moving them would remove the gap
        if (_CommitsHasGap(op.src.rev.commit, op.src.commits)) return false;
        return _InsertionIsNop(op.src.rev.commit, op.dst.position, op.src.commits);
    }
    
    static git_merge_file_favor_t _FileFavor(const T_Rev& src, const T_Rev& dst) {
        // Required arguments:
        assert(src);
        // If there's only one rev, then use _THEIRS (conflicts not allowed)
        if (!dst) return GIT_MERGE_FILE_FAVOR_THEIRS;
        // If the two revs have refs and they're the same ref, use _THEIRS (conflicts not allowed)
        if (src.ref && dst.ref && dst.ref==src.ref) return GIT_MERGE_FILE_FAVOR_THEIRS;
        // Otherwise, use _NORMAL (conflicts allowed)
        return GIT_MERGE_FILE_FAVOR_NORMAL;
    }
    
    static std::optional<OpResult> _MoveCommits(const Ctx& ctx, const Op& op) {
        // Required arguments:
        assert(op.src.rev);
        assert(op.dst.rev);
        
        // When moving commits, the source and destination must be refs (branches
        // or tags) since we're modifying both
        if (!op.src.rev.ref) throw RuntimeError("source must be a reference (branch or tag)");
        if (!op.dst.rev.ref) throw RuntimeError("destination must be a reference (branch or tag)");
        
        if (_MoveIsNop(op)) return std::nullopt;
        
        // Move commits within the same ref (branch/tag)
        if (op.src.rev.ref == op.dst.rev.ref) {
            // References are the same, so the commits must be the same too
            assert(op.src.rev.commit == op.dst.rev.commit);
            
            // Add and remove commits
            _AddRemoveResult srcDstResult = _AddRemoveCommits(
                ctx,
                _FileFavor(op.src.rev, op.dst.rev),
                op.dst.rev.commit,  // dst:         Commit
                op.src.commits,     // add:         std::set<Commit>
                op.src.rev.commit,  // addSrc:      Commit
                op.dst.position,    // addPosition: Commit
                op.src.commits      // remove:      std::set<Commit>
            );
            
            // Replace the source branch/tag
            ctx.refReplace(op.src.rev.ref, srcDstResult.commit);
            // srcRev/dstRev: we're not using the result from revReplace() as the OpResult src/dst revs,
            // and instead use repo.revReload() to get the new revs. This is to handle the case where
            // we're moving commits between eg master and master~4. In this case, the revs are different
            // even though the underlying refs are the same, so we can't use the same rev for both
            // OpResult.src.rev and OpResult.dst.rev.
            T_Rev srcRev = op.src.rev;
            T_Rev dstRev = op.dst.rev;
            (Rev&)srcRev = ctx.repo.revReload(srcRev);
            (Rev&)dstRev = ctx.repo.revReload(dstRev);
            return OpResult{
                .src = {
                    .rev = srcRev,
                    .selection = srcDstResult.added,
                    .selectionPrev = op.src.commits,
                },
                .dst = {
                    .rev = dstRev,
                    .selection = srcDstResult.added,
                    .selectionPrev = op.src.commits,
                },
            };
        
        // Move commits between different refs (branches/tags)
        } else {
            // Remove commits from `op.src`
            _AddRemoveResult srcResult = _AddRemoveCommits(
                ctx,
                _FileFavor(op.src.rev, op.dst.rev),
                op.src.rev.commit,  // dst:         Commit
                {},                 // add:         std::set<Commit>
                nullptr,            // addSrc:      Commit
                nullptr,            // addPosition: Commit
                op.src.commits      // remove:      std::set<Commit>
            );
            
            // Add commits to `op.dst`
            _AddRemoveResult dstResult = _AddRemoveCommits(
                ctx,
                _FileFavor(op.src.rev, op.dst.rev),
                op.dst.rev.commit,  // dst:         Commit
                op.src.commits,     // add:         std::set<Commit>
                op.src.rev.commit,  // addSrc:      Commit
                op.dst.position,    // addPosition: Commit
                {}                  // remove:      std::set<Commit>
            );
            
            // Replace the source and destination branches/tags
            T_Rev srcRev = op.src.rev;
            T_Rev dstRev = op.dst.rev;
            (Rev&)srcRev = ctx.refReplace(srcRev.ref, srcResult.commit);
            (Rev&)dstRev = ctx.refReplace(dstRev.ref, dstResult.commit);
            return OpResult{
                .src = {
                    .rev = srcRev,
                    .selection = {},
                    .selectionPrev = op.src.commits,
                },
                .dst = {
                    .rev = dstRev,
                    .selection = dstResult.added,
                    .selectionPrev = {},
                },
            };
        }
    }
    
    static std::optional<OpResult> _CopyCommits(const Ctx& ctx, const Op& op) {
        // Required arguments:
        assert(op.src.rev);
        assert(op.dst.rev);
        
        if (!op.dst.rev.ref) throw RuntimeError("destination must be a reference (branch or tag)");
        
        // Add commits to `op.dst`
        _AddRemoveResult dstResult = _AddRemoveCommits(
            ctx,
            _FileFavor(op.src.rev, op.dst.rev),
            op.dst.rev.commit,  // dst:         Commit
            op.src.commits,     // add:         std::set<Commit>
            op.src.rev.commit,  // addSrc:      Commit
            op.dst.position,    // addPosition: Commit
            {}                  // remove:      std::set<Commit>
        );
        
        // Replace the destination branch/tag
        T_Rev dstRev = op.dst.rev;
        (Rev&)dstRev = ctx.refReplace(dstRev.ref, dstResult.commit);
        return OpResult{
            .src = {
                .rev = op.src.rev,
            },
            .dst = {
                .rev = dstRev,
                .selection = dstResult.added,
                .selectionPrev = {},
            },
        };
    }
    
    static std::optional<OpResult> _DeleteCommits(const Ctx& ctx, const Op& op) {
        // Required arguments:
        assert(op.src.rev);
        // Illegal arguments:
        assert(!op.dst.rev);
        
        if (!op.src.rev.ref) throw RuntimeError("source must be a reference (branch or tag)");
        
        // Remove commits from `op.src`
        _AddRemoveResult srcResult = _AddRemoveCommits(
            ctx,
            _FileFavor(op.src.rev, op.dst.rev),
            op.src.rev.commit,  // dst:         Commit
            {},                 // add:         std::set<Commit>
            nullptr,            // addSrc:      Commit
            nullptr,            // addPosition: Commit
            op.src.commits      // remove:      std::set<Commit>
        );
        
        if (!srcResult.commit) {
            throw RuntimeError("can't delete last commit");
        }
        
        // Replace the source branch/tag
        T_Rev srcRev = op.src.rev;
        (Rev&)srcRev = ctx.refReplace(srcRev.ref, srcResult.commit);
        return OpResult{
            .src = {
                .rev = srcRev,
                .selection = {},
                .selectionPrev = op.src.commits,
            },
        };
    }
    
    static std::optional<OpResult> _CombineCommits(const Ctx& ctx, const Op& op) {
        // Required arguments:
        assert(op.src.rev);
        // Illegal arguments:
        assert(!op.dst.rev);
        
        if (!op.src.rev.ref) throw RuntimeError("source must be a reference (branch or tag)");
        if (op.src.commits.size() < 2) throw RuntimeError("at least 2 commits are required to combine");
        if (_CommitsHasMerge(op.src.commits)) throw RuntimeError("can't combine with merge commit");
        
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
                head = head.parent(); // TODO:MERGE
            }
        }
        
        // Combine `head` with all the commits in `integrate`
        for (const Commit& commit : integrate) {
            head = _CommitIntegrate(ctx, _FileFavor(op.src.rev, op.dst.rev), head, commit);
        }
        
        // Remember the final commit containing all the integrated commits
        Commit integrated = head;
        
        // Attach every commit in `attach` to `head`
        for (const Commit& commit : attach) {
            // TODO:MERGE
            head = _CommitParentSet(ctx, _FileFavor(op.src.rev, op.dst.rev), commit, head);
        }
        
        // Replace the source branch/tag
        T_Rev srcRev = op.src.rev;
        (Rev&)srcRev = ctx.refReplace(srcRev.ref, head);
        return OpResult{
            .src = {
                .rev = srcRev,
                .selection = {integrated},
                .selectionPrev = op.src.commits,
            },
        };
    }
    
    static std::string _Trim(std::string_view str) {
        std::string x(str);
        x.erase(x.find_last_not_of(' ')+1);
        x.erase(0, x.find_first_not_of(' '));
        return x;
    }
    
    struct _CommitAuthor {
        std::string name;
        std::string email;
        bool operator ==(const _CommitAuthor& x) const {
            return name==x.name && email==x.email;
        }
    };
    
    struct _CommitMessage {
        std::optional<_CommitAuthor> author;
        std::optional<Time> time;
        std::string message;
        
        bool operator ==(const _CommitMessage& x) const {
            return author==x.author &&
                   time==x.time &&
                   message==x.message;
        }
    };
    
    static std::optional<_CommitAuthor> _CommitAuthorParse(std::string_view str) {
        size_t emailStart = str.find_first_of('<');
        if (emailStart == std::string::npos) return std::nullopt;
        size_t emailEnd = str.find_last_of('>');
        if (emailEnd == std::string::npos) return std::nullopt;
        if (emailStart >= emailEnd) return std::nullopt;
        
        std::string name = _Trim(str.substr(0, emailStart));
        if (name.empty()) return std::nullopt;
        
        std::string email = _Trim(str.substr(emailStart+1, emailEnd-emailStart-1));
        if (email.empty()) return std::nullopt;
        
        return _CommitAuthor{
            .name = name,
            .email = email,
        };
    }
    
    static std::optional<Time> _CommitTimeParse(const std::string& str) {
        try {
            return TimeFromString(str);
        } catch (...) {
            return std::nullopt;
        }
    }
    
    static constexpr const char _AuthorPrefix[] = "Author:";
    static constexpr const char _TimePrefix[]   = "Date:";
    
    static _CommitMessage _CommitMessageForCommit(Commit commit) {
        const git_signature* sig = git_commit_author(*commit);
        return _CommitMessage{
            .author  = _CommitAuthor{sig->name, sig->email},
            .time    = TimeForGitTime(sig->when),
            .message = git_commit_message(*commit),
        };
    }
    
    static std::string _StringFromCommitMessage(const _CommitMessage& msg) {
        assert(msg.author);
        assert(msg.time);
        
        std::stringstream ss;
        ss << _AuthorPrefix << " " << msg.author->name << " <" << msg.author->email << ">" << '\n';
        ss << _TimePrefix << "   " << StringFromTime(*msg.time) << '\n';
        ss << '\n';
        ss << msg.message;
        return ss.str();
    }
    
    static _CommitMessage _CommitMessageFromString(std::string_view str) {
        // Read back the file
        const std::vector<std::string> lines = String::Split(str, "\n");
        auto iter = lines.begin();
        
        // Find author string
        std::optional<std::string> authorStr;
        for (; !authorStr && iter!=lines.end(); iter++) {
            std::string line = _Trim(*iter);
            if (Toastbox::String::StartsWith(_AuthorPrefix, line)) {
                authorStr = _Trim(line.substr(std::size(_AuthorPrefix)-1));
            }
            else if (!line.empty()) break;
        }
        
        // Find time string
        std::optional<std::string> timeStr;
        for (; !timeStr && iter!=lines.end(); iter++) {
            std::string line = _Trim(*iter);
            if (Toastbox::String::StartsWith(_TimePrefix, line)) {
                timeStr = _Trim(line.substr(std::size(_TimePrefix)-1));
            }
            else if (!line.empty()) break;
        }
        
        // Skip whitespace until message starts
        for (; iter!=lines.end(); iter++) {
            std::string line = _Trim(*iter);
            if (!line.empty()) break;
        }
        
        // Re-compose message after skipping author+time+whitspace
        std::string msg;
        for (; iter!=lines.end(); iter++) {
            msg += (msg.empty() ? *iter : "\n" + *iter);
        }
        
        std::optional<_CommitAuthor> author;
        std::optional<Time> time;
        
        if (authorStr) author = _CommitAuthorParse(*authorStr);
        if (timeStr) time = _CommitTimeParse(*timeStr);
        
        return _CommitMessage{
            .author = author,
            .time = time,
            .message = msg,
        };
    }
    
    static std::optional<OpResult> _EditCommit(const Ctx& ctx, const Op& op) {
        // Required arguments:
        assert(op.src.rev);
        assert(op.src.commits.size() == 1); // Programmer error
        // Illegal arguments:
        assert(!op.dst.rev);
        
        if (!op.src.rev.ref) throw RuntimeError("source must be a reference (branch or tag)");
        
        // Write the commit message to the file
        const Commit origCommit = *op.src.commits.begin();
        const git_signature* origAuthor = git_commit_author(*origCommit);
        const _CommitMessage origMsg = _CommitMessageForCommit(origCommit);
        
        // _CommitMessage -> String
        const std::string origMsgStr = _StringFromCommitMessage(origMsg);
        
        // Execute the editor and read back the result
        const std::string newMsgStr = EditorRun(ctx.repo, ctx.spawn, origMsgStr);
        
        // String -> _CommitMessage
        const _CommitMessage newMsg = _CommitMessageFromString(newMsgStr);
        
        // Nop if the message wasn't changed
        if (origMsg == newMsg) return std::nullopt;
        
        // Construct a new signature, using the original signature values if a new value didn't exist
        const char* newName = newMsg.author ? newMsg.author->name.c_str() : origAuthor->name;
        const char* newEmail = newMsg.author ? newMsg.author->email.c_str() : origAuthor->email;
        time_t newTime = newMsg.time ? newMsg.time->time : origAuthor->when.time;
        int newOffset = newMsg.time ? newMsg.time->offset : origAuthor->when.offset;
        const Signature newAuthor = Signature::Create(newName, newEmail, newTime, newOffset);
        const Commit newCommit = ctx.repo.commitAmend(origCommit, newAuthor, newMsg.message);
        
        // Rewrite the rev
        // Add and remove commits
        _AddRemoveResult srcResult = _AddRemoveCommits(
            ctx,
            _FileFavor(op.src.rev, op.dst.rev),
            op.src.rev.commit,  // dst:         Commit
            {newCommit},        // add:         std::set<Commit>
            newCommit,          // addSrc:      Commit
            origCommit,         // addPosition: Commit
            {origCommit}        // remove:      std::set<Commit>
        );
        
        // Replace the source branch/tag
        T_Rev srcRev = op.src.rev;
        (Rev&)srcRev = ctx.refReplace(srcRev.ref, srcResult.commit);
        return OpResult{
            .src = {
                .rev = srcRev,
                .selection = srcResult.added,
                .selectionPrev = op.src.commits,
            },
        };
    }
    
public:
    static std::optional<OpResult> Exec(const Ctx& ctx, const Op& op) {
        try {
            switch (op.type) {
            case Op::Type::None:    return std::nullopt;
            case Op::Type::Move:    return _MoveCommits(ctx, op);
            case Op::Type::Copy:    return _CopyCommits(ctx, op);
            case Op::Type::Delete:  return _DeleteCommits(ctx, op);
            case Op::Type::Combine: return _CombineCommits(ctx, op);
            case Op::Type::Edit:    return _EditCommit(ctx, op);
            }
            abort();
        
        } catch (const ConflictResolveCanceled&) {
            // Conflict resolution was canceled
            return std::nullopt;
        
        } catch (...) {
            throw;
        }
    }

}; // class Modify

} // namespace Git
