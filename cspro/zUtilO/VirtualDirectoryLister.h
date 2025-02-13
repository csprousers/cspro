#pragma once


// this class creates a virtual directory listing based on a group of files

class VirtualDirectoryLister
{
public:
    VirtualDirectoryLister(const std::vector<std::wstring>& filenames);

    // indicates if the directory exists; the directory should not have a trailing slash
    bool DirectoryExists(const std::wstring& directory) const;

    struct VirtualPath
    {
        std::wstring path; // directories will be returned without trailing slashes
        bool is_directory;
    };

    // gets the virtual paths in a directory; the directory must exist
    const std::vector<VirtualPath>& GetVirtualPaths(const std::wstring& directory) const;

private:
    struct VirtualDirectory
    {
        std::wstring constructed_directory; // stored with trailing slashes
        std::vector<std::wstring> filenames;
    };

    // each of the Add... functions adds paths in path-sorted order
    void AddSubdirectoriesForDirectory(std::vector<VirtualPath>& virtual_paths, const std::wstring& directory) const;
    void AddFilesForVirtualDirectory(std::vector<VirtualPath>& virtual_paths, const VirtualDirectory& virtual_directory) const;

private:
    std::vector<VirtualDirectory> m_virtualDirectories;
    mutable std::map<std::wstring, std::vector<VirtualPath>> m_cachedVirtualPaths;
};
