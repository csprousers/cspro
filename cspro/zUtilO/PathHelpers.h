#pragma once

#include <zUtilO/zUtilO.h>
#include <zUtilO/ConnectionString.h>


class CLASS_DECL_ZUTILO PathHelpers
{
public:
    struct StandardizedFilePath;

    static std::string GetDirectoryName(const std::vector<StandardizedFilePath>& standardized_file_paths);
    static std::string GetFilePathInDirectory(std::string_view filename_sv, const std::vector<StandardizedFilePath>& standardized_file_paths);
    static std::string GetFilePathInDirectory(std::string_view filename_sv, std::string directory_name);

    static ConnectionString AppendToConnectionStringFilename(const ConnectionString& connection_string, const char* append_text);

    static std::vector<ConnectionString> SplitSingleStringIntoConnectionStrings(cs::string_sz connection_string_single_string);

    static std::string CreateSingleStringFromConnectionStrings(const std::vector<ConnectionString>& connection_strings,
                                                               bool create_string_for_data_file_dlg,
                                                               const std::string& filename_for_relative_path_evaluation = std::string());

    static void ExpandConnectionStringWildcards(std::vector<ConnectionString>& expanded_connection_strings,
                                                const ConnectionString& connection_string);

    static std::vector<ConnectionString> ExpandConnectionStringWildcards(const ConnectionString& connection_string);
};


struct PathHelpers::StandardizedFilePath
{
    StandardizedFilePath(std::string file_path_)
        :   file_path(std::move(file_path_))
    {
    }

    StandardizedFilePath(const ConnectionString& connection_string)
        :   file_path(connection_string.HasFilePath() ? connection_string.GetFilePath() : std::string())
    {
    }

    std::string file_path;
};
