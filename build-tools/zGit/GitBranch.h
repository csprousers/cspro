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
    GitBranch(const GitBranch& rhs) = default;
    GitBranch(GitBranch&& rhs) noexcept;
    ~GitBranch() noexcept;

    // Returns the non-null git_reference object that GitBranch wraps.
    operator const git_reference*() const noexcept { return m_branchRef; }

    // Returns the branch's name.
    const std::string& GetName() const noexcept;

    // Returns the branch's target, throwing an exception if unavailable.
    GitObjectId GetTarget() const;

private:
    git_reference* m_branchRef;
    std::string m_name;
};
