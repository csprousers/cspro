#include "StdAfx.h"
#include "VirtualDirectoryLister.h"


VirtualDirectoryLister::VirtualDirectoryLister(const std::vector<std::string>& file_paths)
{
    struct VolumeAndCommonRoot
    {
        std::string volume;
        std::string common_root;
    };

    struct DirectoryInformation
    {
        std::string directory;
        std::vector<std::string> file_paths;
        const VolumeAndCommonRoot* volume_and_column_root;
    };

    // calculate all of the unique directories along with their associated files
    std::vector<DirectoryInformation> directory_infos;

    for( const std::string& file_path : file_paths )
    {
        std::string directory = PortableFunctions::PathGetDirectory(file_path);

        ASSERT(directory == PortableFunctions::PathEnsureTrailingSlash(directory) &&
               directory == PortableFunctions::PathToNativeSlash(directory));

        auto directory_info_lookup = std::find_if(directory_infos.begin(), directory_infos.end(),
                                                  [&](const DirectoryInformation& di) { return SO::EqualsNoCase(di.directory, directory); });

        if( directory_info_lookup != directory_infos.cend() )
        {
            directory_info_lookup->file_paths.emplace_back(file_path);
        }

        else
        {
            directory_infos.emplace_back(DirectoryInformation { std::move(directory), { file_path }, nullptr });
        }
    }

    // categorize the directories by "volume" and calculate the common roots
    std::vector<std::unique_ptr<VolumeAndCommonRoot>> volume_and_common_roots;

    for( DirectoryInformation& directory_info : directory_infos )
    {
        const size_t slash_pos = directory_info.directory.find_first_of(Path::SlashChars_sv);
        std::string volume = directory_info.directory.substr(0, slash_pos + 1);

        // make the volume look like a directory; C:\ -> C
        volume = SO::TrimRight(PortableFunctions::PathRemoveTrailingSlash(volume), ':');

        auto volume_lookup = std::find_if(volume_and_common_roots.begin(), volume_and_common_roots.end(),
                                          [&](const std::unique_ptr<VolumeAndCommonRoot>& vacr) { return SO::EqualsNoCase(vacr->volume, volume); });

        if( volume_lookup == volume_and_common_roots.end() )
        {
            auto vacr = std::make_unique<VolumeAndCommonRoot>(VolumeAndCommonRoot { std::move(volume), directory_info.directory });
            directory_info.volume_and_column_root = vacr.get();
            volume_and_common_roots.emplace_back(std::move(vacr));
        }

        else
        {
            directory_info.volume_and_column_root = volume_lookup->get();
            (*volume_lookup)->common_root = Path::GetCommonRoot((*volume_lookup)->common_root, directory_info.directory);
            ASSERT(!(*volume_lookup)->common_root.empty());
        }
    }

    // now create virtual directories using the "volume" but leaving out the common roots
    const bool add_volume_to_directory = ( volume_and_common_roots.size() > 1 );

    for( DirectoryInformation& directory_info : directory_infos )
    {
        ASSERT(directory_info.volume_and_column_root != nullptr);

        const std::string_view directory_without_common_root_sv = std::string_view(directory_info.directory).substr(directory_info.volume_and_column_root->common_root.length());

        std::string constructed_path = add_volume_to_directory ? Path::Combine(directory_info.volume_and_column_root->volume, directory_without_common_root_sv) :
                                                                 std::string(directory_without_common_root_sv);

        ASSERT(constructed_path == PortableFunctions::PathEnsureTrailingSlash(constructed_path));
        ASSERT(!constructed_path.empty() || directory_infos.size() == 1);

        m_virtualDirectories.emplace_back(VirtualDirectory
            {
                std::move(constructed_path),
                std::move(directory_info.file_paths)
            });
    }
}


bool VirtualDirectoryLister::DirectoryExists(const std::string& directory) const
{
    if( directory.empty() ) // the root directory
        return true;

    ASSERT(directory == PortableFunctions::PathRemoveTrailingSlash(directory) &&
           directory == PortableFunctions::PathToNativeSlash(directory));

    const auto& lookup = std::find_if(m_virtualDirectories.cbegin(), m_virtualDirectories.cend(),
        [&](const VirtualDirectory& vd)
        {
            ASSERT(vd.constructed_directory == PortableFunctions::PathEnsureTrailingSlash(vd.constructed_directory));

            if( SO::StartsWithNoCase(vd.constructed_directory, directory) )
            {
                ASSERT(vd.constructed_directory.length() > directory.length());
                return ( vd.constructed_directory[directory.length()] == Path::NativeSlashChar );
            }

            return false;
        });

    return ( lookup != m_virtualDirectories.cend() );
}


