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

    std::string GetAndroidDirectory(const AndroidDirectory android_directory)
    {
        return PortableFunctions::PathRemoveTrailingSlash(( android_directory == AndroidDirectory::CSEntry ) ? PlatformInterface::GetInstance()->GetCSEntryDirectory() :
                                                                                                               GetDownloadsDirectory());
    }
}

#endif // ANDROID



// --------------------------------------------------------------------------
// SpecialDirectoryLister static methods
// --------------------------------------------------------------------------

std::string SpecialDirectoryLister::EvaluateFilter(const Nodes::Path::FilterType filter_type)
{
    const ContentType content_type = ( filter_type == Nodes::Path::FilterType::Audio )    ?   ContentType::Audio :
                                     ( filter_type == Nodes::Path::FilterType::Geometry ) ?   ContentType::Geometry :
                                   /*( filter_type == Nodes::Path::FilterType::Image )    ?*/ ContentType::Image;

    std::string filter;

    for( const char* const extension : MimeType::GetExtensionsForSupportedContentType(content_type) )
        SO::AppendWithSeparator(filter, FileExtensions::CreateWildcard(extension), ";");

    return filter;
}


std::string SpecialDirectoryLister::EvaluateFilter(const std::string_view filter_sv)
{
    // return if no special filter is used
    if( filter_sv.find(SpecialFilterPrefix) == std::string_view::npos )
        return std::string(filter_sv);

    std::string evaluated_filter;

    // process each filter component
    constexpr char FilterComponentSeparator = ';';

    for( const std::string_view& filter_component_sv : SO::SplitString<std::string_view>(filter_sv, FilterComponentSeparator, true, false) )
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
            const std::string_view file_type_sv = Nodes::Path::Text::FileType;
            const size_t expected_type_pos = 1 + file_type_sv.length() + 1;

            if( filter_component_sv.length() > expected_type_pos &&
                SO::StartsWith(filter_component_sv.substr(1), file_type_sv) &&
                filter_component_sv[expected_type_pos - 1] == '.' )
            {
                const std::string_view type_sv = filter_component_sv.substr(expected_type_pos);
                filter_type = ( type_sv == Nodes::Path::Text::Audio )    ? std::make_optional(Nodes::Path::FilterType::Audio) :
                              ( type_sv == Nodes::Path::Text::Geometry ) ? std::make_optional(Nodes::Path::FilterType::Geometry) :
                              ( type_sv == Nodes::Path::Text::Image )    ? std::make_optional(Nodes::Path::FilterType::Image) :
                                                                           std::nullopt;
            }

            if( !filter_type.has_value() )
                throw CSProException("The filter '%s' is not valid", std::string(filter_component_sv).c_str());

            SO::AppendWithSeparator(evaluated_filter, EvaluateFilter(*filter_type), FilterComponentSeparator);
        }
    }

    return evaluated_filter;
}


SpecialDirectoryLister::SpecialDirectory SpecialDirectoryLister::EvaluateSpecialDirectory(std::string_view directory_sv)
{
    if( !IsSpecialDirectory(directory_sv) )
        return PortableFunctions::PathToNativeSlash(PortableFunctions::PathRemoveTrailingSlash(std::string(directory_sv)));

    if( directory_sv == SpecialDirectoryAndroidRoot_sv )
        return AndroidRoot();

    // process MediaStore directories, removing the special directory prefix
    directory_sv.remove_prefix(1);

    const std::string_view media_sv = MediaStore::Text::Media;
    const size_t expected_type_pos = media_sv.length() + 1;

    if( directory_sv.length() > expected_type_pos &&
        SO::StartsWith(directory_sv, media_sv) &&
        directory_sv[expected_type_pos - 1] == '.' )
    {
        // there might be a path as part of the directory; e.g.: |Media.Images/Safari Images
        std::string_view media_type_sv = directory_sv.substr(expected_type_pos);
        std::string subdirectory;
        const size_t slash_pos = media_type_sv.find_first_of(Path::SlashChars_sv);

        if( slash_pos != std::string_view::npos )
        {
            subdirectory = PortableFunctions::PathToNativeSlash(PortableFunctions::PathRemoveTrailingSlash(std::string(media_type_sv.substr(slash_pos + 1))));
            media_type_sv = media_type_sv.substr(0, slash_pos);
        }

        const std::optional<MediaStore::MediaType> media_type = FromString<MediaStore::MediaType>(media_type_sv);

        if( media_type.has_value() )
            return MediaStoreDirectory { *media_type, std::move(subdirectory) };
    }

    throw CSProException("The directory '%s' is not valid", std::string(directory_sv).c_str());
}


