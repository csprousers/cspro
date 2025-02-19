#include "StdAfx.h"
#include "DirectoryLister.h"

#ifdef WIN32
#include <filesystem>
#else
#include <dirent.h>
#endif


namespace
{
    constexpr const char* FileSpecSeparator = ";";

    constexpr std::string_view RegexEscapeCharacters_sv = ".$^{[(|)+\\";
}


std::string CreateRegularExpressionFromFileSpec(const std::string_view file_spec_sv)
{
    std::string pattern = "^";

    auto add_segment = [&](const std::string_view segment_sv)
    {
        const char* segment_itr = segment_sv.data();
        const char* segment_end = segment_itr + segment_sv.length();

        for( ; segment_itr != segment_end; ++segment_itr )
        {
            const char ch = *segment_itr;

            if( ch == Path::WildcardQuestionMark )
            {
                pattern.push_back('.');
            }

            else if( ch == Path::WildcardAsterisk )
            {
                // replace *.* with .* to match Windows file matching behavior where *.*
                // matches a file spec that doesn't have a .
                if( SO::StartsWith(segment_sv, "*.*") )
                    segment_itr += 2;

                pattern.append(".*");
            }

            else if( RegexEscapeCharacters_sv.find(ch) != std::string_view::npos )
            {
                // escape the regex special character
                pattern.push_back('\\');
                pattern.push_back(ch);
            }

            // paths will be considered case insensitive
            else if( is_alpha(ch) )
            {
                pattern.push_back('[');
                pattern.push_back(static_cast<char>(std::tolower(ch)));
                pattern.push_back(static_cast<char>(std::toupper(ch)));
                pattern.push_back(']');
            }

            else
            {
                pattern.push_back('[');
                pattern.push_back(ch);
                pattern.push_back(']');
            }
        }
    };


    // a single file spec
    if( file_spec_sv.find(*FileSpecSeparator) == std::string_view::npos )
    {
        add_segment(file_spec_sv);
    }

    // multiple file specs
    else
    {
        bool add_pipe = false;

        for( const std::string_view segment_sv : SO::SplitString<std::string_view>(file_spec_sv, FileSpecSeparator) )
        {
            if( add_pipe )
            {
                pattern.push_back('|');
            }

            else
            {
                add_pipe = true;
            }

            pattern.push_back('(');
            add_segment(segment_sv);
            pattern.push_back(')');
        }
    }

    pattern.push_back('$');

    return pattern;
}


DirectoryLister& DirectoryLister::SetNameFilter(const std::string_view file_spec_sv)
{
    try
    {
        m_nameFilter.reset();

        if( !SO::IsWhitespace(file_spec_sv) )
            m_nameFilter.emplace(CreateRegularExpressionFromFileSpec(file_spec_sv));
    }

    catch( const std::regex_error& )
    {
        ASSERT(false);
    }

    return *this;
}


bool DirectoryLister::MatchesNameFilter(const std::string& path) const
{
    ASSERT(m_nameFilter.has_value());

    return std::regex_match(path, *m_nameFilter);
}


void DirectoryLister::AddPaths(std::vector<std::string>& paths, const std::string& directory_path)
{
    std::vector<std::wstring> wide_paths;
    AddPaths(wide_paths, directory_path);

    for( const std::wstring& wide_path : wide_paths )
        paths.emplace_back(UTF8_TODO::GetUtf8(wide_path));
}


#ifdef WIN32

void DirectoryLister::AddPaths(std::vector<std::wstring>& paths, const InterfaceString directory_path)
{
    ASSERT(m_includeFiles || m_includeDirectories);

    auto process_entries = [&](auto&& directory_iterator)
    {
        for( const std::filesystem::directory_entry& directory_entry : directory_iterator )
        {
            const bool is_regular_file = directory_entry.is_regular_file();

            auto passes_filters = [&]()
            {
                // apply name filters...
                if( m_nameFilter.has_value() &&
                    ( is_regular_file || m_filterDirectories ) &&
                    !MatchesNameFilter(UTF8_TODO::GetUtf8(directory_entry.path().filename().native())) )
                {
                    return false;
                }

                // ...and hidden/system path filters
                if( !m_includeHiddenSystemPaths )
                {
                    const DWORD attributes = GetFileAttributes(directory_entry.path().c_str());

                    if( ( attributes & FILE_ATTRIBUTE_HIDDEN ) != 0 ||
                        ( attributes & FILE_ATTRIBUTE_SYSTEM ) != 0 )
                    {
                        return false;
                    }
                }

                return true;
            };

            if( is_regular_file )
            {
                if( m_includeFiles && passes_filters() )
                    paths.emplace_back(directory_entry.path().native());
            }

            else if( directory_entry.is_directory() && m_includeDirectories && passes_filters() )
            {
                // add directories with a trailing slash (since that is what the predecessor function ReadChildFiles did)
                if( m_includeTrailingSlashOnDirectories )
                {
                    paths.emplace_back(PortableFunctions::PathEnsureTrailingSlash<std::wstring>(directory_entry.path()));
                }

                else
                {
                    paths.emplace_back(directory_entry.path());
                }
            }
        }
    };

    try
    {
        const std::filesystem::path path(directory_path.c_str());

        if( m_recursive )
        {
            process_entries(std::filesystem::recursive_directory_iterator(path));
        }

        else
        {
            process_entries(std::filesystem::directory_iterator(path));
        }
    }

    catch( const std::filesystem::filesystem_error& )
    {
        ASSERT(!PortableFunctions::FileIsDirectory(directory_path));
    }
}

