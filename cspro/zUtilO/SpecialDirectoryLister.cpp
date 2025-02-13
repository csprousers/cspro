#include "StdAfx.h"
#include "SpecialDirectoryLister.h"
#include "MimeType.h"
#include "VirtualDirectoryLister.h"
#include <zPlatformO/PlatformInterface.h>
#include <zToolsO/ObjectTransporter.h>
#include <zToolsO/VectorHelpers.h>
#include <zEngineO/Nodes/Path.h>


// --------------------------------------------------------------------------
// SpecialDirectoryLister subclasses in this file:
//
//     - RealDirectoryLister
//         - for listing a real directory on the disk
//
//     - NullDirectoryLister
//         - for listing nothing, which is used for Windows implementations
//           of Android-specific listers
//
//     - MediaStoreDirectoryLister
//         - for listing the files in the Android MediaStore;
//           the parent directory is the Android root;
//           this can be used on Windows as well, but will return an empty
//           list of files
// 
//     - AndroidRealDirectoryLister (Android only)
//         - for listing a real directory on the disk;
//           when the parent directory goes above the CSEntry or Downloads
//           directories, it is returned as the Android root
//
//     - AndroidRootDirectoryLister (Android only)
//         - for listing the Android root, which will show the CSEntry,
//           Downloads, and MediaStore directories
//
// --------------------------------------------------------------------------


#ifdef ANDROID

namespace
{
    enum class AndroidDirectory { CSEntry, Downloads };

    std::wstring GetAndroidDirectory(AndroidDirectory android_directory)
    {
        return PortableFunctions::PathRemoveTrailingSlash(( android_directory == AndroidDirectory::CSEntry ) ? PlatformInterface::GetInstance()->GetCSEntryDirectory() :
                                                                                                               GetDownloadsFolder());
    }
}

#endif // ANDROID



// --------------------------------------------------------------------------
// SpecialDirectoryLister static methods
// --------------------------------------------------------------------------

std::wstring SpecialDirectoryLister::EvaluateFilter(Nodes::Path::FilterType filter_type)
{
    const ContentType content_type = ( filter_type == Nodes::Path::FilterType::Audio )    ?   ContentType::Audio :
                                     ( filter_type == Nodes::Path::FilterType::Geometry ) ?   ContentType::Geometry :
                                   /*( filter_type == Nodes::Path::FilterType::Image )    ?*/ ContentType::Image;

    std::wstring filter;

    for( const TCHAR* extension : MimeType::GetExtensionsForSupportedContentType(content_type) )
        SO::AppendWithSeparator(filter, std::wstring(_T("*.")) + extension, _T(";"));

    return filter;
}


std::wstring SpecialDirectoryLister::EvaluateFilter(wstring_view filter_sv)
{
    // return if no special filter is used
    if( filter_sv.find(SpecialFilterPrefix) == wstring_view::npos )
        return filter_sv;

    std::wstring evaluated_filter;

    // process each filter component
    constexpr TCHAR FilterComponentSeparator = ';';

    for( const wstring_view& filter_component_sv : SO::SplitString<wstring_view>(filter_sv, FilterComponentSeparator, true, false) )
    {
        ASSERT(!filter_component_sv.empty());

        if( filter_component_sv.front() != SpecialFilterPrefix )
        {
            SO::AppendWithSeparator(evaluated_filter, filter_component_sv, FilterComponentSeparator);
        }

        // a valid component will look like: |FileType.Audio
        else
        {
            std::optional<Nodes::Path::FilterType> filter_type;
            wstring_view file_type_sv = Nodes::Path::Text::FileType;
            size_t expected_type_pos = 1 + file_type_sv.length() + 1;

            if( filter_component_sv.length() > expected_type_pos &&
                SO::StartsWith(filter_component_sv.substr(1), file_type_sv) &&
                filter_component_sv[expected_type_pos - 1] == '.' )
            {
                wstring_view type_sv = filter_component_sv.substr(expected_type_pos);
                filter_type = ( type_sv == Nodes::Path::Text::Audio )    ? std::make_optional(Nodes::Path::FilterType::Audio) :
                              ( type_sv == Nodes::Path::Text::Geometry ) ? std::make_optional(Nodes::Path::FilterType::Geometry) :
                              ( type_sv == Nodes::Path::Text::Image )    ? std::make_optional(Nodes::Path::FilterType::Image) :
                                                                           std::nullopt;
            }

            if( !filter_type.has_value() )
                throw CSProException(_T("The filter '%s' is not valid"), std::wstring(filter_component_sv).c_str());

            SO::AppendWithSeparator(evaluated_filter, EvaluateFilter(*filter_type), FilterComponentSeparator);
        }
    }

    return evaluated_filter;
}


