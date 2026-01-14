#include "StdAfx.h"
#include "GitTree.h"


GitTree::GitTree(git_tree& tree) noexcept
    :   m_tree(&tree)
{
}


GitTree::GitTree(GitTree&& rhs) noexcept
    :   m_tree(rhs.m_tree)
{
    rhs.m_tree = false;
}


GitTree::~GitTree() noexcept
{
    if( m_tree != nullptr )
        git_tree_free(m_tree);
}


size_t GitTree::GetEntryCount() const noexcept
{
    return git_tree_entrycount(m_tree);
}


GitTreeEntry GitTree::GetEntryByIndex(const size_t index) const
{
    const git_tree_entry* const tree_entry = git_tree_entry_byindex(m_tree, index);

    if( tree_entry == nullptr )
        ThrowGitException();

    return GitTreeEntry(*m_tree, *tree_entry);
}


GitTreeEntry GitTree::GetEntryByPath(const cs::string_sz path) const
{
    ASSERT(strchr(path.c_str(), '\\') == nullptr);

    git_tree_entry* tree_entry;

    switch( git_tree_entry_bypath(&tree_entry, m_tree, path.c_str()) )
    {
        case 0:
            return GitTreeEntry(*m_tree, *tree_entry);

        case GIT_ENOTFOUND:
            throw CSProException("The path was not found in the repository: %s", path.c_str());

        default:
            ThrowGitException();
    }
}
