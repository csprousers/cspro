#include "StdAfx.h"
#include "PathHelpers.h"
#include <zToolsO/DirectoryLister.h>


CString PathHelpers::GetDirectoryName(const std::vector<StandardizedFilename>& standardized_filenames)
{
    for( const StandardizedFilename& standardized_filename : standardized_filenames )
    {
        if( !standardized_filename.filename.IsEmpty() )
            return PortableFunctions::PathGetDirectory<CString>(standardized_filename.filename);
    }

    return ReturnProgrammingError(CString());
}


CString PathHelpers::GetFilenameInDirectory(const TCHAR* filename, const std::vector<StandardizedFilename>& standardized_filenames)
{
    return GetDirectoryName(standardized_filenames) + filename;
}


CString PathHelpers::GetFilenameInDirectory(const TCHAR* filename, CString directory_name)
{
    return GetFilenameInDirectory(filename, { StandardizedFilename(std::move(directory_name)) });
}


ConnectionString PathHelpers::AppendToConnectionStringFilename(const ConnectionString& connection_string, const TCHAR* append_text)
{
    if( connection_string.IsFilenamePresent() && !PathHasWildcardCharacters(connection_string.GetFilename()) )
    {
        CString extension = PortableFunctions::PathGetFileExtension<CString>(connection_string.GetFilename());

        CString output_filename = PortableFunctions::PathRemoveFileExtension<CString>(connection_string.GetFilename());
        output_filename.Append(append_text);

        if( !extension.IsEmpty() )
            output_filename.AppendFormat(_T(".%s"), extension.GetString());

        return ConnectionString(connection_string.ToString(CS2WS(output_filename)));
    }

    return ConnectionString();
}


std::vector<ConnectionString> PathHelpers::SplitSingleStringIntoConnectionStrings(const TCHAR* connection_string_single_string)
{
    std::vector<ConnectionString> connection_strings;

    const TCHAR* start_pos = connection_string_single_string;
    bool in_quotes = false;

    auto process_connection_string = [&connection_strings, &start_pos](const TCHAR* this_start_pos, const TCHAR* this_end_pos)
    {
        if( this_end_pos >= this_start_pos )
        {
            CString filename(this_start_pos, this_end_pos - this_start_pos + 1);
            filename.Trim();

            if( !filename.IsEmpty() )
                connection_strings.emplace_back(filename);
        }
    };

    const TCHAR* itr = connection_string_single_string;

    for( ; *itr != 0; itr++ )
    {
        if( *itr == '"' )
        {
            // add a normal filename
            if( !in_quotes )
            {
                process_connection_string(start_pos, itr - 1);
                start_pos = itr;
                in_quotes = true;
            }

            // add a quoted filename
            else
            {
                process_connection_string(start_pos + 1, itr - 1);
                start_pos = itr + 1;
                in_quotes = false;
            }
        }
    }

    // add any last filename (even if it began but didn't end with a quote)
    process_connection_string(start_pos, itr - 1);

    return connection_strings;
}


CString PathHelpers::CreateSingleStringFromConnectionStrings(const std::vector<ConnectionString>& connection_strings,
                                                             bool create_string_for_data_file_dlg,
                                                             const CString& filename_for_relative_path_evaluation/* = CString()*/)
{
    CString connection_string_single_string;

    if( !connection_strings.empty() )
    {
        bool has_multiple_files = ( connection_strings.size() > 1 );
        CString starting_directory;

        if( create_string_for_data_file_dlg && connection_strings.front().IsFilenamePresent() )
            starting_directory = PortableFunctions::PathGetDirectory<CString>(connection_strings.front().GetFilename());

        for( const ConnectionString& connection_string : connection_strings )
        {
            bool use_full_path = starting_directory.IsEmpty() ||
                ( connection_string.IsFilenamePresent() && ( starting_directory.CompareNoCase(PortableFunctions::PathGetDirectory<CString>(connection_string.GetFilename())) != 0 ) );

            std::wstring filename =
                !filename_for_relative_path_evaluation.IsEmpty() ? connection_string.ToRelativeString(PortableFunctions::PathGetDirectory(filename_for_relative_path_evaluation)) :
                use_full_path                                    ? connection_string.ToString() :
                                                                   connection_string.ToStringWithoutDirectory();

            // when wildcards are used, force a | character to the filename so that special processing on the
            // dialog is triggered (unless multiple filenames are being used, in which case it isn't necessary)
            if( create_string_for_data_file_dlg && !has_multiple_files &&
                PathHasWildcardCharacters(filename) && ( filename.find('|') == std::wstring::npos ) )
            {
                filename.push_back('|');
            }

            connection_string_single_string.AppendFormat(has_multiple_files ? _T("%s\"%s\"") : _T("%s%s"),
                                                         connection_string_single_string.IsEmpty() ? _T("") : _T(" "),
                                                         filename.c_str());
        }
    }

    return connection_string_single_string;
}


void PathHelpers::ExpandConnectionStringWildcards(std::vector<ConnectionString>& expanded_connection_strings,
                                                  const ConnectionString& connection_string)
{
    // evaluate the filename to see if it has any wildcards
    if( PathHasWildcardCharacters(connection_string.GetFilename()) )
    {
        for( const std::wstring& evaluated_filename : DirectoryLister().SetNameFilter(PortableFunctions::PathGetFilename(connection_string.GetFilename()))
                                                                       .GetPaths(PortableFunctions::PathGetDirectory(connection_string.GetFilename())) )
        {
            expanded_connection_strings.emplace_back(connection_string.ToString(evaluated_filename));
        }
    }

    else
    {
        expanded_connection_strings.emplace_back(connection_string);
    }
}