SpecialDirectoryLister::SpecialDirectory SpecialDirectoryLister::EvaluateSpecialDirectory(wstring_view directory_sv)
{
    if( !IsSpecialDirectory(directory_sv) )
        return PortableFunctions::PathToNativeSlash(PortableFunctions::PathRemoveTrailingSlash(directory_sv));

    if( directory_sv == SpecialDirectoryAndroidRoot )
        return AndroidRoot();

    // process MediaStore directories, removing the special directory prefix
    directory_sv = directory_sv.substr(1);

    const wstring_view media_sv = MediaStore::Text::Media;
    const size_t expected_type_pos = media_sv.length() + 1;

    if( directory_sv.length() > expected_type_pos &&
        SO::StartsWith(directory_sv, media_sv) &&
        directory_sv[expected_type_pos - 1] == '.' )
    {
        // there might be a path as part of the directory; e.g.: |Media.Images/Safari Images
        wstring_view media_type_sv = directory_sv.substr(expected_type_pos);
        std::wstring subdirectory;
        const size_t slash_pos = media_type_sv.find_first_of(PortableFunctions::PathSlashChars);

        if( slash_pos != wstring_view::npos )
        {
            subdirectory = PortableFunctions::PathToNativeSlash(PortableFunctions::PathRemoveTrailingSlash(media_type_sv.substr(slash_pos + 1)));
            media_type_sv = media_type_sv.substr(0, slash_pos);
        }

        std::optional<MediaStore::MediaType> media_type = FromString<MediaStore::MediaType>(media_type_sv);

        if( media_type.has_value() )
            return MediaStoreDirectory { *media_type, std::move(subdirectory) };
    }

    throw CSProException(_T("The directory '%s' is not valid"), std::wstring(directory_sv).c_str());
}


bool SpecialDirectoryLister::SpecialDirectoryExists(const SpecialDirectory& special_directory, bool for_disk_paths_only_return_true_for_directories)
{
    if( std::holds_alternative<std::wstring>(special_directory) )
    {
        return for_disk_paths_only_return_true_for_directories ? PortableFunctions::FileIsDirectory(std::get<std::wstring>(special_directory)) :
                                                                 PortableFunctions::FileExists(std::get<std::wstring>(special_directory));
    }

    else if( std::holds_alternative<AndroidRoot>(special_directory) )
    {
        return OnAndroid();
    }

    else
    {
        ASSERT(std::holds_alternative<MediaStoreDirectory>(special_directory));
        const MediaStoreDirectory& media_store_directory = std::get<MediaStoreDirectory>(special_directory);

        if( media_store_directory.subdirectory.empty() )
            return true;

        std::shared_ptr<const VirtualDirectoryLister> virtual_directory_lister = GetVirtualDirectoryLister(media_store_directory.media_type);
        ASSERT(virtual_directory_lister != nullptr);

        return virtual_directory_lister->DirectoryExists(media_store_directory.subdirectory);
    }

}


