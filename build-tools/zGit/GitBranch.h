#pragma once

#include <zGit/zGit.h>

struct git_reference;
class GitObjectId;


// --------------------------------------------------------------------------
// GitBranch
//
// Wraps git_reference: "in-memory representation of a reference."
// --------------------------------------------------------------------------

class ZGIT_API GitBranch
{
public:
    // GitBranch assumes ownership of the git_reference object.
    GitBranch(git_reference& branch_ref) noexcept;
    GitBranch(const GitBranch& rhs) = delete;
    GitBranch(GitBranch&& rhs) noexcept;
    ~GitBranch() noexcept;

    GitBranch& operator=(GitBranch&& rhs) noexcept;

    // Compares the branch's target IDs.
    bool operator==(const GitBranch& rhs) const noexcept;
    bool operator!=(const GitBranch& rhs) const noexcept { return !operator==(rhs); }

    // Returns the non-null git_reference object that GitBranch wraps.
    operator const git_reference*() const noexcept { return m_branchRef; }

    // Returns the branch's name.
    const std::string& GetName() const noexcept;

    // Returns a local branch's remote tracking branch.
    // Null is returned when the local branch is not tracking a remote branch.
    // An exception is thrown on error (e.g., the branch is not a local branch).
    std::unique_ptr<GitBranch> GetUpstreamBranch() const;

    // Returns the branch's target, throwing an exception if unavailable.
    GitObjectId GetTarget() const;

    // Deletes the branch, throwing an exception on error.
    // Using GitBranch is no longer valid after this operation.
    void Delete();

private:
    git_reference* m_branchRef;
    std::string m_name;
};
