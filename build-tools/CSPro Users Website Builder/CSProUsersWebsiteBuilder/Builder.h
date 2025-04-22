#pragma once

#include "Directories.h"


class Builder
{
public:
    Builder(Directories directories, LoggingListBox& logging_list_box);

    void UpdateMobileWorkshop();

    void UpdateGooglePlayPrivacyPolicy();

private:
    void RecycleDirectory(const std::string& directory);
    void CopyFile(const std::string& input_file_path, const std::string& output_file_path);
    void CopyDirectoryRecursive(const std::string& input_directory, const std::string& output_directory);

    void BuildDocSet(const std::string& csdocset_file_path, const std::string& build_name);

private:
    Directories m_directories;
    LoggingListBox& m_loggingListBox;
};
