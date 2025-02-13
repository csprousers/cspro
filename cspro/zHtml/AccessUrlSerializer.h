#pragma once

#include <zHtml/zHtml.h>
#include <zToolsO/SerializerHelper.h>

class JsonWriter;


namespace AccessUrl
{
    class ZHTML_API SerializerHelper : public ::SerializerHelper::Helper
    {
    public:
        // marked as virtual so that users of the inline functions defined in this file do not need to be linked against zHtml
        virtual void WriteFileAccessUrl(JsonWriter& json_writer, NullTerminatedStringView filename);
    };


    inline void WriteFileAccessUrl(JsonWriter& json_writer, NullTerminatedStringView filename)
    {
        if( SO::IsWhitespace(filename) )
            return;

        auto access_url_serializer_helper = json_writer.GetSerializerHelper().Get<AccessUrl::SerializerHelper>();

        if( access_url_serializer_helper != nullptr )        
            access_url_serializer_helper->WriteFileAccessUrl(json_writer, filename);
    }
}
