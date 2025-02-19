#pragma once

#include <zAppO/SyncTypes.h>
#include <zUtilO/SyncConnectionString.h>


struct AppSyncParameters
{
    SyncConnectionString sync_connection_string;
    SyncDirection sync_direction = SyncDirection::Put;

    // serialization
    static AppSyncParameters CreateFromJson(const JsonNode& json_node);
    void WriteJson(JsonWriter& json_writer) const;
    void serialize(Serializer& ar);
};
