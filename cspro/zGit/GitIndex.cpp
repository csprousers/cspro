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


GitObjectId GitIndex::GetObjectIdByPath(const cs::string_sz path) const
{
    const git_index_entry* const index_entry = git_index_get_bypath(m_index, path.c_str(), GIT_INDEX_STAGE_NORMAL);

    if( index_entry == nullptr )
        throw GitException("The path was not found in the index: %s", path.c_str());

    return GitObjectId(index_entry->id);
}


std::map<std::string, GitObjectId> GitIndex::GetPathObjectIdMap() const
{
    git_index_iterator* index_iterator;

    if( git_index_iterator_new(&index_iterator, m_index) != 0 )
        throw GitException();

    const RAII::RunOnDestruction free_iterator([&]() { git_index_iterator_free(index_iterator); });

    std::map<std::string, GitObjectId> path_object_id_map;
    const git_index_entry* index_entry;
    int next_result;

    while( ( next_result = git_index_iterator_next(&index_entry, index_iterator) ) == 0 )
    {
        path_object_id_map.try_emplace(index_entry->path, index_entry->id);
    }

    if( next_result != GIT_ITEROVER )
        throw GitException();

    ASSERT(path_object_id_map.size() == GetEntryCount());

    return path_object_id_map;
}


void GitIndex::AddEntry(const GitObjectId& oid, const cs::string_sz path, const uint32_t mode)
{
    git_index_entry index_entry { };

    memcpy(&index_entry.id, static_cast<const git_oid*>(oid), sizeof(git_oid));
    index_entry.path = path.c_str();
    index_entry.mode = mode;

    if( git_index_add(m_index, &index_entry) != 0 )
        throw GitException();
}


void GitIndex::RemoveEntryByPath(const cs::string_sz path)
{
    if( git_index_remove_bypath(m_index, path.c_str()) != 0 )
        throw GitException();
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
