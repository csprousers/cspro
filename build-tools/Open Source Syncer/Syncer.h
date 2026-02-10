#pragma once

#include <zGit/GitIgnoreEvaluator.h>
#include <zGit/GitTree.h>


class Syncer
{
public:
    Syncer(LoggingListBox& logging_list_box);
    ~Syncer();

    // Compares the files of the private and open sources repositories at the given commit.
    // Any differences are logged, false is returned if there are unexpected errors.
    bool CompareRepositories(const GitCommit& cs_commit, const GitCommit& os_commit, bool verbose);

    // Mirrors the feature branches.
    void MirrorFeatureBranches(const GitBranch& os_merge_branch,
                               const GitCommit& cs_oldest_merge_commit, const GitCommit& cs_newest_merge_commit);

    // Mirrors the single commit in the specified branch.
    void ManualMirrorSingleCommit(const GitBranch& os_branch, const GitCommit& cs_commit);

    // Mirrors the merge commit in the specified branch, using the two provided commits as the commit's parents.
    void ManualMirrorMergeCommit(const GitBranch& os_branch, const GitCommit& cs_merge_commit,
                                 const GitCommit& os_parent_commit1, const GitCommit& os_parent_commit2);

    // Reads tags from the repository of built libraries, storing them in the
    // settings database for future use.
    void RefreshLibraryTags();

    // Creates a commit in the open source libraries repository with the
    // built libraries at the given commit in the open source repository.
    void CommitBuildLibraries(const GitCommit& cs_commit);

private:
    // cs = private CSPro repository
    // os = public open source repository

    // Returns true if the file should not be included in the open source repository
    // because it was defined in the exclusions.txt file.
    bool IsFileExcluded(const std::string& cs_file_path);

    // Returns true when the file in the open source repository
    // has a replacement file defined in the replacements.json file.
    template<typename T = bool>
    T HasFileReplacement(const std::string& cs_file_path);

    // Returns a non-empty string containing the replacement data when the file in the open
    // source repository has a replacement file defined in the replacements.json file.
    std::string GetFileReplacement(const git_diff_file& new_file);

    // Returns the appropriate version of SQLite without the SQLite Encryption Extension (SEE).
    std::string CreateSqliteWithoutSEE(const git_diff_file& new_file);

    // Populates information about releases, used by CreateHistoryLog.
    void PopulateReleaseTags();

    // Returns the HISTORY.md file showing pull requests up to the commit.
    std::string CreateHistoryLog(const GitCommit& cs_latest_commit);

    // Updates the BUILD.md file with information about the built libraries to be
    // used at this point.
    void UpdateBuildDetails(GitIndex& os_index, const std::string& libraries_tag);

    // Walks the parents from one merge commit to another, returning the oldest and newest merge commits,
    // and all merge commits in between. The commits are returned in order from oldest to newest.
    static std::vector<GitCommit> GetOrderedMergedCommits(const GitCommit& oldest_merge_commit,
                                                          const GitCommit& newest_merge_commit);

    // Creates a commit in the open source repository, using the author / signature / message from the source commit.
    GitCommit CreateMirroredCommit(const GitCommit& cs_commit, const GitTree& os_written_tree,
                                   const GitCommit& os_parent_commit1, const GitCommit* os_parent_commit2);

    // Mirrors the feature branch, returning the merge commit.
    GitCommit MirrorFeatureBranch(const GitBranch& os_merge_branch,
                                  const GitCommit& cs_old_merge_commit, const GitCommit& cs_new_merge_commit);

    // Mirrors the feature branch's commits and returns the final commit.
    GitCommit MirrorFeatureBranchCommits(const GitCommit& cs_old_merge_commit, const GitCommit& cs_new_merge_commit,
                                         const GitCommit& cs_last_merged_commit);

    // Mirrors a single commit.
    void MirrorCommit(GitIndex& os_index, GitTree& cs_parent_tree, GitTree& cs_tree);

    // Methods to mirror files.
    void MirrorFile(GitIndex& os_index, const git_diff_delta& diff_delta);
    void MirrorFileAddEntry(GitIndex& os_index, const git_diff_file& new_file, const void* data, size_t size);
    void MirrorFileAddBinary(GitIndex& os_index, const git_diff_file& new_file);
    void MirrorFileAddText(GitIndex& os_index, const git_diff_file& new_file);
    void MirrorFileDelete(GitIndex& os_index, cs::string_sz path);
    void MirrorFileModifyBinary(GitIndex& os_index, const git_diff_file& new_file);
    void MirrorFileModifyText(GitIndex& os_index, const git_diff_file& old_file, const git_diff_file& new_file);
    void MirrorFileRenameBinary(GitIndex& os_index, const git_diff_file& old_file, const git_diff_file& new_file);
    void MirrorFileRenameText(GitIndex& os_index, const git_diff_file& old_file, const git_diff_file& new_file);

    // Populates information about the built libraries used by CSPro that are not
    // committed to the open source repository.
    void PopulateBuiltLibraries(const GitCommit& cs_commit);

    // Returns a cache key for the built libraries. The local version is only valid locally,
    // as it uses file times, and is only a shortcut to access the actual version used.
    std::string CalculateBuiltLibrariesCacheKey(bool local_version);

    // Returns the tag in the open source libraries repository that contains
    // the built libraries at the given commit. An exception is thrown when it does not exist.
    std::string GetTagForBuiltLibraries(const GitCommit& cs_commit);

private:
    Controller& m_controller;
    LoggingListBox& m_loggingListBox;

    std::optional<GitIgnoreEvaluator> m_exclusionEvaluator;

    struct FileReplacement { bool is_file_path; std::string file_path_or_routine; };
    std::map<std::string, FileReplacement> m_fileReplacements;

    struct TagCommits;
    std::vector<TagCommits> m_releaseTags;

    struct PullRequest;
    struct GroupedPullRequests;
    std::unique_ptr<GroupedPullRequests> m_lastHistoryLogCreationGroupedPullRequests;

    struct BuiltLibrary;
    std::vector<BuiltLibrary> m_builtLibraries;
};
