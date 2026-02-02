#pragma once

#include <zGit/zGit.h>

struct git_index;


// --------------------------------------------------------------------------
// GitIndex
//
// Wraps git_index: "memory representation of an index file."
// --------------------------------------------------------------------------

class ZGIT_API GitIndex
{
public:
    // GitIndex assumes ownership of the git_index object.
    GitIndex(git_index& index) noexcept;
    GitIndex(const GitIndex& rhs) = delete;
    GitIndex(GitIndex&& rhs) noexcept;
    ~GitIndex() noexcept;

    // Returns the non-null git_index object that GitIndex wraps.
    operator const git_index*() const noexcept { return m_index; }
    operator git_index*() noexcept             { return m_index; }

    // Returns the number of entries currently in the index
    size_t GetEntryCount() const noexcept;

    // Returns the path of a file by index.
    // An exception is thrown if the entry cannot be found.
    std::string GetPathByIndex(size_t index) const;

    // Returns the object ID for an entry by path.
    // An exception is thrown if the entry cannot be found.
    GitObjectId GetObjectIdByPath(cs::string_sz path) const;

    // Adds an entry using content previously added as a blob.
    void AddEntry(const GitObjectId& oid, cs::string_sz path, uint32_t mode);

    // Removes an entry from the index by path.
    // No error occurs if the path is not found.
    void RemoveEntryByPath(cs::string_sz path);

    // Adds all non-ignored files (modified and new) in the working directory.
    // The index is updated and then written to the disk.
    void StageAllFilesInWorkingDirectory();

private:
    git_index* m_index;
};
