#pragma once

#include <zGit/zGit.h>
#include <zGit/GitCommit.h>
#include <zGit/GitObjectId.h>
#include <zGit/GitRepository.h>

struct git_revwalk;


// --------------------------------------------------------------------------
// GitRevisionWalker
//
// Wraps git_revwalk: "representation of an in-progress walk through the
// commits in a repo."
// --------------------------------------------------------------------------

class ZGIT_API GitRevisionWalker
{
public:
    GitRevisionWalker(GitRepository& repo);
    GitRevisionWalker(const GitRevisionWalker& rhs) = delete;
    GitRevisionWalker(GitRevisionWalker&& rhs) = delete;
    ~GitRevisionWalker() noexcept;

    // Executes the callback function for each commit.
    // Commits are walked in topological and time order, starting at the head.
    // The callback function should return true to continue processing.
    void WalkFromHead(const std::function<bool(GitCommit)>& callback_function);

    // Executes the callback function for each commit.
    // Commits are walked in topological and time order, starting at the head, ending with end_commit.
    void WalkFromHead(const GitCommit& end_commit, const std::function<void(GitCommit)>& callback_function);

    // Executes the callback function for each commit.
    // Commits are walked in topological and time order, starting at the specified commit.
    // If end_commit is provided, "end" means the "oldest" commit.
    // When applicable, the callback function should return true to continue processing.
    void Walk(const GitObjectId& start_oid, const std::function<bool(GitCommit)>& callback_function);
    void Walk(const GitCommit& start_commit, const std::function<bool(GitCommit)>& callback_function);
    void Walk(const GitCommit& start_commit, const GitCommit& end_commit, const std::function<void(GitCommit)>& callback_function);

    // Returns the commits walked using the corresponding WalkFromHead methods.
    std::vector<GitCommit> GetCommitsFromHead();
    std::vector<GitCommit> GetCommitsFromHead(const GitCommit& end_commit);

private:
    GitRepository& m_repo;
    git_revwalk* m_walker;
};
