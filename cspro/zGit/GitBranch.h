#pragma once

#include <zGit/zGit.h>

class GitObjectId;
class GitReference;
class GitRepository;


// --------------------------------------------------------------------------
// GitBranch
//
// A wrapper around a branch name, with the branch's target evaluated before
// each use.
// --------------------------------------------------------------------------

class ZGIT_API GitBranch
{
public:
    GitBranch(GitRepository& repo, std::string name) noexcept;
    GitBranch(GitRepository& repo, const GitReference& branch_ref);

    // Compares the branches' names and then target IDs.
    bool operator==(const GitBranch& rhs) const;
    bool operator!=(const GitBranch& rhs) const { return !operator==(rhs); }

    // Returns the branch's name.
    const std::string& GetName() const noexcept { return m_name; }

    // Returns the current branch reference.
    GitReference GetReference() const;

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
    static std::string GetBranchName(const GitReference& branch_ref);

private:
    GitRepository* m_repo;
    std::string m_name;
};
