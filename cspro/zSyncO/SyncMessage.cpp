#include "stdafx.h"
#include "SyncMessage.h"


SyncMessage::SyncMessage(const int64_t timestamp, SharableString name, std::variant<SharableString, JsonNode> value)
    :   m_timestamp(timestamp),
        m_name(std::move(name)),
        m_value(std::move(value))
{
}


SyncMessage::SyncMessage(SharableString name, SharableString value)
    :   SyncMessage(::GetTimestamp<int64_t>(), std::move(name), std::move(value))
{
}


SyncMessage::SyncMessage(SharableString name, JsonNode value)
    :   SyncMessage(::GetTimestamp<int64_t>(), std::move(name), std::move(value))
{
}


SharableString SyncMessage::GetValueForOnSyncMessage() const
{
    if( std::holds_alternative<SharableString>(m_value) )
    {
        return std::get<SharableString>(m_value);
    }

    else
    {
        ASSERT(std::holds_alternative<JsonNode>(m_value));

        return std::get<JsonNode>(m_value).IsEmpty()  ? SharableString() :
               std::get<JsonNode>(m_value).IsString() ? std::get<JsonNode>(m_value).Get<SharableString>() :
                                                        std::get<JsonNode>(m_value).GetNodeAsSharableString();
    }
}


std::optional<JsonNode> SyncMessage::GetValueAsOptionalJson() const
{
    if( std::holds_alternative<SharableString>(m_value) )
    {
        if( std::get<SharableString>(m_value).IsSet() )
            return Json::Parse(Encoders::ToJsonString(std::get<SharableString>(m_value).GetString()));

        return std::nullopt;
    }

    else
    {
        ASSERT(std::holds_alternative<JsonNode>(m_value));

        return std::get<JsonNode>(m_value);
    }
}


SharableString SyncMessage::GetValueAsOptionalJsonText() const
{
    if( std::holds_alternative<SharableString>(m_value) )
    {
        if( std::get<SharableString>(m_value).IsSet() )
            return Encoders::ToJsonString(std::get<SharableString>(m_value).GetString());

        return SharableString();
    }

    else
    {
        ASSERT(std::holds_alternative<JsonNode>(m_value));

        return std::get<JsonNode>(m_value).IsEmpty() ? SharableString() :
                                                       std::get<JsonNode>(m_value).GetNodeAsSharableString();
    }
}


SyncMessage SyncMessage::CreateFromJson(const JsonNode& json_node)
{
    const bool use_key = ( !json_node.Contains(JK::name) && json_node.Contains(JK::key) ); // remove once 8.0 support is gone

    return SyncMessage(json_node.Contains(JK::timestamp) ? json_node.GetDate() : 0,
                       json_node.Get<SharableString>(use_key ? JK::key : JK::name),
                       json_node.GetOrEmpty(JK::value));
}


void SyncMessage::WriteJson(JsonWriter& json_writer) const
{
    ASSERT(m_timestamp != 0);

    json_writer.BeginObject()
               .WriteDate(JK::timestamp, m_timestamp)
               .Write(JK::name, m_name);

    WriteJsonValueNode(json_writer);

    json_writer.EndObject();
}


void SyncMessage::WriteJsonValueNode(JsonWriter& json_writer) const
{
    if( std::holds_alternative<SharableString>(m_value) )
    {
        json_writer.WriteIfHasValue(JK::value, std::get<SharableString>(m_value));
    }

    else
    {
        ASSERT(std::holds_alternative<JsonNode>(m_value));

        json_writer.WriteIfNotEmpty(JK::value, std::get<JsonNode>(m_value));
    }
}


#include <zToolsO/Serializer.h>
static_assert(Serializer::GetEarliestSupportedVersion() < Serializer::Iteration_8_1_000_1,
              "prior to 8.1, the Bluetooth code would check for the type of this message;"
              "once 8.0 is no longer supported, remove all of the below code, including references to 'key' as opposed to 'name'");
constexpr std::string_view TypeForBluetoothSync_sv = "single-string";


SyncMessage SyncMessage::CreateFromJsonFromBluetooth(const JsonNode& json_node)
{
    if( json_node.Contains(JK::type) && json_node.Get<std::string_view>(JK::type) != TypeForBluetoothSync_sv )
        throw JsonParseException("Unknown sync message type");

    return CreateFromJson(json_node);
}


void SyncMessage::WriteJsonForBluetooth(JsonWriter& json_writer) const
{
    json_writer.BeginObject()
               .Write(JK::type, TypeForBluetoothSync_sv)
               .WriteDate(JK::timestamp, m_timestamp)
               .Write(JK::name, m_name)
               .Write(JK::key, m_name); // remove once 8.0 support is gone

    WriteJsonValueNode(json_writer);

    json_writer.EndObject();
}
