#include "StdAfx.h"
#include "GitTreeEntry.h"


GitTreeEntry::GitTreeEntry(const git_tree& tree, const git_tree_entry& tree_entry) noexcept
    :   m_tree(&tree),
        m_treeEntry(&tree_entry)
{
}


std::string GitTreeEntry::GetName() const noexcept
{
    return git_tree_entry_name(m_treeEntry);
}


GitObjectType GitTreeEntry::GetType() const noexcept
{
    const git_object_t type = git_tree_entry_type(m_treeEntry);
    ASSERT(type >= GIT_OBJECT_COMMIT || type <= GIT_OBJECT_TAG);
    return static_cast<GitObjectType>(type);
}


GitObject GitTreeEntry::GetObject() const
{
    git_object* object;

    if( git_tree_entry_to_object(&object, git_tree_owner(m_tree), m_treeEntry) != 0 )
        ThrowGitException();

    return GitObject(*object);
}
