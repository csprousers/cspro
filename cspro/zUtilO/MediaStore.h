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
        constexpr const TCHAR* Media = _T("Media");
    }

    CLASS_DECL_ZUTILO const std::vector<const TCHAR*>& GetMediaTypeStrings();

    // returns a non-null pointer to the media filenames
    CLASS_DECL_ZUTILO std::shared_ptr<const std::vector<std::wstring>> GetMediaFilenames(MediaType media_type);
}

CLASS_DECL_ZUTILO const TCHAR* ToString(MediaStore::MediaType media_type);

template<> CLASS_DECL_ZUTILO std::optional<MediaStore::MediaType> FromString<MediaStore::MediaType>(wstring_view text);
