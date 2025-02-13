#pragma once

#include <zToolsO/zToolsO.h>


namespace Base64
{
    template<typename StringType>
    CLASS_DECL_ZTOOLSO StringType Encode(const void* contents, size_t size);

    template<typename StringType>
    StringType Encode(const std::vector<std::byte>& contents)
    {
        return Encode<StringType>(contents.data(), contents.size());
    }

    template<typename StringType, typename ReturnType = StringType>
    CLASS_DECL_ZTOOLSO ReturnType Decode(const StringType& encoded_string);
};
