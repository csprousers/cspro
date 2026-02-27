#pragma once

#include <zGit/GitIgnoreEvaluator.h>

struct ReleaseTag;


class Syncer
{
public:
    // cs = private CSPro repository
    // os = public open source repository

    Syncer(Controller& controller) noexcept;
    ~Syncer();

    // Returns the filename of the HISTORY.md file.
    static const char* GetHistoryFilename() noexcept;

    // Creates the contents of the HISTORY.md file showing pull requests up to the commit.
    std::string CreateHistoryLog(const GitCommit& cs_latest_commit);

    // Compares the files of the private and open sources repositories at the given commit.
    // Any differences are logged, false is returned if there are unexpected errors.
    bool CompareRepositories(const GitCommit& cs_commit, const GitCommit& os_commit, bool verbose);

    // Finds the oldest merge commit in the private repository that matches the open source repository's
    // current commit, using the same branch name, and then finds the most recent merge commit to sync,
    // returning an exception if there are no unsyncable merge commits.
    std::tuple<GitCommit, GitCommit> FindUnsyncedMergeCommits(const GitBranch& os_merge_branch);

    // Mirrors the feature branches.
    void MirrorFeatureBranches(const GitBranch& os_merge_branch,
                               const GitCommit& cs_oldest_merge_commit, const GitCommit& cs_newest_merge_commit);

    // Mirrors a single commit, or a range of commits, in the specified branch.
    // When mirroring a range of commits, cs_newest_commit must be a descendant of cs_oldest_commit.
    void MirrorCommits(const GitBranch& os_branch,
                       const GitCommit& cs_oldest_commit, const GitCommit* cs_newest_commit);

    // Mirrors the merge commit in the specified branch, using the two provided commits as the commit's parents.
    void MirrorMergeCommit(const GitBranch& os_branch, const GitCommit& cs_merge_commit,
                           const GitCommit& os_parent_commit1, const GitCommit& os_parent_commit2);

private:
    // Returns true if the file should not be included in the open source repository
    // because it was defined in the exclusions.txt file.
    bool IsFileExcluded(const std::string& cs_file_path);

    // Creates the BUILD.md file with information about the built libraries to be used at this point.
    void UpdateBuildDetails(GitIndex& os_index, GitTree& cs_tree, const std::string& libraries_tag);

    // Walks the parents from one merge commit to another, returning the oldest and newest merge commits,
    // and all merge commits in between. The commits are returned in order from oldest to newest.
    static std::vector<GitCommit> GetOrderedMergeCommits(const GitCommit& oldest_merge_commit,
                                                         const GitCommit& newest_merge_commit);

    // Walks the parents from one commit to another for commits that are not merge commits, returning the
    // oldest and newest commits, and all commits in between. The commits are returned in order from oldest to newest.
    static std::vector<GitCommit> GetOrderedCommits(const GitCommit& oldest_commit, const GitCommit& newest_commit);

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

private:
    Controller& m_controller;

    std::optional<GitIgnoreEvaluator> m_exclusionEvaluator;

    std::vector<ReleaseTag> m_releaseTags;

    struct PullRequest;
    struct GroupedPullRequests;
    std::unique_ptr<GroupedPullRequests> m_lastHistoryLogCreationGroupedPullRequests;
};
