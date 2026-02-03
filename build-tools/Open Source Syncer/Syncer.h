#pragma once

#include <zUtilO/SettingsDb.h>
#include <zUtilF/LoggingListBox.h>
#include <zGit/GitCommit.h>
#include <zGit/GitIgnoreEvaluator.h>
#include <zGit/GitRepository.h>
#include <zGit/GitTag.h>
#include <zGit/GitTree.h>


class Syncer
{
public:
    Syncer(SettingsDb& settings_db, LoggingListBox& logging_list_box);

    void SetOpenSourceDirectory(const std::string& open_source_directory);

    std::vector<GitTag> GetTags() const;

    void CreateRelease(cs::string_sz commit_string);

    void ValidateRelease(cs::string_sz commit_string);

    void GenerateFileList(cs::string_sz commit_string);

private:
    // cs = private CSPro repository
    // os = public open source repository

    std::tuple<GitCommit, GitTree> LookupCommitAndGetTree(cs::string_sz commit_string);

    void PopulateRepoPaths(const GitTree& tree, const std::string& base_path);

    // Returns true if the file should not be included in the open source repository
    // because it was defined in the exclusions.txt file.
    bool IsFileExcluded(const std::string& cs_file_path);

    void PruneRepoPaths();

    void PrepareOutputDirectory();

    void CopyFilesToOutputDirectory();

    // Returns true when the file in the open source repository
    // has a replacement file defined in the replacements.json file.
    template<typename T = bool>
    T HasFileReplacement(const std::string& cs_file_path);

    // Returns a non-null object containing the replacement data when the file in the open
    // source repository has a replacement file defined in the replacements.json file.
    std::unique_ptr<BinaryBlock> GetFileReplacement(const git_diff_file& new_file);

    void CopyReplacementFiles();

    void CreateSqliteWithoutSEE(const GitTree& tree);

    struct TagCommits;
    std::vector<TagCommits> GetReleaseTags(std::string_view earliest_tag_sv);

    void CreateHistoryLog(const GitCommit& latest_commit);

    void EnsureRepositoriesMatch(bool add_space_before_log);

    void StartMirror();

    // Creates a commit in the open source repository, using the author / signature / message from the source commit.
    GitCommit CreateMirroredCommit(const GitCommit& cs_commit, const GitTree& os_written_tree,
                                   const GitCommit& os_parent_commit1, const GitCommit* os_parent_commit2);

    // Mirrors the feature branch, returning the merge commit.
    GitCommit MirrorFeatureBranch(const GitBranch& os_merge_branch, const GitCommit& os_start_commit,
                                  const GitCommit& cs_old_merge_commit, const GitCommit& cs_new_merge_commit);

    // Mirrors the feature branch's commits and returns the final commit.
    GitCommit MirrorFeatureBranchCommits(const GitCommit& cs_old_merge_commit, const GitCommit& cs_new_merge_commit,
                                         const GitCommit& cs_last_merged_commit);

    // Mirrors a single commit.
    void MirrorCommit(GitIndex& os_index, GitTree& cs_parent_tree, GitTree& cs_tree);

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
    SettingsDb& m_settingsDb;
    LoggingListBox& m_loggingListBox;

    std::string m_overridesDirectory;
    std::optional<GitIgnoreEvaluator> m_exclusionEvaluator;
    struct FileReplacement { bool is_file_path; std::string file_path_or_routine; };
    std::map<std::string, FileReplacement> m_fileReplacements;

    GitRepository m_privateRepo;

    GitRepository m_openSourceRepo;
    std::string m_openSourceDirectory;
    std::vector<std::string> m_repoPaths;
    std::map<std::string, GitObject> m_repoBlobObjects; // path -> object
};
