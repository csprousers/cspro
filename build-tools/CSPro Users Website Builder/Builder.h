#pragma once

#include "Inputs.h"
#include <zGit/GitIgnoreEvaluator.h>


class Builder
{
public:
    Builder(Inputs inputs, LoggingListBox& logging_list_box);

    void BuildSite();

    void UpdateBlog();

    void UpdateReleases();

    void UpdateHelps();

    void UpdateMobileWorkshop();

    void UpdateGooglePlayPrivacyPolicy();

    void ClearOutputs(UINT nID);

    void CreateWebsiteUpdaters(const std::string& last_processed_commit_sha);

private:
    void RecycleDirectory(const std::string& directory);

    void CopyFile(const std::string& input_file_path, const std::string& output_file_path,
                  bool add_message_to_log = true);

    void CopyDirectoryRecursive(const std::string& input_directory, const std::string& output_directory);

    struct BuildBlog { const std::string& posts_directory; };
    void BuildDocSet(const std::string& csdocset_file_path, std::variant<const char*, BuildBlog> build_name_or_build_blog);

    size_t ClearOutputs(GitIgnoreEvaluator& exclusion_evaluator, const std::string& directory_path, bool recursive);

    static void CreateWebsiteRemoveScript(const std::string& script_file_path, const std::vector<std::string>& removed_files);

    static void CreateWebsiteFilesZip(const std::string& zip_file_path, GitRepository& repo,
                                      const std::vector<std::tuple<std::string, GitObjectId>>& added_modified_files);

private:
    Inputs m_inputs;
    LoggingListBox& m_loggingListBox;
};
