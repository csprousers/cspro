#pragma once

#include <zGit/zGit.h>
#include <zGit/GitInitializer.h>
#include <zGit/GitObjectId.h>

struct git_repository;
class GitBranch;
class GitCommit;
class GitIndex;
enum class GitObjectType;
class GitTag;


// --------------------------------------------------------------------------
// GitRepository
//
// Wraps git_repository: "representation of an existing git repository,
// including all its object contents."
//
// Exceptions are thrown on error.
// --------------------------------------------------------------------------

class ZGIT_API GitRepository
{
public:
    GitRepository() noexcept;
    GitRepository(const GitRepository& rhs) = delete;
    GitRepository(GitRepository&& rhs) noexcept;
    ~GitRepository() noexcept;

    // Returns the non-null git_repository object that GetRepository wraps.
    operator git_repository*() noexcept             { return m_repo; }
    operator const git_repository*() const noexcept { return m_repo; }


    // --------------------------------------------------------------------------
    // Open + Close + Information
    // --------------------------------------------------------------------------

    // Opens a repository.
    void Open(std::string repo_directory, bool bare = false) { Open(std::move(repo_directory), false, bare); }

    // Opens a repository in bare mode (with no working directory).
    void OpenBare(std::string repo_directory) { Open(std::move(repo_directory), false, true); }

    // Creates a new repository.
    void Create(std::string repo_directory, bool bare) { Open(std::move(repo_directory), true, bare); }

    // Closes a repository.
    void Close() noexcept;

    // Returns the repository's working directory.
    // The directory is returned using native slashes, with a trailing slash.
    // A blank string is returned if no repository is open, or if the repository
    // was opened in bare mode.
    std::string GetWorkingDirectory() const noexcept;


    // --------------------------------------------------------------------------
    // Branches
    // --------------------------------------------------------------------------

    // Returns the branch pointed to by HEAD.
    GitBranch GetCurrentBranch() const;

    // Looks up the branch, throwing an exception if not found.
    GitBranch LookupBranch(cs::string_sz branch_name) const;

    // Creates a new branch, throwing an exception on error (e.g., if a branch with
    // the name already exists). This does not change the current branch.
    GitBranch CreateBranch(cs::string_sz branch_name, const GitCommit& commit) const;

    // Executes the callback function for each of the repository's local branches.
    // The callback function should return true to continue processing.
    void ForeachLocalBranch(const std::function<bool(GitBranch)>& callback_function) const;

    // Resets the current branch to the commit using the mode "mixed."
    void ResetBranchMixed(const GitCommit& commit) const;


    // --------------------------------------------------------------------------
    // Indices + Statuses + Differences
    // --------------------------------------------------------------------------

    // Returns the repository's index file.
    GitIndex GetIndex() const;

    // Returns the status of a file by path.
    // An exception is thrown if the entry cannot be found.
    // Status codes are in status.h.
    unsigned int GetStatusByPath(cs::string_sz path) const;

    // Executes the callback function for each file in the index, passing the
    // path and status code. These are paths that Git is tracking: "the index
    // (or 'cache', or 'staging area') is the contents of the next commit."
    // Status codes are in status.h.
    void ForeachStatusInIndex(const std::function<void(std::string path, unsigned int status_flags)>& callback_function) const;

    // Executes the callback function for each file in the working directory with
    // a different status from the index, passing the path and status code. The
    // files could be new (untracked), modified, deleted, etc. These are paths that
    // are different "based on [an] index to working directory comparison."
    // Status codes are in status.h.
    void ForeachStatusInWorkingDirectory(const std::function<void(std::string path, unsigned int status_flags)>& callback_function) const;

    // Executes the callback function for each file in the working directory with
    // a different status from the commit's tree, passing the path and difference code.
    // Difference codes are in diff.h.
    void ForeachDifferenceInWorkingDirectory(const GitCommit& commit, const std::function<void(std::string path, unsigned int diff_flags)>& callback_function) const;


    // --------------------------------------------------------------------------
    // Objects
    // --------------------------------------------------------------------------

    // Looks up the object, potentially only of a certain type, throwing an
    // exception if not found.
    GitObject LookupObject(const GitObjectId& oid, GitObjectType type) const;
    GitObject LookupObject(const GitObjectId& oid) const;


    // --------------------------------------------------------------------------
    // Commits
    // --------------------------------------------------------------------------

    // Looks up the commit, throwing an exception if not found.
    // Hex hashes can be provided in both short or long forms.
    GitCommit LookupCommit(const GitObjectId& oid) const;
    GitCommit LookupCommit(cs::string_sz hex_hash) const;

    // Returns the commit associated with the tag, throwing an exception on error.
    GitCommit LookupCommit(const GitTag& tag) const;

    // Returns true if the commit "is the descendant of another commit."
    bool IsCommitDescendantOf(const GitCommit& commit, const GitCommit& ancestor) const;


    // --------------------------------------------------------------------------
    // Tags
    // --------------------------------------------------------------------------

    // Executes the callback function for each of the repository's tags.
    // The callback function should return true to continue processing.
    void ForeachTag(const std::function<bool(GitTag)>& callback_function) const;

    // Returns all of the repository's tags.
    std::vector<GitTag> GetTags() const;


    // --------------------------------------------------------------------------
    // Ignore Rules
    // --------------------------------------------------------------------------

    // Adds ignore rules.
    void AddIgnoreRule(cs::string_sz rules);
    void AddIgnoreRulesFromFile(const std::string& file_path);

    // Clears ignore rules.
    void ClearIgnoreRules();

    // Returns true if the path is to be ignored based on the ignore rules.
    bool IsPathIgnored(const std::string& path) const;
    bool IsPathIgnored(std::string&& path) const;


private:
    void EnsureRepositoryIsOpen() const;

    void Open(std::string repo_directory, bool create, bool bare);

    template<typename GitObjectT>
    GitObject LookupObject(const GitObjectId& oid, GitObjectT type) const;

    bool IsPathIgnoredWorker(cs::string_sz path) const;

private:
    GitInitializer m_gitInitializer;
    std::string m_repoDirectory;
    git_repository* m_repo;
};
