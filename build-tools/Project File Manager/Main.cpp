#include "stdafx.h"
#include <zToolsO/DirectoryLister.h>
#include <zToolsO/FileIO.h>
#include <zUtilO/CSProExecutables.h>


struct ProjectPath
{
    std::string vcxproj_file_path;
    std::string vcxproj_text;

    std::string vcxproj_filter_file_path;
    std::string vcxproj_filter_text;
};


std::vector<std::string> runtime_errors;

std::vector<ProjectPath> LoadProjectPaths(const std::string& solution_directory);
std::string RemoveSourceControlStrings(const std::string& project_text);
std::string StandardizeProjectPaths(const std::string& project_directory, const std::string& project_text);
std::string GetDiskPathCase(const std::string& project_directory, const std::string& path);


int wmain(const int argc, const wchar_t* const argv[])
{
    try
    {
        const std::string solution_directory = ( argc == 2 ) ? MakeFullPath(GetWorkingDirectory(), TC::ToUtf8(argv[1])) :
                                                               MakeFullPath(PortableFunctions::PathGetDirectory(__FILE__), "..\\..\\cspro");

        if( !PortableFunctions::FileIsDirectory(solution_directory) )
            throw CSProException("The solution directory does not exist: " + solution_directory);

        const std::vector<ProjectPath> project_paths = LoadProjectPaths(solution_directory);

        for( const ProjectPath& project_path : project_paths )
        {
            const std::string project_directory = PortableFunctions::PathGetDirectory(project_path.vcxproj_file_path);

            std::string modified_vcxproj_text = RemoveSourceControlStrings(project_path.vcxproj_text);
            modified_vcxproj_text = StandardizeProjectPaths(project_directory, modified_vcxproj_text);

            if( modified_vcxproj_text != project_path.vcxproj_text )
                FileIO::WriteText(project_path.vcxproj_file_path, modified_vcxproj_text, true);

            const std::string modified_vcxproj_filter_text = StandardizeProjectPaths(project_directory, project_path.vcxproj_filter_text);

            if( modified_vcxproj_filter_text != project_path.vcxproj_filter_text )
                FileIO::WriteText(project_path.vcxproj_filter_file_path, modified_vcxproj_filter_text, true);
        }
    }

    catch( const CSProException& exception )
    {
        MessageBoxW(nullptr, TC::ToWide(exception.what()).c_str(), L"Project File Manager", MB_OK | MB_ICONEXCLAMATION);
    }

    if( !runtime_errors.empty() )
    {
        const std::string message = SO::CreateSingleString(runtime_errors, SO::Newline_crlf_sv);
        MessageBoxW(nullptr, TC::ToWide(message).c_str(), L"Project File Manager", MB_OK | MB_ICONEXCLAMATION);
    }
}


std::vector<ProjectPath> LoadProjectPaths(const std::string& solution_directory)
{
    std::vector<ProjectPath> project_paths;

    DirectoryLister directory_lister(true, false, true);

    DirectoryLister project_file_lister(false, true, false);
    project_file_lister.SetNameFilter("*.vcxproj;*.vcxproj.filters");

    for( const std::string& directory : directory_lister.GetPaths(solution_directory) )
    {
        std::vector<std::string> project_file_paths = project_file_lister.GetPaths(directory);

        if( project_file_paths.empty() )
            continue;

        if( project_file_paths.size() != 2 || ( project_file_paths[0] + ".filters" )  != project_file_paths[1] )
            throw CSProException("Unexpected files in " + directory);

        std::string vcxproj_text = FileIO::ReadText(project_file_paths[0]);
        std::string vcxproj_filter_text = FileIO::ReadText(project_file_paths[1]);

        project_paths.emplace_back(ProjectPath
            {
                std::move(project_file_paths[0]),
                std::move(vcxproj_text),
                std::move(project_file_paths[1]),
                std::move(vcxproj_filter_text)
            });
    }

    return project_paths;
}


