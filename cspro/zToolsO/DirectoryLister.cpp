#include "StdAfx.h"
#include "DirectoryLister.h"

#ifdef WIN32
#include <filesystem>
#else
#include <dirent.h>
#endif


namespace
{
    constexpr TCHAR WildcardAsterisk     = '*';
    constexpr TCHAR WildcardQuestionMark = '?';

    constexpr const TCHAR* FileSpecSeparator = _T(";");

    constexpr std::wstring_view RegexEscapeCharacters = _T(".$^{[(|)+\\");
}


bool PathHasWildcardCharacters(wstring_view path_sv)
{
    constexpr TCHAR WildcardCharacters[] = { WildcardAsterisk, WildcardQuestionMark, 0 };
    return ( path_sv.find_first_of(WildcardCharacters) != wstring_view::npos );
}


std::wstring CreateRegularExpressionFromFileSpec(const wstring_view file_spec_sv)
{
    std::wstring pattern = _T("^");

    auto add_segment = [&](const wstring_view segment_sv)
    {
        const TCHAR* segment_itr = segment_sv.data();
        const TCHAR* segment_end = segment_itr + segment_sv.length();

        for( ; segment_itr != segment_end; ++segment_itr )
        {
            const TCHAR ch = *segment_itr;

            if( ch == WildcardQuestionMark )
            {
                pattern.push_back('.');
            }

            else if( ch == WildcardAsterisk )
            {
                // replace *.* with .* to match Windows file matching behavior where *.*
                // matches a file spec that doesn't have a .
                if( SO::StartsWith(segment_sv, _T("*.*")) )
                    segment_itr += 2;

                pattern.append(_T(".*"));
            }

            else if( RegexEscapeCharacters.find(ch) != std::wstring_view::npos )
            {
                // escape the regex special character
                pattern.push_back('\\');
                pattern.push_back(ch);
            }

            // paths will be considered case insensitive
            else if( is_alpha(ch) )
            {
                pattern.push_back('[');
                pattern.push_back(std::towlower(ch));
                pattern.push_back(std::towupper(ch));
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
    if( file_spec_sv.find(*FileSpecSeparator) == wstring_view::npos )
    {
        add_segment(file_spec_sv);
    }

    // multiple file specs
    else
    {
        bool add_pipe = false;

        for( const wstring_view segment_sv : SO::SplitString<wstring_view>(file_spec_sv, FileSpecSeparator) )
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


FileSpecRegex CreateRegexFromFileSpec(const wstring_view file_spec_sv)
{
    const std::wstring pattern = CreateRegularExpressionFromFileSpec(file_spec_sv);

#ifdef WIN32
    return std::wregex(pattern);
#else
    return std::regex(UTF8Convert::WideToUTF8(pattern));
#endif
}


DirectoryLister& DirectoryLister::SetNameFilter(const wstring_view file_spec_sv)
{
    try
    {
        m_nameFilter.reset();

        if( !SO::IsWhitespace(file_spec_sv) )
            m_nameFilter = CreateRegexFromFileSpec(file_spec_sv);
    }

    catch( const std::regex_error& )
    {
        ASSERT(false);
    }

    return *this;
}


bool DirectoryLister::MatchesNameFilter(const NullTerminatedString path) const
{
    ASSERT(m_nameFilter.has_value());

#ifdef WIN32
    return std::regex_match(path.c_str(), *m_nameFilter);
#else
    return std::regex_match(UTF8Convert::WideToUTF8(path), *m_nameFilter);
#endif
}


#ifdef WIN32

void DirectoryLister::AddPaths(std::vector<std::wstring>& paths, const NullTerminatedString directory)
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
                    !MatchesNameFilter(directory_entry.path().filename().native()) )
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
        const std::filesystem::path path(directory);

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
        ASSERT(!PortableFunctions::FileIsDirectory(directory));
    }
}

#endif


void DirectoryLister::AddFilenamesWithPossibleWildcard(std::vector<std::wstring>& filenames, const NullTerminatedString filename,
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

        if( PathHasWildcardCharacters(filename_only) )
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


#ifndef WIN32

// a temporary implementation until std::filesystem is available on the NDK
void DirectoryLister::AddPaths(std::vector<std::wstring>& paths, const NullTerminatedString directory)
{
    DIR* dir = opendir(UTF8Convert::WideToUTF8(directory).c_str());

    if( dir == nullptr )
        return;

    dirent* dir_entry;

    while( ( dir_entry = readdir(dir) ) != nullptr )
    {
        const std::wstring name = UTF8Convert::UTF8ToWide(dir_entry->d_name);

        if( name == _T(".") || name == _T("..") )
            continue;

        const std::wstring path = PortableFunctions::PathAppendToPath<std::wstring>(directory, name);
        const bool is_directory = ( dir_entry->d_type == DT_DIR );

        auto passes_filters = [&]()
        {
            // apply name filters...
            if( m_nameFilter.has_value() &&
                ( !is_directory || m_filterDirectories ) &&
                !MatchesNameFilter(name) )
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
