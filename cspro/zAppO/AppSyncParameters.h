#pragma once

#include <zAppO/SyncTypes.h>

template<typename CharType> class JsonNode;
class JsonWriter;
class Serializer;


struct AppSyncParameters
{
    std::wstring server;
    SyncDirection sync_direction = SyncDirection::Put;

    // serialization
    static AppSyncParameters CreateFromJson(const JsonNode<wchar_t>& json_node);
    void WriteJson(JsonWriter& json_writer) const;
    void serialize(Serializer& ar);
};
