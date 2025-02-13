#pragma once

#include <zToolsO/zToolsO.h>
#include <regex>


CLASS_DECL_ZTOOLSO bool PathHasWildcardCharacters(wstring_view path_sv);

CLASS_DECL_ZTOOLSO std::wstring CreateRegularExpressionFromFileSpec(wstring_view file_spec_sv);

#ifdef WIN32
typedef std::wregex FileSpecRegex;
#else
typedef std::regex FileSpecRegex;
#endif

// CreateRegexFromFileSpec can throw a regex_error
CLASS_DECL_ZTOOLSO FileSpecRegex CreateRegexFromFileSpec(wstring_view file_spec_sv);



// the DirectoryLister class returns the paths in a given directory, with options:
//     - to include only files or only directories;
//     - to search recursively; and
//     - to search with a filter (which will be applied only to the name);
// if the directory, which can be provided with or without a trailing slash, does not exist, no paths will be returned

class CLASS_DECL_ZTOOLSO DirectoryLister
{
protected:
    DirectoryLister(const DirectoryLister& directory_lister) = default;

public:
    // by default all non-hidden/system files in the directory
    // are listed (not recursively and not including directories)
    DirectoryLister(bool recursive = false, bool include_files = true, bool include_directories = false,
                    bool include_trailing_slash_on_directories = true, bool filter_directories = true);

    DirectoryLister& SetRecursive(bool recursive = true);
    DirectoryLister& SetIncludeFiles(bool include_files = true);
    DirectoryLister& SetIncludeDirectories(bool include_directories = true);
    DirectoryLister& SetIncludeHiddenSystemPaths(bool include_hidden_system_paths = true);
    DirectoryLister& SetNameFilter(wstring_view file_spec_sv);

    bool UsingNameFilter() const { return m_nameFilter.has_value(); }
    bool MatchesNameFilter(NullTerminatedString path) const;

    std::vector<std::wstring> GetPaths(NullTerminatedString directory);

    void AddPaths(std::vector<std::wstring>& paths, NullTerminatedString directory);

    // adds the filenames that exist on the disk that match the provided filename, which can include wildcards
    static void AddFilenamesWithPossibleWildcard(std::vector<std::wstring>& filenames, NullTerminatedString filename,
                                                 bool include_non_existant_file_when_filename_does_not_use_wildcards);

    static std::vector<std::wstring> GetFilenamesWithPossibleWildcard(NullTerminatedString filename,
                                                                      bool include_non_existant_file_when_filename_does_not_use_wildcards);

protected:
    bool FilterFiles() const       { return m_nameFilter.has_value(); }
    bool FilterDirectories() const { return ( m_filterDirectories && m_nameFilter.has_value() ); }

protected:
    bool m_recursive;
    bool m_includeFiles;
    bool m_includeDirectories;
    bool m_includeHiddenSystemPaths;
    bool m_includeTrailingSlashOnDirectories;
    bool m_filterDirectories;

private:
    std::optional<FileSpecRegex> m_nameFilter;
};



// --------------------------------------------------------------------------
// inline implementations
// --------------------------------------------------------------------------

inline DirectoryLister::DirectoryLister(const bool recursive/* = false*/, const bool include_files/* = true*/, const bool include_directories/* = false*/,
                                        const bool include_trailing_slash_on_directories/* = true*/, const bool filter_directories/* = true*/)
    :   m_recursive(recursive),
        m_includeFiles(include_files),
        m_includeDirectories(include_directories),
        m_includeHiddenSystemPaths(false),
        m_includeTrailingSlashOnDirectories(include_trailing_slash_on_directories),
        m_filterDirectories(filter_directories)
{
}


inline DirectoryLister& DirectoryLister::SetRecursive(const bool recursive/* = true*/)
{
    m_recursive = recursive;
    return *this;
}


inline DirectoryLister& DirectoryLister::SetIncludeFiles(const bool include_files/* = true*/)
{
    m_includeFiles = include_files;
    return *this;
}


inline DirectoryLister& DirectoryLister::SetIncludeDirectories(const bool include_directories/* = true*/)
{
    m_includeDirectories = include_directories;
    return *this;
}


inline DirectoryLister& DirectoryLister::SetIncludeHiddenSystemPaths(const bool include_hidden_system_paths/* = true*/)
{
    m_includeHiddenSystemPaths = include_hidden_system_paths;
    return *this;
}


inline std::vector<std::wstring> DirectoryLister::GetPaths(const NullTerminatedString directory)
{
    std::vector<std::wstring> paths;
    AddPaths(paths, directory);
    return paths;
}


inline std::vector<std::wstring> DirectoryLister::GetFilenamesWithPossibleWildcard(const NullTerminatedString filename,
                                                                                   const bool include_non_existant_file_when_filename_does_not_use_wildcards)
{
    std::vector<std::wstring> filenames;
    AddFilenamesWithPossibleWildcard(filenames, filename, include_non_existant_file_when_filename_does_not_use_wildcards);
    return filenames;
}