std::string RemoveSourceControlStrings(const std::string& project_text)
{
    auto contains_source_control_string = [](const std::string_view text_sv)
    {
        constexpr std::string_view SourceControlStrings_sv[] =
        {
            "<SccProjectName>",
            "<SccLocalPath>",
            "<SccProvider>",
            "<SccAuxPath>"
        };

        return ( std::find_if(std::begin(SourceControlStrings_sv), std::end(SourceControlStrings_sv),
                              [&](const std::string_view sv) { return ( text_sv.find(sv) != std::string_view::npos ); }) != std::end(SourceControlStrings_sv) );
    };

    // quit if none of the strings exist
    if( !contains_source_control_string(project_text) )
        return project_text;

    // removes the lines containing these strings
    std::vector<std::string> lines = SO::SplitString(project_text, SO::Newline_crlf_sv, false, false);

    for( auto lines_itr = lines.begin(); lines_itr != lines.end(); )
    {
        if( contains_source_control_string(*lines_itr) )
        {
            lines_itr = lines.erase(lines_itr);
        }

        else
        {
            ++lines_itr;
        }
    }

    return SO::CreateSingleString(lines, SO::Newline_crlf_sv);
}


std::string StandardizeProjectPaths(const std::string& project_directory, const std::string& project_text)
{
    struct Entity
    {
        int sort_value;
        std::string path;
        std::vector<std::string> lines;
    };

    std::vector<Entity> entities;
    int sort_value_counter = 0;
    bool in_path_section = false;
    const char* expecting_end_tag = nullptr;

    const std::regex compile_regex(R"(^\s*<ClCompile\sInclude=\"(.*)\".*>\s*$)");
    const std::regex include_regex(R"(^\s*<ClInclude\sInclude=\"(.*)\".*>\s*$)");
    std::smatch matches;

    SO::ForeachLine<std::string>(project_text, true,
        [&](std::string line)
        {
            if( expecting_end_tag != nullptr )
            {
                ASSERT(in_path_section && !entities.empty() && !entities.back().path.empty());

                if( SO::Trim(line) == expecting_end_tag )
                    expecting_end_tag = nullptr;
            }

            else if( const bool matches_compile = std::regex_match(line, matches, compile_regex);
                     matches_compile || std::regex_match(line, matches, include_regex) )
            {
                // sort increment the sort counter when coming in from a non-path section
                if( !in_path_section )
                {
                    ++sort_value_counter;
                    in_path_section = true;
                }

                // add an entry, making sure the path matches what is on the disk
                entities.emplace_back(Entity { sort_value_counter, GetDiskPathCase(project_directory, matches.str(1)) });

                // see if we need to look for an end tag
                std::string_view trimmed_line_sv = SO::TrimRight(line);
                trimmed_line_sv.remove_prefix(trimmed_line_sv.length() - 2);

                if( trimmed_line_sv != "/>" )
                {
                    expecting_end_tag = matches_compile ? "</ClCompile>" :
                                                          "</ClInclude>";
                }
            }

            else
            {
                in_path_section = false;
                ++sort_value_counter;
                entities.emplace_back(Entity { sort_value_counter });
            }

            entities.back().lines.emplace_back(std::move(line));
        });

    ASSERT(!in_path_section && expecting_end_tag == nullptr);

    // sort the entries by path in alphabetical order
    std::sort(entities.begin(), entities.end(),
        [&](const Entity& e1, const Entity& e2)
        {
            return ( e1.sort_value == e2.sort_value ) ? ( SO::CompareNoCase(e1.path, e2.path) < 0 ) :
                                                        ( e1.sort_value < e2.sort_value );
        });

    std::vector<std::string> lines;

    for( Entity& entity : entities )
    {
        lines.insert(lines.end(), std::make_move_iterator(entity.lines.begin()),
                                  std::make_move_iterator(entity.lines.end()));
    }

    return SO::CreateSingleString(lines, SO::Newline_crlf_sv);
}


std::string GetDiskPathCase(const std::string& project_directory, const std::string& path)
{
    const std::string evaluated_path = MakeFullPath(project_directory, path);
    wchar_t short_path[MAX_PATH];
    wchar_t long_path[MAX_PATH];

    if( GetShortPathName(TC::ToWide(evaluated_path).c_str(), short_path, _countof(short_path)) <= 0 ||
        GetLongPathName(short_path, long_path, _countof(long_path)) <= 0 )
    {
        runtime_errors.emplace_back("GetDiskPathCase warning: " + evaluated_path);
        return path;
    }

    std::string disk_filename = PortableFunctions::PathGetFilename(TC::ToUtf8(long_path));

    if( PortableFunctions::PathGetFilename(path) == disk_filename )
        return path;

    const bool path_is_only_filename = PortableFunctions::PathGetDirectory(path).empty();

    return path_is_only_filename ? std::move(disk_filename) :
                                   PortableFunctions::PathReplaceFilename(path, disk_filename);
}
