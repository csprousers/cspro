#pragma once

#include <zSyncO/zSyncO.h>
#include <zJson/JsonNode.h>


class SYNC_API SyncMessage
{
public:
    // Creates a sync message using the name and optional value (as a string or JSON).
    SyncMessage(int64_t timestamp, SharableString name, std::variant<SharableString, JsonNode> value);
    SyncMessage(SharableString name, SharableString value);
    SyncMessage(SharableString name, JsonNode value);

    int64_t GetTimestamp() const { return m_timestamp; }

    const SharableString& GetName() const { return m_name; }

    // Returns the value formatted for the OnSyncMessage logic function.
    SharableString GetValueForOnSyncMessage() const;

    // Returns the optional value formatted as JSON.
    std::optional<JsonNode> GetValueAsOptionalJson() const;
    SharableString GetValueAsOptionalJsonText() const;

    // Deserialization routines throw exceptions if not valid.
    static SyncMessage CreateFromJson(const JsonNode& json_node);
    void WriteJson(JsonWriter& json_writer) const;

    static SyncMessage CreateFromJsonFromBluetooth(const JsonNode& json_node);
    void WriteJsonForBluetooth(JsonWriter& json_writer) const;

private:
    void WriteJsonValueNode(JsonWriter& json_writer) const;

private:
    int64_t m_timestamp;
    SharableString m_name;
    std::variant<SharableString, JsonNode> m_value;
};
