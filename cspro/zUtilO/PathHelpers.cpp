#include "StdAfx.h"
#include "PathHelpers.h"
#include <zToolsO/DirectoryLister.h>


std::string PathHelpers::GetDirectoryName(const std::vector<StandardizedFilePath>& standardized_file_paths)
{
    for( const StandardizedFilePath& standardized_file_path : standardized_file_paths )
    {
        if( !standardized_file_path.file_path.empty() )
            return PortableFunctions::PathGetDirectory(standardized_file_path.file_path);
    }

    return std::string();
}


std::string PathHelpers::GetFilePathInDirectory(const std::string_view filename_sv, const std::vector<StandardizedFilePath>& standardized_filenames)
{
    return Path::Combine(GetDirectoryName(standardized_filenames), filename_sv);
}


std::string PathHelpers::GetFilePathInDirectory(const std::string_view filename_sv, std::string directory_name)
{
    return GetFilePathInDirectory(filename_sv, { StandardizedFilePath(std::move(directory_name)) });
}


ConnectionString PathHelpers::AppendToConnectionStringFilename(const ConnectionString& connection_string, const char* const append_text)
{
    if( !connection_string.HasFilePath() || Path::HasWildcardCharacters(connection_string.GetFilePath()) )
        return ConnectionString();

    const std::string extension = PortableFunctions::PathGetFileExtension(connection_string.GetFilePath());

    std::string output_file_path = PortableFunctions::PathRemoveFileExtension(connection_string.GetFilePath());
    output_file_path.append(append_text);

    output_file_path = PortableFunctions::PathAppendFileExtension(output_file_path, extension);

    return ConnectionString(connection_string.ToString(output_file_path));

}


std::vector<ConnectionString> PathHelpers::SplitSingleStringIntoConnectionStrings(const cs::string_sz connection_string_single_string)
{
    std::vector<ConnectionString> connection_strings;

    const char* start_pos = connection_string_single_string.c_str();
    bool in_quotes = false;

    auto process_connection_string = [&](const char* const this_start_pos, const char* const this_end_pos)
    {
        if( this_end_pos >= this_start_pos )
        {
            std::string_view text_sv(this_start_pos, this_end_pos - this_start_pos + 1);
            text_sv = SO::Trim(text_sv);

            if( !text_sv.empty() )
                connection_strings.emplace_back(text_sv);
        }
    };

    const char* itr = start_pos;

    for( ; *itr != '\0'; itr++ )
    {
        if( *itr == '"' )
        {
            // add a normal path
            if( !in_quotes )
            {
                process_connection_string(start_pos, itr - 1);
                start_pos = itr;
                in_quotes = true;
            }

            // add a quoted path
            else
            {
                process_connection_string(start_pos + 1, itr - 1);
                start_pos = itr + 1;
                in_quotes = false;
            }
        }
    }

    // add any last path (even if it started but didn't end with a quote)
    process_connection_string(start_pos, itr - 1);

    return connection_strings;
}


std::string PathHelpers::CreateSingleStringFromConnectionStrings(const std::vector<ConnectionString>& connection_strings,
                                                                 const bool create_string_for_data_file_dlg,
                                                                 const std::string& filename_for_relative_path_evaluation/* = std::string()*/)
{
    std::string connection_string_single_string;

    if( !connection_strings.empty() )
    {
        const bool has_multiple_files = ( connection_strings.size() > 1 );
        std::string starting_directory;

        if( create_string_for_data_file_dlg && connection_strings.front().HasFilePath() )
            starting_directory = PortableFunctions::PathGetDirectory(connection_strings.front().GetFilePath());

        for( const ConnectionString& connection_string : connection_strings )
        {
            const bool use_full_path = ( ( starting_directory.empty() ) ||
                                         ( connection_string.HasFilePath() && !SO::EqualsNoCase(starting_directory, PortableFunctions::PathGetDirectory(connection_string.GetFilePath())) ) );

            std::string filename =
                !filename_for_relative_path_evaluation.empty() ? connection_string.ToRelativeString(PortableFunctions::PathGetDirectory(filename_for_relative_path_evaluation)) :
                use_full_path                                  ? connection_string.ToString() :
                                                                 connection_string.ToStringWithoutDirectory();

            // when wildcards are used, force a | character to the filename so that special processing on the
            // dialog is triggered (unless multiple filenames are being used, in which case it isn't necessary)
            if( create_string_for_data_file_dlg &&
                !has_multiple_files &&
                Path::HasWildcardCharacters(filename) &&
                filename.find(PropertyString::PropertySeparatorInitial) == std::string::npos )
            {
                filename.push_back(PropertyString::PropertySeparatorInitial);
            }

            connection_string_single_string.append(FormatText(has_multiple_files ? "%s\"%s\"" : "%s%s",
                                                              connection_string_single_string.empty() ? "" : " ",
                                                              filename.c_str()));
        }
    }

    return connection_string_single_string;
}


void PathHelpers::ExpandConnectionStringWildcards(std::vector<ConnectionString>& expanded_connection_strings,
                                                  const ConnectionString& connection_string)
{
    // evaluate the filename to see if it has any wildcards
    if( connection_string.HasFilePath() )
    {
        const std::string filename = PortableFunctions::PathGetFilename(connection_string.GetFilePath());

        if( Path::HasWildcardCharacters(filename) )
        {
            for( std::string& evaluated_file_path : DirectoryLister().SetNameFilter(filename)
                                                                     .GetPaths(PortableFunctions::PathGetDirectory(connection_string.GetFilePath())) )
            {
                expanded_connection_strings.emplace_back(connection_string.ToString(std::move(evaluated_file_path)));
            }

            return;
        }
    }

    expanded_connection_strings.emplace_back(connection_string);
}


std::vector<ConnectionString> PathHelpers::ExpandConnectionStringWildcards(const ConnectionString& connection_string)
{
    std::vector<ConnectionString> expanded_connection_strings;
    ExpandConnectionStringWildcards(expanded_connection_strings, connection_string);
    return expanded_connection_strings;
}
