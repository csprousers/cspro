#include "StdAfx.h"
#include "MediaStore.h"
#include <zPlatformO/PlatformInterface.h>
#include <zToolsO/ObjectTransporter.h>


namespace
{
    const std::vector<const TCHAR*> MediaTypeStrings =
    {
        _T("Audio"),
        _T("Images"),
        _T("Video"),
    };
}

const std::vector<const TCHAR*>& MediaStore::GetMediaTypeStrings()
{
    return MediaTypeStrings;
}


const TCHAR* ToString(MediaStore::MediaType media_type)
{
    size_t index = static_cast<size_t>(media_type);
    ASSERT(index < MediaTypeStrings.size());
    return MediaTypeStrings[index];
}


template<> std::optional<MediaStore::MediaType> FromString<MediaStore::MediaType>(wstring_view text)
{
    for( size_t i = 0; i < MediaTypeStrings.size(); ++i )
    {
        if( SO::Equals(text, MediaTypeStrings[i]) )
            return static_cast<MediaStore::MediaType>(i);
    }

    return std::nullopt;
}


std::shared_ptr<const std::vector<std::wstring>> MediaStore::GetMediaFilenames(MediaType media_type)
{
    // because querying the media filenames is a non-trivial task, the results will be cached
    struct MediaFilenamesCacheableObject : public CacheableObject
    {
        std::map<MediaType, std::shared_ptr<const std::vector<std::wstring>>> media_filenames_map;
    };

    MediaFilenamesCacheableObject& cache = ObjectTransporter::GetObjectCacher().GetOrCreate<MediaFilenamesCacheableObject>();

    const auto& cache_lookup = cache.media_filenames_map.find(media_type);

    if( cache_lookup != cache.media_filenames_map.cend() )
        return cache_lookup->second;

    std::unique_ptr<std::vector<std::wstring>> media_filenames;
    
#ifdef WIN_DESKTOP
    media_filenames = std::make_unique<std::vector<std::wstring>>();
#else
    media_filenames = std::make_unique<std::vector<std::wstring>>(PlatformInterface::GetInstance()->GetApplicationInterface()->GetMediaFilenames(media_type));
#endif

    return cache.media_filenames_map.try_emplace(media_type, std::move(media_filenames)).first->second;
}
