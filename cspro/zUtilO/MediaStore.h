#pragma once

#include <zUtilO/zUtilO.h>
#include <zUtilO/FromString.h>


namespace MediaStore
{
    // do not renumber
    enum class MediaType : int
    {        
        Audio  = 0,
        Images = 1,
        Video  = 2,
    };

    namespace Text
    {
        constexpr const char* Media = "Media";
    }

    CLASS_DECL_ZUTILO const std::vector<const char*>& GetMediaTypeStrings();

    // returns the media file paths
    CLASS_DECL_ZUTILO const std::vector<std::string>& GetMediaFilePaths(MediaType media_type);
}

CLASS_DECL_ZUTILO const char* ToString(MediaStore::MediaType media_type);

template<> CLASS_DECL_ZUTILO std::optional<MediaStore::MediaType> FromString<MediaStore::MediaType>(std::string_view text_sv);
