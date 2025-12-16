#pragma once

#include <zUtilO/SettingsDb.h>
#include <zUtilF/LoggingListBox.h>
#include <zGit/GitCommit.h>
#include <zGit/GitRepository.h>
#include <zGit/GitTag.h>
#include <zGit/GitTree.h>


class Creator
{
public:
    Creator(SettingsDb& settings_db);

    std::vector<GitTag> GetTags() const;

    void Initialize(LoggingListBox& logging_list_box, const std::string& open_source_directory);

    void CreateRelease(cs::string_sz commit_string);

    void ValidateRelease(cs::string_sz commit_string);

    void GenerateFileList(cs::string_sz commit_string);

private:
    std::tuple<GitCommit, GitTree> LookupCommitAndGetTree(cs::string_sz commit_string);

    void PopulateRepoPaths(const GitTree& tree, const std::string& base_path);

    void PruneRepoPaths();

    void PrepareOutputDirectory();

    void CopyFilesToOutputDirectory();

    void CopyReplacementFiles();

    void CreateSqliteWithoutSEE(const GitTree& tree);

    struct TagCommits;
    std::vector<TagCommits> GetReleaseTags(std::string_view earliest_tag_sv);

    void CreateHistoryLog(const GitCommit& latest_commit);

    void EnsureRepositoriesMatch(bool add_space_before_log);

private:
    SettingsDb& m_settingsDb;
    std::string m_overridesDirectory;
    GitRepository m_repo;
    LoggingListBox* m_loggingListBox;
    std::string m_openSourceDirectory;
    std::vector<std::string> m_repoPaths;
    std::map<std::string, GitObject> m_repoBlobObjects; // path -> object
};
