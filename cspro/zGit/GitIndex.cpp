#include "StdAfx.h"
#include "GitIndex.h"


GitIndex::GitIndex(git_index& index) noexcept
    :   m_index(&index)
{
}


GitIndex::GitIndex(GitIndex&& rhs) noexcept
    :   m_index(rhs.m_index)
{
    rhs.m_index = false;
}


GitIndex::~GitIndex() noexcept
{
    if( m_index != nullptr )
        git_index_free(m_index);
}


size_t GitIndex::GetEntryCount() const noexcept
{
    return git_index_entrycount(m_index);
}


std::string GitIndex::GetPathByIndex(const size_t index) const
{
    const git_index_entry* const index_entry = git_index_get_byindex(m_index, index);

    if( index_entry == nullptr )
        throw GitException();

    return index_entry->path;
}


void GitIndex::StageAllFilesInWorkingDirectory()
{
    const git_strarray pathspec = { nullptr, 0 };

    if( git_index_add_all(m_index, &pathspec, GIT_INDEX_ADD_DEFAULT, nullptr, nullptr) != 0 ||
        git_index_write(m_index) != 0 )
    {
        throw GitException();
    }
}
