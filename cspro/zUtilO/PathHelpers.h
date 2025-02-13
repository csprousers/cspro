#pragma once

#include <zUtilO/zUtilO.h>
#include <zUtilO/ConnectionString.h>


namespace PathHelpers
{
    struct StandardizedFilename
    {
        StandardizedFilename(const TCHAR* _filename)
            :   filename(_filename)
        {
        }

        StandardizedFilename(CString _filename)
            :   filename(std::move(_filename))
        {
        }

        StandardizedFilename(const ConnectionString& connection_string)
            :   filename(connection_string.IsFilenamePresent() ? WS2CS(connection_string.GetFilename()) : CString())
        {
        }

        CString filename;
    };

    CLASS_DECL_ZUTILO CString GetDirectoryName(const std::vector<StandardizedFilename>& standardized_filenames);
    CLASS_DECL_ZUTILO CString GetFilenameInDirectory(const TCHAR* filename, const std::vector<StandardizedFilename>& standardized_filenames);
    CLASS_DECL_ZUTILO CString GetFilenameInDirectory(const TCHAR* filename, CString directory_name);

    CLASS_DECL_ZUTILO ConnectionString AppendToConnectionStringFilename(const ConnectionString& connection_string, const TCHAR* append_text);

    CLASS_DECL_ZUTILO std::vector<ConnectionString> SplitSingleStringIntoConnectionStrings(const TCHAR* connection_string_single_string);

    CLASS_DECL_ZUTILO CString CreateSingleStringFromConnectionStrings(const std::vector<ConnectionString>& connection_strings,
                                                                      bool create_string_for_data_file_dlg,
                                                                      const CString& filename_for_relative_path_evaluation = CString());

    CLASS_DECL_ZUTILO void ExpandConnectionStringWildcards(std::vector<ConnectionString>& expanded_connection_strings,
                                                           const ConnectionString& connection_string);

    inline std::vector<ConnectionString> ExpandConnectionStringWildcards(const ConnectionString& connection_string)
    {
        std::vector<ConnectionString> expanded_connection_strings;
        ExpandConnectionStringWildcards(expanded_connection_strings, connection_string);
        return expanded_connection_strings;
    }
}
