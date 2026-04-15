#pragma once

#include "Inputs.h"


class Builder
{
public:
    Builder(Inputs inputs, LoggingListBox& logging_list_box);

    void BuildSite();

    void UpdateBlog();

    void UpdateHelps();

    void UpdateMobileWorkshop();

    void UpdateGooglePlayPrivacyPolicy();

private:
    void RecycleDirectory(const std::string& directory);
    void CopyFile(const std::string& input_file_path, const std::string& output_file_path,
                  FileOverwriteFlag file_overwrite_flag = FileOverwriteFlag::Fail, bool add_message_to_log = true);
    void CopyDirectoryRecursive(const std::string& input_directory, const std::string& output_directory,
                                FileOverwriteFlag file_overwrite_flag = FileOverwriteFlag::Fail);

    struct BuildBlog { const std::string& posts_directory; };
    void BuildDocSet(const std::string& csdocset_file_path, std::variant<const char*, BuildBlog> build_name_or_build_blog);

private:
    Inputs m_inputs;
    LoggingListBox& m_loggingListBox;
};
