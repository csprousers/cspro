#pragma once

#include <zGit/zGit.h>
#include <zGit/GitObject.h>

struct git_tree;
struct git_tree_entry;


// --------------------------------------------------------------------------
// GitTreeEntry
//
// Wraps git_tree_entry: "representation of each one of the entries in a tree
// object."
// --------------------------------------------------------------------------

class ZGIT_API GitTreeEntry
{
public:
    GitTreeEntry(const git_tree& tree, const git_tree_entry& tree_entry) noexcept;

    // Returns the entry's name.
    std::string GetName() const noexcept;

    // Returns the entry's type.
    GitObjectType GetType() const noexcept;

    // Returns the entry's object, throwing an exception on error.
    GitObject GetObject() const;

private:
    const git_tree* m_tree;
    const git_tree_entry* m_treeEntry;
};
