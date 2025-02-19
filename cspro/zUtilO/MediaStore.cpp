#include "StdAfx.h"
#include "MediaStore.h"
#include <zPlatformO/PlatformInterface.h>
#include <zToolsO/ObjectTransporter.h>


const std::vector<const char*>& MediaStore::GetMediaTypeStrings()
{
    static const std::vector<const char*> MediaTypeStrings =
    {
        "Audio",
        "Images",
        "Video",
    };

    return MediaTypeStrings;
}


const char* ToString(const MediaStore::MediaType media_type)
{
    const std::vector<const char*>& media_type_strings = MediaStore::GetMediaTypeStrings();
    const size_t index = static_cast<size_t>(media_type);
    ASSERT(index < media_type_strings.size());
    return media_type_strings[index];
}


template<> std::optional<MediaStore::MediaType> FromString<MediaStore::MediaType>(const std::string_view text_sv)
{
    const std::vector<const char*>& media_type_strings = MediaStore::GetMediaTypeStrings();

    for( size_t i = 0; i < media_type_strings.size(); ++i )
    {
        if( SO::Equals(text_sv, media_type_strings[i]) )
            return static_cast<MediaStore::MediaType>(i);
    }

    return std::nullopt;
}


const std::vector<std::string>& MediaStore::GetMediaFilePaths(const MediaType media_type)
{
    // because querying the media filenames is a non-trivial task, the results will be cached
    struct MediaFilenamesCacheableObject : public CacheableObject
    {
        std::map<MediaType, std::unique_ptr<const std::vector<std::string>>> media_file_paths_map;
    };

    MediaFilenamesCacheableObject& cache = ObjectTransporter::GetObjectCacher().GetOrCreate<MediaFilenamesCacheableObject>();

    const auto& cache_lookup = cache.media_file_paths_map.find(media_type);

    if( cache_lookup != cache.media_file_paths_map.cend() )
        return *cache_lookup->second;

    auto media_file_paths = std::make_unique<const std::vector<std::string>>(
#ifndef WIN_DESKTOP
        PlatformInterface::GetInstance()->GetApplicationInterface()->GetMediaFilePaths(media_type)
#endif
    );

    return *cache.media_file_paths_map.try_emplace(media_type, std::move(media_file_paths)).first->second;
}
