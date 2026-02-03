#pragma once

#include <zGit/zGit.h>
#include <zGit/GitTreeEntry.h>

struct git_tree;
class GitIndex;


// --------------------------------------------------------------------------
// GitTree
//
// Wraps git_tree: "representation of a tree object."
// --------------------------------------------------------------------------

class ZGIT_API GitTree
{
public:
    // GitTree assumes ownership of the git_tree object.
    GitTree(git_tree& tree) noexcept;
    GitTree(const GitTree& rhs);
    GitTree(GitTree&& rhs) noexcept;
    ~GitTree() noexcept;

    GitTree& operator=(GitTree&& rhs) noexcept;

    // Returns the non-null git_tree object that GitTree wraps.
    operator const git_tree*() const noexcept { return m_tree; }
    operator git_tree*() noexcept             { return m_tree; }

    // Returns the number of entries listed in the tree.
    size_t GetEntryCount() const noexcept;

    // Returns a tree entry by index.
    // An exception is thrown if the entry cannot be found.
    GitTreeEntry GetEntryByIndex(size_t index) const;

    // Returns a tree entry by path.
    // An exception is thrown if the entry cannot be found.
    GitTreeEntry GetEntryByPath(cs::string_sz path) const;

    // Returns an in-memory index with the contents of the tree.
    GitIndex GetIndex() const;

private:
    git_tree* m_tree;
};