void SpecialDirectoryLister::ValidateStartAndRootSpecialDirectories(const SpecialDirectory& start_directory, const SpecialDirectory& root_directory)
{
    if( start_directory.index() == root_directory.index() )
    {
        auto throw_exception = [&]()
        {
            throw CSProException(_T("The start directory ('%s') must be within the root directory ('%s')."), GetSpecialDirectoryPath(start_directory).c_str(),
                                                                                                             GetSpecialDirectoryPath(root_directory).c_str());
        };

        if( std::holds_alternative<std::wstring>(start_directory) )
        {
            if( !SO::StartsWithNoCase(std::get<std::wstring>(start_directory), std::get<std::wstring>(root_directory)) )
                throw_exception();
        }

        else if( std::holds_alternative<SpecialDirectoryLister::MediaStoreDirectory>(start_directory) )
        {
            const SpecialDirectoryLister::MediaStoreDirectory& start_media_store_directory = std::get<SpecialDirectoryLister::MediaStoreDirectory>(start_directory);
            const SpecialDirectoryLister::MediaStoreDirectory& root_media_store_directory = std::get<SpecialDirectoryLister::MediaStoreDirectory>(root_directory);

            if( start_media_store_directory.media_type != root_media_store_directory.media_type ||
                !SO::StartsWithNoCase(start_media_store_directory.subdirectory, root_media_store_directory.subdirectory) )
            {
                throw_exception();
            }
        }
    }

    else if( !std::holds_alternative<SpecialDirectoryLister::AndroidRoot>(root_directory) )
    {
        throw CSProException("You cannot specify a media directory along with a non-media directory.");
    }
}


std::wstring SpecialDirectoryLister::GetSpecialDirectoryName(const SpecialDirectory& special_directory)
{
    if( std::holds_alternative<std::wstring>(special_directory) )
    {
        ASSERT(std::get<std::wstring>(special_directory) == PortableFunctions::PathRemoveTrailingSlash(std::get<std::wstring>(special_directory)));
        return PortableFunctions::PathGetFilename(std::get<std::wstring>(special_directory));
    }

    else if( std::holds_alternative<AndroidRoot>(special_directory) )
    {
        return _T("Android");
    }

    else
    {
        ASSERT(std::holds_alternative<MediaStoreDirectory>(special_directory));
        const MediaStoreDirectory& media_store_directory = std::get<MediaStoreDirectory>(special_directory);

        return media_store_directory.subdirectory.empty() ? ToString(media_store_directory.media_type) :
                                                            PortableFunctions::PathGetFilename(media_store_directory.subdirectory);
    }
}


std::wstring SpecialDirectoryLister::GetSpecialDirectoryPath(const SpecialDirectory& special_directory)
{
    if( std::holds_alternative<std::wstring>(special_directory) )
    {
        ASSERT(std::get<std::wstring>(special_directory) == PortableFunctions::PathRemoveTrailingSlash(std::get<std::wstring>(special_directory)));
        return std::get<std::wstring>(special_directory);
    }

    else if( std::holds_alternative<AndroidRoot>(special_directory) )
    {
        return SpecialDirectoryAndroidRoot;
    }

    else
    {
        ASSERT(std::holds_alternative<MediaStoreDirectory>(special_directory));
        const MediaStoreDirectory& media_store_directory = std::get<MediaStoreDirectory>(special_directory);
        return CreateMediaStoreDirectoryPath(media_store_directory.media_type, media_store_directory.subdirectory);
    }
}


std::wstring SpecialDirectoryLister::CreateMediaStoreDirectoryPath(MediaStore::MediaType media_type, const std::wstring& subdirectory)
{
    std::wstring directory = SpecialDirectoryPrefix + std::wstring(MediaStore::Text::Media) + _T(".") + ToString(media_type);

    if( !subdirectory.empty() )
        directory = PortableFunctions::PathAppendToPath(directory, subdirectory);

#ifdef _DEBUG
    SpecialDirectory special_directory = EvaluateSpecialDirectory(directory);
    ASSERT(std::holds_alternative<MediaStoreDirectory>(special_directory) &&
           std::get<MediaStoreDirectory>(special_directory).media_type == media_type &&
           std::get<MediaStoreDirectory>(special_directory).subdirectory == subdirectory);
#endif

    return directory;
}


