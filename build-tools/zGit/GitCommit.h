#pragma once

#include <zGit/zGit.h>
#include <zGit/GitSignature.h>

struct git_commit;
struct git_oid;
class GitObjectId;
class GitTree;


// --------------------------------------------------------------------------
// GitCommit
//
// Wraps git_commit: "parsed representation of a commit object."
// --------------------------------------------------------------------------

class ZGIT_API GitCommit
{
public:
    // GitCommit assumes ownership of the git_commit object.
    GitCommit(git_commit& commit) noexcept;
    GitCommit(const GitCommit& rhs) = default;
    GitCommit(GitCommit&& rhs) noexcept;
    ~GitCommit() noexcept;

    GitCommit& operator=(GitCommit&& rhs) noexcept;

    // The comparison compares the commits' object IDs, not the contents of the commits.
    // Two commits with the same message and author details will be considered
    // different if they are not actually the same commit. To compare the contents
    // only, use the Equals method.
    bool operator==(const GitCommit& rhs) const noexcept;
    bool operator!=(const GitCommit& rhs) const noexcept { return !operator==(rhs); }

    // Returns true if the message and author details are the same, regardless of
    // whether the commits' object IDs match.
    bool Equals(const GitCommit& rhs) const noexcept;

    // Returns the non-null git_commit object that GitCommit wraps.
    operator const git_commit*() const noexcept { return m_commit; }

    // Returns the non-null git_oid object referencing the commit.
    operator const git_oid*() const noexcept;

    // Returns a GitObjectId object referencing the commit.
    GitObjectId GetObjectId() const noexcept;

    // Returns the commit message with whitespace trimmed.
    // If a UTF-8 BOM preceeds the message, it is also stripped.
    const std::string& GetMessage() const noexcept;

    // Returns details about the commit's author.
    const GitSignature& GetAuthor() const noexcept;

    // Return the number of parents for this commit.
    unsigned int GetParentCount() const noexcept;

    // Returns the tree for this commit.
    GitTree GetTree() const;

private:
    git_commit* m_commit;
    std::string m_message;
    std::optional<GitSignature> m_author;
};
