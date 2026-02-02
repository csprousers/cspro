#pragma once

#include <zGit/zGit.h>
#include <zGit/GitInitializer.h>
#include <zGit/GitObjectId.h>

struct git_repository;
class GitBlob;
class GitBranch;
class GitCommit;
class GitDiff;
class GitIndex;
enum class GitObjectType;
class GitTag;
class GitTree;
class GitSignature;


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
    GitBranch LookupBranch(std::string branch_name) const;

    // Creates a new branch, throwing an exception on error (e.g., if a branch with
    // the name already exists). This does not change the current branch.
    GitBranch CreateBranch(std::string branch_name, const GitCommit& commit) const;

    // Executes the callback function for each of the repository's local branches.
    // The callback function, which can throw exceptions, should return true to continue processing.
    void ForeachLocalBranch(const std::function<bool(GitBranch)>& callback_function) const;

    // Sets the HEAD to the specified branch in safe mode:
    // "Allow safe updates that cannot overwrite uncommitted data. If the uncommitted
    // changes don't conflict with the checked out files, the checkout will still proceed,
    // leaving the changes intact."
    void CheckoutBranch(const GitBranch& branch) const;

    // Resets the current branch to the commit using the mode "mixed."
    void ResetBranchMixed(const GitCommit& commit) const;


    // --------------------------------------------------------------------------
    // Indices + Statuses + Differences
    // --------------------------------------------------------------------------

    // Returns the repository's index file. The index may be a cached version.
    GitIndex GetIndex() const;

    // Returns the repository's index file, refreshing it rather than using a cached version.
    GitIndex GetUpdatedIndex() const;

    // Writes the index as a tree, throwing an exception on error.
    GitTree WriteTree(GitIndex& index) const;

    // Returns the status of a file by path.
    // An exception is thrown if the entry cannot be found.
    // Status codes are in status.h.
    unsigned int GetStatusByPath(cs::string_sz path) const;

    // Returns true if there are changes in the index or working directory.
    bool HasChanges() const;

    // Executes the callback function for each file in the index, passing the
    // path and status code. These are paths that Git is tracking: "the index
    // (or 'cache', or 'staging area') is the contents of the next commit."
    // The callback function can throw exceptions.
    // Status codes are in status.h.
    void ForeachStatusInIndex(const std::function<void(std::string path, unsigned int status_flags)>& callback_function) const;

    // Executes the callback function for each file in the working directory with
    // a different status from the index, passing the path and status code. The
    // files could be new (untracked), modified, deleted, etc. These are paths that
    // are different "based on [an] index to working directory comparison."
    // // The callback function can throw exceptions.
    // Status codes are in status.h.
    void ForeachStatusInWorkingDirectory(const std::function<void(std::string path, unsigned int status_flags)>& callback_function) const;

    // Returns an object than can be used to determine differences between two trees.
    // The diff_flags value is a combination of git_diff_option_t options (defined in diff.h).
    // If not specified, details about the differences within files themselves are not loaded.
    GitDiff GetDifference(GitTree& old_tree, GitTree& new_tree, uint32_t diff_flags) const;
    GitDiff GetDifference(GitTree& old_tree, GitTree& new_tree) const;

    // Returns an object than can be used to determine differences in the working directory
    // that have a different status from the specified commit's tree. For speed, the routine
    // compares the tree to the index and then the index to the working directory,
    // git_diff_tree_to_index + git_diff_index_to_workdir, rather than calling
    // git_diff_tree_to_workdir_with_index.
    GitDiff GetDifferenceInWorkingDirectory(const GitCommit& commit) const;


    // --------------------------------------------------------------------------
    // Objects + Blobs
    // --------------------------------------------------------------------------

    // Looks up the object, potentially only of a certain type, throwing an
    // exception if not found.
    GitObject LookupObject(const GitObjectId& oid, GitObjectType type) const;
    GitObject LookupObject(const GitObjectId& oid) const;

    // Looks up a blob by object ID, throwing an exception if not found.
    GitBlob LookupBlob(const GitObjectId& oid) const;


    // --------------------------------------------------------------------------
    // Commits
    // --------------------------------------------------------------------------

    // Looks up the commit, throwing an exception if not found.
    // Hex hashes can be provided in both short or long forms.
    GitCommit LookupCommit(const GitObjectId& oid) const;
    GitCommit LookupCommit(cs::string_sz hex_hash) const;

    // Returns the commit associated with the branch's target, throwing an exception on error.
    GitCommit LookupCommit(const GitBranch& branch) const;

    // Returns the commit associated with the tag, throwing an exception on error.
    GitCommit LookupCommit(const GitTag& tag) const;

    // Returns true if the commit "is the descendant of another commit."
    bool IsCommitDescendantOf(const GitCommit& commit, const GitCommit& ancestor) const;

    // Creates a commit, updating the HEAD of the current branch, making it point to
    // this commit. An exception is thrown on error.
    GitObjectId CreateCommit(const GitSignature& author, const GitSignature& committer,
                             cs::string_sz message, const GitTree& tree,
                             const GitCommit& parent_commit1, const GitCommit* parent_commit2 = nullptr);

    GitObjectId CreateCommit(const GitSignature& author_and_committer,
                             cs::string_sz message, const GitTree& tree,
                             const GitCommit& parent_commit1, const GitCommit* parent_commit2 = nullptr);

    // --------------------------------------------------------------------------
    // Tags
    // --------------------------------------------------------------------------

    // Executes the callback function for each of the repository's tags.
    // The callback function, which can throw exceptions, should return true to continue processing.
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