std::shared_ptr<const VirtualDirectoryLister> SpecialDirectoryLister::GetVirtualDirectoryLister(MediaStore::MediaType media_type)
{
    struct VirtualDirectoryListersCacheableObject : public CacheableObject
    {
        std::map<MediaStore::MediaType, std::shared_ptr<const VirtualDirectoryLister>> virtual_directory_listers_map;
    };

    VirtualDirectoryListersCacheableObject& cache = ObjectTransporter::GetObjectCacher().GetOrCreate<VirtualDirectoryListersCacheableObject>();

    const auto& cache_lookup = cache.virtual_directory_listers_map.find(media_type);

    if( cache_lookup != cache.virtual_directory_listers_map.cend() )
        return cache_lookup->second;

    std::shared_ptr<const std::vector<std::wstring>> media_filenames = MediaStore::GetMediaFilenames(media_type);
    ASSERT(media_filenames != nullptr);
    
    return cache.virtual_directory_listers_map.try_emplace(media_type, std::make_shared<VirtualDirectoryLister>(*media_filenames)).first->second;
}



// --------------------------------------------------------------------------
// RealDirectoryLister
// --------------------------------------------------------------------------

class RealDirectoryLister : public SpecialDirectoryLister
{
public:
    template<typename... Args>
    RealDirectoryLister(std::wstring directory, Args&&... directory_lister_args);

    std::vector<std::wstring> GetSpecialPaths() override;
    std::optional<std::wstring> GetParentDirectory() override;

protected:
    const std::wstring m_directory;
};


template<typename... Args>
RealDirectoryLister::RealDirectoryLister(std::wstring directory, Args&&... directory_lister_args)
    :   SpecialDirectoryLister(std::forward<Args>(directory_lister_args)...),
        m_directory(std::move(directory))
{
    ASSERT(m_directory == PortableFunctions::PathRemoveTrailingSlash(m_directory));

    if( !PortableFunctions::FileIsDirectory(m_directory) )
        throw CSProException(_T("The path is not a valid directory: %s"), m_directory.c_str());
}


std::vector<std::wstring> RealDirectoryLister::GetSpecialPaths()
{
    // DirectoryLister expects the directory with a trailing slash
    return DirectoryLister::GetPaths(PortableFunctions::PathEnsureTrailingSlash(m_directory));
}


std::optional<std::wstring> RealDirectoryLister::GetParentDirectory()
{
    const std::wstring parent_directory = PortableFunctions::PathGetDirectory(m_directory);
    const std::wstring parent_directory_with_trailing_slash_removed = PortableFunctions::PathRemoveTrailingSlash(parent_directory);

    if( parent_directory != parent_directory_with_trailing_slash_removed )
        return parent_directory_with_trailing_slash_removed;

    return std::nullopt;
}



// --------------------------------------------------------------------------
// NullDirectoryLister
// --------------------------------------------------------------------------

class NullDirectoryLister : public SpecialDirectoryLister
{
public:
    std::vector<std::wstring> GetSpecialPaths() override      { return { }; }
    std::optional<std::wstring> GetParentDirectory() override { return std::nullopt; }
};



// --------------------------------------------------------------------------
// MediaStoreDirectoryLister
// --------------------------------------------------------------------------

class MediaStoreDirectoryLister : public SpecialDirectoryLister
{
public:
    template<typename... Args>
    MediaStoreDirectoryLister(MediaStoreDirectory media_store_directory, Args&&... directory_lister_args);

    std::vector<std::wstring> GetSpecialPaths() override;
    std::optional<std::wstring> GetParentDirectory() override;

private:
    MediaStoreDirectory m_mediaStoreDirectory;
    std::shared_ptr<const VirtualDirectoryLister> m_virtualDirectoryLister;
};


template<typename... Args>
MediaStoreDirectoryLister::MediaStoreDirectoryLister(MediaStoreDirectory media_store_directory, Args&&... directory_lister_args)
    :   SpecialDirectoryLister(std::forward<Args>(directory_lister_args)...),
        m_mediaStoreDirectory(media_store_directory),
        m_virtualDirectoryLister(GetVirtualDirectoryLister(m_mediaStoreDirectory.media_type))
{
    ASSERT(m_virtualDirectoryLister != nullptr);

    if( !m_virtualDirectoryLister->DirectoryExists(m_mediaStoreDirectory.subdirectory) )
    {
        throw CSProException(_T("The path is not a valid directory: %s"), CreateMediaStoreDirectoryPath(m_mediaStoreDirectory.media_type,
                                                                                                        m_mediaStoreDirectory.subdirectory).c_str());
    }
}


