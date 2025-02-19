#pragma once

#include <zHtml/zHtml.h>
#include <zToolsO/SerializerHelper.h>


namespace AccessUrl
{
    class ZHTML_API SerializerHelper : public ::SerializerHelper::Helper
    {
    public:
        // marked as virtual so that users of the inline functions defined in this file do not need to be linked against zHtml
        virtual void WriteFileAccessUrl(JsonWriter& json_writer, InterfaceString file_path);
    };


    inline void WriteFileAccessUrl(JsonWriter& json_writer, InterfaceString file_path)
    {
        if( SO::IsWhitespace(file_path.GetString()) )
            return;

        auto access_url_serializer_helper = json_writer.GetSerializerHelper().Get<AccessUrl::SerializerHelper>();

        if( access_url_serializer_helper != nullptr )
            access_url_serializer_helper->WriteFileAccessUrl(json_writer, std::move(file_path));
    }
}