#endif


void DirectoryLister::AddFilenamesWithPossibleWildcard(std::vector<std::wstring>& filenames, const NullTerminatedString filename, // UTF8_TODO remove
                                                       const bool include_non_existant_file_when_filename_does_not_use_wildcards)
{
    // short-circuit the most common request
    if( PortableFunctions::FileIsRegular(filename) )
    {
        filenames.emplace_back(filename);
    }

    else
    {
        const std::wstring filename_only = PortableFunctions::PathGetFilename(filename);

        if( Path::HasWildcardCharacters(UTF8_TODO::GetUtf8(filename_only)) )
        {
            DirectoryLister().SetNameFilter(filename_only)
                             .AddPaths(filenames, PortableFunctions::PathGetDirectory(filename));
        }

        else if( include_non_existant_file_when_filename_does_not_use_wildcards )
        {
            filenames.emplace_back(filename);
        }
    }
}


void DirectoryLister::AddFilePathsWithPossibleWildcard(std::vector<std::string>& file_paths, const std::string& file_path,
                                                       const bool include_non_existant_file_when_file_path_does_not_use_wildcards)
{
    // short-circuit the most common request
    if( PortableFunctions::FileIsRegular(file_path) )
    {
        file_paths.emplace_back(file_path);
    }

    else
    {
        const std::string filename_only = PortableFunctions::PathGetFilename(file_path);

        if( Path::HasWildcardCharacters(filename_only) )
        {
            DirectoryLister().SetNameFilter(filename_only)
                             .AddPaths(file_paths, PortableFunctions::PathGetDirectory(file_path));
        }

        else if( include_non_existant_file_when_file_path_does_not_use_wildcards )
        {
            file_paths.emplace_back(file_path);
        }
    }
}


std::vector<std::string> DirectoryLister::GetFilePathsWithPossibleWildcard(const std::string& file_path,
                                                                           const bool include_non_existant_file_when_filename_does_not_use_wildcards)
{
    std::vector<std::string> file_paths;
    AddFilePathsWithPossibleWildcard(file_paths, file_path, include_non_existant_file_when_filename_does_not_use_wildcards);
    return file_paths;
}



#ifndef WIN32

// a temporary implementation until std::filesystem is available on the NDK
void DirectoryLister::AddPaths(std::vector<std::wstring>& paths, const InterfaceString directory_path)
{
    DIR* dir = opendir(directory_path.c_str());

    if( dir == nullptr )
        return;

    dirent* dir_entry;

    while( ( dir_entry = readdir(dir) ) != nullptr )
    {
        const std::wstring name = UTF8_TODO::GetWide(dir_entry->d_name);

        if( name == _T(".") || name == _T("..") )
            continue;

        const std::wstring path = PortableFunctions::PathAppendToPath<std::wstring>(UTF8_TODO::GetWide(directory_path.GetString()), name);
        const bool is_directory = ( dir_entry->d_type == DT_DIR );

        auto passes_filters = [&]()
        {
            // apply name filters...
            if( m_nameFilter.has_value() &&
                ( !is_directory || m_filterDirectories ) &&
                !MatchesNameFilter(UTF8_TODO::GetUtf8(name)) )
            {
                return false;
            }

            // ...and hidden/system path filters
            if( !m_includeHiddenSystemPaths && !path.empty() && path.front() == '.' )
            {
                return false;
            }

            return true;
        };

        if( is_directory )
        {
            if( m_includeDirectories && passes_filters() )
            {
                // add trailing / to directories to be consistent with Windows
                if( m_includeTrailingSlashOnDirectories )
                {
                    paths.emplace_back(PortableFunctions::PathEnsureTrailingSlash(path));
                }

                else
                {
                    paths.emplace_back(path);
                }
            }

            if( m_recursive )
                AddPaths(paths, path);
        }

        else if( dir_entry->d_type == DT_REG || dir_entry->d_type == DT_LNK )
        {
            if( m_includeFiles && passes_filters() )
                paths.emplace_back(std::move(path));
        }
    }

    closedir(dir);
}

#endif