std::vector<std::wstring> MediaStoreDirectoryLister::GetSpecialPaths()
{
    std::vector<std::wstring> paths;

    for( const VirtualDirectoryLister::VirtualPath& virtual_path : m_virtualDirectoryLister->GetVirtualPaths(m_mediaStoreDirectory.subdirectory) )
    {
        if( virtual_path.is_directory )
        {
            if( m_includeDirectories && ( !FilterDirectories() || MatchesNameFilter(PortableFunctions::PathGetFilename(virtual_path.path)) ) )
            {
                std::wstring& path = paths.emplace_back(CreateMediaStoreDirectoryPath(m_mediaStoreDirectory.media_type, virtual_path.path));
                ASSERT(path == PortableFunctions::PathRemoveTrailingSlash(path));

                if( m_includeTrailingSlashOnDirectories )
                    path = PortableFunctions::PathEnsureTrailingSlash(path);
            }

            if( m_recursive )
            {
                MediaStoreDirectoryLister media_store_directory_lister_for_subdirectory = *this;
                media_store_directory_lister_for_subdirectory.m_mediaStoreDirectory.subdirectory = virtual_path.path;

                VectorHelpers::Append(paths, media_store_directory_lister_for_subdirectory.GetSpecialPaths());
            }
        }

        else
        {
            if( m_includeFiles && ( !FilterFiles() || MatchesNameFilter(PortableFunctions::PathGetFilename(virtual_path.path)) ) )
                paths.emplace_back(virtual_path.path);
        }
    }

    return paths;
}


std::optional<std::wstring> MediaStoreDirectoryLister::GetParentDirectory()
{
    if( m_mediaStoreDirectory.subdirectory.empty() )
    {
        if constexpr(OnAndroid())
        {
            return SpecialDirectoryAndroidRoot;
        }

        else
        {
            return std::nullopt;
        }
    }

    else
    {
        const std::wstring parent_directory = PortableFunctions::PathRemoveTrailingSlash(PortableFunctions::PathGetDirectory(m_mediaStoreDirectory.subdirectory));
        ASSERT(m_virtualDirectoryLister->DirectoryExists(parent_directory));

        return CreateMediaStoreDirectoryPath(m_mediaStoreDirectory.media_type, parent_directory);
    }
}



#ifdef ANDROID

// --------------------------------------------------------------------------
// AndroidRealDirectoryLister
// --------------------------------------------------------------------------

class AndroidRealDirectoryLister : public RealDirectoryLister
{
public:
    using RealDirectoryLister::RealDirectoryLister;

    std::optional<std::wstring> GetParentDirectory() override;
};


std::optional<std::wstring> AndroidRealDirectoryLister::GetParentDirectory()
{
    std::optional<bool> parent_directory_is_android_root;

    auto check_directory = [&](AndroidDirectory android_directory)
    {
        const std::wstring directory = GetAndroidDirectory(android_directory);

        if( SO::StartsWithNoCase(m_directory, directory) )
            parent_directory_is_android_root = ( m_directory.length() <= directory.length() );
    };

    check_directory(AndroidDirectory::CSEntry);

    if( !parent_directory_is_android_root.has_value() )
        check_directory(AndroidDirectory::Downloads);

    if( parent_directory_is_android_root == false )
        return RealDirectoryLister::GetParentDirectory();

    return SpecialDirectoryAndroidRoot;
}



// --------------------------------------------------------------------------
// AndroidRootDirectoryLister
// --------------------------------------------------------------------------

class AndroidRootDirectoryLister : public SpecialDirectoryLister
{
public:
    using SpecialDirectoryLister::SpecialDirectoryLister;