const std::vector<VirtualDirectoryLister::VirtualPath>& VirtualDirectoryLister::GetVirtualPaths(const std::string& directory) const
{
    ASSERT(DirectoryExists(directory));

    // used the cached paths when available
    const auto& lookup = m_cachedVirtualPaths.find(directory);

    if( lookup != m_cachedVirtualPaths.cend() )
        return lookup->second;

    // calculate the virtual paths...
    std::vector<VirtualPath> virtual_paths;

    // ...add the subdirectories
    AddSubdirectoriesForDirectory(virtual_paths, directory);

    // ...add the files as applicable
    const VirtualDirectory* virtual_directory = nullptr;

    if( m_virtualDirectories.size() == 1 )
    {
        ASSERT(directory.empty());
        virtual_directory = &m_virtualDirectories.front();
    }

    else
    {
        for( const VirtualDirectory& this_vd : m_virtualDirectories )
        {
            ASSERT(this_vd.constructed_directory == PortableFunctions::PathEnsureTrailingSlash(this_vd.constructed_directory));

            if( SO::StartsWithNoCase(this_vd.constructed_directory, directory) )
            {
                ASSERT(this_vd.constructed_directory.length() > directory.length());

                if( this_vd.constructed_directory[directory.length()] == Path::NativeSlashChar &&
                    this_vd.constructed_directory.length() == ( directory.length() + 1 ) )
                {
                    virtual_directory = &this_vd;
                    break;
                }
            }
        }
    }

    if( virtual_directory != nullptr )
        AddFilesForVirtualDirectory(virtual_paths, *virtual_directory);

    return m_cachedVirtualPaths.try_emplace(directory, std::move(virtual_paths)).first->second;
}


void VirtualDirectoryLister::AddSubdirectoriesForDirectory(std::vector<VirtualPath>& virtual_paths, const std::string& directory) const
{
    ASSERT(DirectoryExists(directory));

    const size_t initial_virtual_paths_size = virtual_paths.size();

    for( const VirtualDirectory& virtual_directory : m_virtualDirectories )
    {
        if( !SO::StartsWithNoCase(virtual_directory.constructed_directory, directory) )
            continue;

        const size_t expected_subdirectory_pos = directory.empty() ? 0 : ( directory.length() + 1 );
        ASSERT(expected_subdirectory_pos == 0 || virtual_directory.constructed_directory[expected_subdirectory_pos - 1] == Path::NativeSlashChar);
        const size_t post_subdirectory_slash_pos = virtual_directory.constructed_directory.find(Path::NativeSlashChar, expected_subdirectory_pos);

        if( post_subdirectory_slash_pos != std::string::npos && post_subdirectory_slash_pos < virtual_directory.constructed_directory.length() )
        {
            const std::string_view subdirectory_sv = std::string_view(virtual_directory.constructed_directory).substr(expected_subdirectory_pos,
                                                                                                                      post_subdirectory_slash_pos - expected_subdirectory_pos);

            if( std::find_if(virtual_paths.cbegin(), virtual_paths.cend(),
                             [&](const VirtualPath& virtual_path) { return SO::EqualsNoCase(virtual_path.path, subdirectory_sv); }) == virtual_paths.cend() )
            {
                virtual_paths.emplace_back(VirtualPath { std::string(subdirectory_sv), true });
                ASSERT(virtual_paths.back().path == PortableFunctions::PathRemoveTrailingSlash(virtual_paths.back().path));
            }
        }
    }

    if( initial_virtual_paths_size != virtual_paths.size() )
    {
        // sort the directories
        std::sort(virtual_paths.begin() + initial_virtual_paths_size, virtual_paths.end(),
            [](const VirtualPath& p1, const VirtualPath& p2)
            {
                ASSERT(p1.is_directory && p2.is_directory);
                return ( SO::CompareNoCase(p1.path, p2.path) < 0 );
            });

        // what was added was only the subdirectory, so now make them full directories
        if( !directory.empty() )
        {
            std::for_each(virtual_paths.begin() + initial_virtual_paths_size, virtual_paths.end(),
                [&](VirtualPath& virtual_path)
                {
                    virtual_path.path = Path::Combine(directory, virtual_path.path);
                });
        }
    }
}


void VirtualDirectoryLister::AddFilesForVirtualDirectory(std::vector<VirtualPath>& virtual_paths, const VirtualDirectory& virtual_directory) const
{
    const size_t initial_virtual_paths_size = virtual_paths.size();

    for( const std::string& file_path : virtual_directory.file_paths )
        virtual_paths.emplace_back(VirtualPath { file_path, false });

    // sort the directories
    if( initial_virtual_paths_size != virtual_paths.size() )
    {
        std::sort(virtual_paths.begin() + initial_virtual_paths_size, virtual_paths.end(),
            [](const VirtualPath& p1, const VirtualPath& p2)
            {
                ASSERT(!p1.is_directory && !p2.is_directory);
                return ( SO::CompareNoCase(p1.path, p2.path) < 0 );
            });
    }
}