bool SpecialDirectoryLister::SpecialDirectoryExists(const SpecialDirectory& special_directory, const bool for_disk_paths_only_return_true_for_directories)
{
    if( std::holds_alternative<std::string>(special_directory) )
    {
        return for_disk_paths_only_return_true_for_directories ? PortableFunctions::FileIsDirectory(std::get<std::string>(special_directory)) :
                                                                 PortableFunctions::FileExists(std::get<std::string>(special_directory));
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
            throw CSProException("The start directory ('%s') must be within the root directory ('%s').",
                                 GetSpecialDirectoryPath(start_directory).c_str(),
                                 GetSpecialDirectoryPath(root_directory).c_str());
        };

        if( std::holds_alternative<std::string>(start_directory) )
        {
            if( !SO::StartsWithNoCase(std::get<std::string>(start_directory), std::get<std::string>(root_directory)) )
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


std::string SpecialDirectoryLister::GetSpecialDirectoryName(const SpecialDirectory& special_directory)
{
    if( std::holds_alternative<std::string>(special_directory) )
    {
        ASSERT(std::get<std::string>(special_directory) == PortableFunctions::PathRemoveTrailingSlash(std::get<std::string>(special_directory)));
        return PortableFunctions::PathGetFilename(std::get<std::string>(special_directory));
    }

    else if( std::holds_alternative<AndroidRoot>(special_directory) )
    {
        return "Android";
    }

    else
    {
        ASSERT(std::holds_alternative<MediaStoreDirectory>(special_directory));
        const MediaStoreDirectory& media_store_directory = std::get<MediaStoreDirectory>(special_directory);

        return media_store_directory.subdirectory.empty() ? ToString(media_store_directory.media_type) :
                                                            PortableFunctions::PathGetFilename(media_store_directory.subdirectory);
    }
}


std::string SpecialDirectoryLister::GetSpecialDirectoryPath(const SpecialDirectory& special_directory)
{
    if( std::holds_alternative<std::string>(special_directory) )
    {
        ASSERT(std::get<std::string>(special_directory) == PortableFunctions::PathRemoveTrailingSlash(std::get<std::string>(special_directory)));
        return std::get<std::string>(special_directory);
    }

    else if( std::holds_alternative<AndroidRoot>(special_directory) )
    {
        return std::string(SpecialDirectoryAndroidRoot_sv);
    }

    else
    {
        ASSERT(std::holds_alternative<MediaStoreDirectory>(special_directory));
        const MediaStoreDirectory& media_store_directory = std::get<MediaStoreDirectory>(special_directory);
        return CreateMediaStoreDirectoryPath(media_store_directory.media_type, media_store_directory.subdirectory);
    }
}


std::string SpecialDirectoryLister::CreateMediaStoreDirectoryPath(const MediaStore::MediaType media_type, const std::string& subdirectory)
{
    std::string directory = FormatText("%c%s.%s", SpecialDirectoryPrefix, MediaStore::Text::Media, ToString(media_type));

    if( !subdirectory.empty() )
        directory = Path::Combine(directory, subdirectory);

#ifdef _DEBUG
    const SpecialDirectory special_directory = EvaluateSpecialDirectory(directory);
    ASSERT(std::holds_alternative<MediaStoreDirectory>(special_directory) &&
           std::get<MediaStoreDirectory>(special_directory).media_type == media_type &&
           std::get<MediaStoreDirectory>(special_directory).subdirectory == subdirectory);
#endif

    return directory;
}


std::shared_ptr<const VirtualDirectoryLister> SpecialDirectoryLister::GetVirtualDirectoryLister(const MediaStore::MediaType media_type)
{
    struct VirtualDirectoryListersCacheableObject : public CacheableObject
    {
        std::map<MediaStore::MediaType, std::shared_ptr<const VirtualDirectoryLister>> virtual_directory_listers_map;
    };

    VirtualDirectoryListersCacheableObject& cache = ObjectTransporter::GetObjectCacher().GetOrCreate<VirtualDirectoryListersCacheableObject>();

    const auto& cache_lookup = cache.virtual_directory_listers_map.find(media_type);

    if( cache_lookup != cache.virtual_directory_listers_map.cend() )
        return cache_lookup->second;

    const std::vector<std::string>& media_file_paths = MediaStore::GetMediaFilePaths(media_type);

    return cache.virtual_directory_listers_map.try_emplace(media_type, std::make_unique<VirtualDirectoryLister>(media_file_paths)).first->second;
}



// --------------------------------------------------------------------------
// RealDirectoryLister
// --------------------------------------------------------------------------

class RealDirectoryLister : public SpecialDirectoryLister
{
public:
    template<typename... Args>
    RealDirectoryLister(std::string directory, Args&&... directory_lister_args);

    std::vector<std::string> GetSpecialPaths() override;
    std::optional<std::string> GetParentDirectory() override;

protected:
    std::string m_directory;
};


template<typename... Args>
RealDirectoryLister::RealDirectoryLister(std::string directory, Args&&... directory_lister_args)
    :   SpecialDirectoryLister(std::forward<Args>(directory_lister_args)...),
        m_directory(std::move(directory))
{
    ASSERT(m_directory == PortableFunctions::PathRemoveTrailingSlash(m_directory));

    if( !PortableFunctions::FileIsDirectory(m_directory) )
        throw CSProException("The path is not a valid directory: %s", m_directory.c_str());
}


std::vector<std::string> RealDirectoryLister::GetSpecialPaths()
{
    // DirectoryLister expects the directory with a trailing slash
    return DirectoryLister::GetPaths(PortableFunctions::PathEnsureTrailingSlash(m_directory));
}


std::optional<std::string> RealDirectoryLister::GetParentDirectory()
{
    const std::string parent_directory = PortableFunctions::PathGetDirectory(m_directory);
    const std::string parent_directory_with_trailing_slash_removed = PortableFunctions::PathRemoveTrailingSlash(parent_directory);

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
    std::vector<std::string> GetSpecialPaths() override      { return { }; }
    std::optional<std::string> GetParentDirectory() override { return std::nullopt; }
};



// --------------------------------------------------------------------------
// MediaStoreDirectoryLister
// --------------------------------------------------------------------------

class MediaStoreDirectoryLister : public SpecialDirectoryLister
{
public:
    template<typename... Args>
    MediaStoreDirectoryLister(MediaStoreDirectory media_store_directory, Args&&... directory_lister_args);

    std::vector<std::string> GetSpecialPaths() override;
    std::optional<std::string> GetParentDirectory() override;

private:
    MediaStoreDirectory m_mediaStoreDirectory;
    std::shared_ptr<const VirtualDirectoryLister> m_virtualDirectoryLister;
};


template<typename... Args>
MediaStoreDirectoryLister::MediaStoreDirectoryLister(const MediaStoreDirectory media_store_directory, Args&&... directory_lister_args)
    :   SpecialDirectoryLister(std::forward<Args>(directory_lister_args)...),
        m_mediaStoreDirectory(media_store_directory),
        m_virtualDirectoryLister(GetVirtualDirectoryLister(m_mediaStoreDirectory.media_type))
{
    ASSERT(m_virtualDirectoryLister != nullptr);

    if( !m_virtualDirectoryLister->DirectoryExists(m_mediaStoreDirectory.subdirectory) )
    {
        throw CSProException("The path is not a valid directory: %s",
                             CreateMediaStoreDirectoryPath(m_mediaStoreDirectory.media_type, m_mediaStoreDirectory.subdirectory).c_str());
    }
}


std::vector<std::string> MediaStoreDirectoryLister::GetSpecialPaths()
{
    std::vector<std::string> paths;

    for( const VirtualDirectoryLister::VirtualPath& virtual_path : m_virtualDirectoryLister->GetVirtualPaths(m_mediaStoreDirectory.subdirectory) )
    {
        if( virtual_path.is_directory )
        {
            if( m_includeDirectories && ( !FilterDirectories() || MatchesNameFilter(PortableFunctions::PathGetFilename(virtual_path.path)) ) )
            {
                std::string& path = paths.emplace_back(CreateMediaStoreDirectoryPath(m_mediaStoreDirectory.media_type, virtual_path.path));
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


std::optional<std::string> MediaStoreDirectoryLister::GetParentDirectory()
{
    if( m_mediaStoreDirectory.subdirectory.empty() )
    {
        if constexpr(OnAndroid())
        {
            return std::string(SpecialDirectoryAndroidRoot_sv);
        }

        else
        {
            return std::nullopt;
        }
    }

    else
    {
        const std::string parent_directory = PortableFunctions::PathRemoveTrailingSlash(PortableFunctions::PathGetDirectory(m_mediaStoreDirectory.subdirectory));
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

    std::optional<std::string> GetParentDirectory() override;
};


std::optional<std::string> AndroidRealDirectoryLister::GetParentDirectory()
{
    std::optional<bool> parent_directory_is_android_root;

    auto check_directory = [&](const AndroidDirectory android_directory)
    {
        const std::string directory = GetAndroidDirectory(android_directory);

        if( SO::StartsWithNoCase(m_directory, directory) )
            parent_directory_is_android_root = ( m_directory.length() <= directory.length() );
    };

    check_directory(AndroidDirectory::CSEntry);

    if( !parent_directory_is_android_root.has_value() )
        check_directory(AndroidDirectory::Downloads);

    if( parent_directory_is_android_root == false )
        return RealDirectoryLister::GetParentDirectory();

    return std::string(SpecialDirectoryAndroidRoot_sv);
}



// --------------------------------------------------------------------------
// AndroidRootDirectoryLister
// --------------------------------------------------------------------------

class AndroidRootDirectoryLister : public SpecialDirectoryLister
{
public:
    using SpecialDirectoryLister::SpecialDirectoryLister;

    std::vector<std::string> GetSpecialPaths() override;
    std::optional<std::string> GetParentDirectory() override;
};


std::vector<std::string> AndroidRootDirectoryLister::GetSpecialPaths()
{
    std::vector<std::string> paths;

    auto add_real_directory = [&](const AndroidDirectory android_directory)
    {
        const std::string directory = GetAndroidDirectory(android_directory);
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

    auto add_media_store_directory = [&](const MediaStore::MediaType media_type)
    {
        const std::string media_type_text = ToString(media_type);

        if( m_includeDirectories && ( !FilterDirectories() || MatchesNameFilter(media_type_text) ) )
        {
            std::string& path = paths.emplace_back(CreateMediaStoreDirectoryPath(media_type, SO::Empty_string));
            ASSERT(path == PortableFunctions::PathRemoveTrailingSlash(path));

            if( m_includeTrailingSlashOnDirectories )
                path = PortableFunctions::PathEnsureTrailingSlash(path);
        }

        if( m_recursive )
        {
            MediaStoreDirectoryLister media_store_directory_lister(MediaStoreDirectory { media_type, std::string() }, *this);
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


std::optional<std::string> AndroidRootDirectoryLister::GetParentDirectory()
{
    return std::nullopt;
}

#endif // ANDROID



// --------------------------------------------------------------------------
// SpecialDirectoryLister creation routines
// --------------------------------------------------------------------------

std::unique_ptr<SpecialDirectoryLister> SpecialDirectoryLister::CreateSpecialDirectoryLister(const SpecialDirectory& special_directory,
                                                                                             const bool recursive/* = false*/,
                                                                                             const bool include_files/* = true*/,
                                                                                             const bool include_directories /* = false*/,
                                                                                             const bool include_trailing_slash_on_directories/* = true*/,
                                                                                             const bool filter_directories/* = true*/)
{
    if( std::holds_alternative<std::string>(special_directory) )
    {
#ifdef ANDROID
        using RealDirectoryListerType = AndroidRealDirectoryLister;
#else
        using RealDirectoryListerType = RealDirectoryLister;
#endif

        return std::make_unique<RealDirectoryListerType>(std::get<std::string>(special_directory),
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