    std::vector<std::wstring> GetSpecialPaths() override;
    std::optional<std::wstring> GetParentDirectory() override;
};


std::vector<std::wstring> AndroidRootDirectoryLister::GetSpecialPaths()
{
    std::vector<std::wstring> paths;

    auto add_real_directory = [&](AndroidDirectory android_directory)
    {
        const std::wstring directory = GetAndroidDirectory(android_directory);
        ASSERT(directory == PortableFunctions::PathRemoveTrailingSlash(directory));

        if( directory.empty() )
            return;

        if( m_includeDirectories && ( !FilterDirectories() || MatchesNameFilter(PortableFunctions::PathGetFilename(directory)) ) )
        {
            paths.emplace_back(m_includeTrailingSlashOnDirectories ? PortableFunctions::PathEnsureTrailingSlash(directory) :
                                                                     directory);
        }

        if( m_recursive )
        {
            // DirectoryLister expects the directory with a trailing slash
            return DirectoryLister::AddPaths(paths, PortableFunctions::PathEnsureTrailingSlash(directory));
        }
    };

    auto add_media_store_directory = [&](MediaStore::MediaType media_type)
    {
        const std::wstring media_type_text = ToString(media_type);

        if( m_includeDirectories && ( !FilterDirectories() || MatchesNameFilter(media_type_text) ) )
        {
            std::wstring& path = paths.emplace_back(CreateMediaStoreDirectoryPath(media_type, std::wstring()));
            ASSERT(path == PortableFunctions::PathRemoveTrailingSlash(path));

            if( m_includeTrailingSlashOnDirectories )
                path = PortableFunctions::PathEnsureTrailingSlash(path);
        }

        if( m_recursive )
        {
            MediaStoreDirectoryLister media_store_directory_lister(MediaStoreDirectory { media_type, std::wstring() }, *this);
            VectorHelpers::Append(paths, media_store_directory_lister.GetSpecialPaths());
        }
    };

    add_real_directory(AndroidDirectory::CSEntry);
    add_media_store_directory(MediaStore::MediaType::Audio);
    add_real_directory(AndroidDirectory::Downloads);
    add_media_store_directory(MediaStore::MediaType::Images);
    add_media_store_directory(MediaStore::MediaType::Video);

    return paths;
}


std::optional<std::wstring> AndroidRootDirectoryLister::GetParentDirectory()
{
    return std::nullopt;
}

#endif // ANDROID



// --------------------------------------------------------------------------
// SpecialDirectoryLister creation routines
// --------------------------------------------------------------------------

std::unique_ptr<SpecialDirectoryLister> SpecialDirectoryLister::CreateSpecialDirectoryLister(const SpecialDirectory& special_directory,
    bool recursive/* = false*/, bool include_files/* = true*/, bool include_directories /* = false*/,
    bool include_trailing_slash_on_directories/* = true*/, bool filter_directories/* = true*/)
{
    if( std::holds_alternative<std::wstring>(special_directory) )
    {
#ifdef ANDROID
        using RealDirectoryListerType = AndroidRealDirectoryLister;
#else
        using RealDirectoryListerType = RealDirectoryLister;
#endif

        return std::make_unique<RealDirectoryListerType>(std::get<std::wstring>(special_directory),
                                                         recursive, include_files, include_directories,
                                                         include_trailing_slash_on_directories, filter_directories);
    }

#ifdef ANDROID
    else if( std::holds_alternative<AndroidRoot>(special_directory) )
    {
        return std::make_unique<AndroidRootDirectoryLister>(recursive, include_files, include_directories,
                                                            include_trailing_slash_on_directories, filter_directories);
    }
#endif

    else if( std::holds_alternative<MediaStoreDirectory>(special_directory) )
    {
        return std::make_unique<MediaStoreDirectoryLister>(std::get<MediaStoreDirectory>(special_directory),
                                                           recursive, include_files, include_directories,
                                                           include_trailing_slash_on_directories, filter_directories);
    }

    else
    {
        ASSERT(OnWindows());
        return std::make_unique<NullDirectoryLister>();
    }
}
