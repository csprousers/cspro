#pragma once

#include "RepoFilePath.h"


// --------------------------------------------------------------------------
// LibraryManager
// --------------------------------------------------------------------------

class LibraryManager
{
public:
    struct Build;
    struct Input;

    // Build data is read from the settings database, with exceptions ignored.
    LibraryManager(Controller& controller) noexcept;

    // Returns the builds currently in the settings database.
    // The builds are ordered by tag name in reverse order.
    // The value is returned as a shared pointer so that ManageLibrariesView::OnUpdateLibraryIds
    // can have a copy while the builds are potentially refreshed.
    std::shared_ptr<const std::vector<Build>> GetBuilds() const noexcept { return m_builds; }

    // Reads tags from the open source libraries repository, extracting the
    // library ID from the the comment in a tag's commit.
    void RefreshBuildsFromTags();

    // Returns the paths of files on the disk that are included in a built library.
    const std::vector<RepoFilePath>& GetInputs();

    // Returns information about the inputs that are part of a built library
    // at the specified commit.
    std::vector<Input> GetInputs(const GitCommit& cs_commit);

    // Returns a cache key for the built libraries. The local version is only valid locally,
    // as it uses file times and is only a shortcut to access the actual library ID.
    std::string CalculateCacheKey(const std::vector<Input>& inputs, bool local_version);

    // Creates a commit in the open source libraries repository with the
    // built libraries at the specified commit.
    void CreateAndCommitBuild(const GitCommit& cs_commit);

    // Returns the tag name in the open source libraries repository that contains
    // the built libraries at the given commit. An exception is thrown when it does not exist.
    std::string GetTagForBuiltLibraries(const GitCommit& cs_commit);

private:
    std::unique_ptr<std::vector<Build>> LoadCachedBuilds() const;
    void CacheBuilds(const std::vector<Build>& builds) const;

private:
    Controller& m_controller;

    std::shared_ptr<const std::vector<Build>> m_builds;

    std::vector<RepoFilePath> m_inputRepoFilePaths;

    struct FileData;
    std::map<Input, std::shared_ptr<FileData>> m_fileData;
};


// --------------------------------------------------------------------------
// LibraryManager::Build
// --------------------------------------------------------------------------

struct LibraryManager::Build
{
    std::string tag_name;
    std::string library_id;
    std::string local_hash;
};


// --------------------------------------------------------------------------
// LibraryManager::Input
// --------------------------------------------------------------------------

struct LibraryManager::Input
{
    RepoFilePath repo_file_path;
    std::optional<GitObjectId> cs_blob_oid;
};

bool operator<(const LibraryManager::Input& input1, const LibraryManager::Input& input2) noexcept;
