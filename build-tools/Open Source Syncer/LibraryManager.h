#pragma once


// --------------------------------------------------------------------------
// LibraryManager
// --------------------------------------------------------------------------

class LibraryManager
{
public:
    struct FileHash;
    struct LibrariesData;
    struct LibraryVersion;

    // Build data is read from the settings database, with exceptions ignored.
    LibraryManager(Controller& controller) noexcept;
    ~LibraryManager();

    // Returns an object for interacting with the csprousers/cspro-libraries-third-party repository on GitHub.
    GitHubRepositoryConnection& GetThirdPartyLibrariesGitHubRepositoryConnection();

    // Updates all library data, or the data for a specified number of library types.
    LibrariesData UpdateLibrariesData(bool save_library_data);
    LibrariesData UpdateLibrariesData(const std::vector<std::string>& library_types, bool save_library_data);

    // Returns the file paths of all files that are part of the CSPro build but that are
    // not committed to the repository
    std::set<std::string> GetThirdPartyFilePaths();

    // Returns the libraries ID for the library data, returning:
    // - the value saved in third_party/prebuilt/libraries.json
    // - a freshly calculated value from the libraries in the working directory
    // - the value saved in third_party/prebuilt/libraries.json at a specific commit
    enum class LibrariesIdType { InLibrariesJson, CalculatedFromWorkingDirectory };
    std::string GetLibrariesId(LibrariesIdType libraries_id_type);
    std::string GetLibrariesId(const GitCommit& cs_commit);

    // Returns the release tag associated with the libraries ID.
    // An exception is thrown if the release does not exist on GitHub.
    std::string GetLibrariesReleaseTag(const std::string& libraries_id);

    // Creates a tag and then a release on GitHub for the working directory libraries.
    void CreateLibraryRelease();

private:
    // Returns a path from a repository path.
    std::string CreatePathFromRepoPath(std::string repo_file_path) const;

    // Returns a repository path from a file or directory path.
    std::string CreateRepoPathFromPath(const std::string& path) const;

    // SHA-256 hashes of files will be cached and stored in the settings database.
    void LoadCachedFileHashes();
    void SaveCachedFileHashes() const;
    std::vector<FileHash> CalculateFileHashes(const std::vector<std::string>& file_paths);

    // Reads the current library data saved in third_party/prebuilt/libraries.json.
    LibrariesData LoadLibrariesData() const;

    // Parses the library data.
    LibrariesData ParseLibrariesData(const JsonNode& json_node) const;

    // Saves the library data.
    void SaveLibrariesData(const LibrariesData& libraries_data) const;

    // Updates the libraries ID, a hash of the repository paths and SHA-256 hashes for each target.
    void CalculateLibrariesId(LibrariesData& libraries_data) const;

    // Reads the version numbers of prebuilt libraries, reading these from docs/external-libraries.md.
    std::vector<LibraryVersion> ReadLibraryVersions() const;

    // Returns the file paths of all files that are part of the CSPro build but that are
    // not committed to the repository for the specified library type.
    std::vector<std::string> GetLibraryTargetFilePaths(const std::string& library_type, const std::string& platform) const;

    // Returns the file paths of the files in third_party/prebuilt's bin and lib
    // directories for the specified library type.
    std::vector<std::string> GetPrebuiltLibraryFilePaths(const std::string& library_type) const;

    // Creates .zip files for each library type, returning the filename and the .zip file data.
    std::vector<std::tuple<std::string, std::shared_ptr<const BinaryBlock>>> CreateReleaseAssets(
        const LibrariesData& libraries_data, const std::string& tag_name) const;

    // Creates release notes for the GitHub release.
    std::string CreateReleaseNotes(const std::string& libraries_id, const std::string& libraries_commit_oid_hash) const;

private:
    Controller& m_controller;
    std::string m_librariesDataFilePath;
    std::vector<FileHash> m_fileHashes;
    std::unique_ptr<GitHubRepositoryConnection> m_ghConnection;
};


// --------------------------------------------------------------------------
// LibraryManager::FileHash
// --------------------------------------------------------------------------

struct LibraryManager::FileHash
{
    std::string file_path;
    std::string sha256;
    int64_t file_size;
    int64_t file_modified_time;
};


// --------------------------------------------------------------------------
// LibraryManager::LibrariesData
// --------------------------------------------------------------------------

struct LibraryManager::LibrariesData
{
    struct Target
    {
        std::string library_type;
        std::string platform;
        std::string architecture;
        std::vector<FileHash> files;
    };

    std::string id;
    std::vector<LibraryVersion> library_versions;
    std::vector<Target> targets;
};


// --------------------------------------------------------------------------
// LibraryManager::LibraryVersion
// --------------------------------------------------------------------------

struct LibraryManager::LibraryVersion
{
    std::string name;
    std::string version;
};
