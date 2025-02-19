#pragma once

#include <zToolsO/zToolsO.h>
#include <regex>


CLASS_DECL_ZTOOLSO std::string CreateRegularExpressionFromFileSpec(std::string_view file_spec_sv);



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
    DirectoryLister& SetNameFilter(std::string_view file_spec_sv);
    DirectoryLister& SetNameFilter(wstring_view file_spec_sv) { return SetNameFilter(UTF8_TODO::GetUtf8(file_spec_sv)); }

    bool UsingNameFilter() const { return m_nameFilter.has_value(); }
    bool MatchesNameFilter(const std::string& path) const;

    std::vector<std::wstring> GetPaths(NullTerminatedString directory);
    std::vector<std::string> GetPaths(cs::string_sz directory) { return UTF8_TODO::GetUtf8(GetPaths(UTF8_TODO::GetWide(directory))); }

    void AddPaths(std::vector<std::string>& paths, const std::string& directory_path);
    void AddPaths(std::vector<std::wstring>& paths, InterfaceString directory_path);

    // adds the file paths that exist on the disk that match the provided file path, which can include wildcards
    static void AddFilenamesWithPossibleWildcard(std::vector<std::wstring>& filenames, NullTerminatedString filename,
                                                 bool include_non_existant_file_when_filename_does_not_use_wildcards);

    static void AddFilePathsWithPossibleWildcard(std::vector<std::string>& file_paths, const std::string& file_path,
                                                 bool include_non_existant_file_when_file_path_does_not_use_wildcards);

    static std::vector<std::string> GetFilePathsWithPossibleWildcard(const std::string& file_path,
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
    std::optional<std::regex> m_nameFilter;
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
