#include "stdafx.h"
#include "AppSyncParameters.h"


CREATE_ENUM_JSON_SERIALIZER(SyncDirection,
    { SyncDirection::Put,  _T("put") },
    { SyncDirection::Get,  _T("get") },
    { SyncDirection::Both, _T("both") })


AppSyncParameters AppSyncParameters::CreateFromJson(const JsonNode<wchar_t>& json_node)
{
    return AppSyncParameters
    {
        json_node.Get<std::wstring>(JK::server),
        json_node.Get<SyncDirection>(JK::direction)
    };
}


void AppSyncParameters::WriteJson(JsonWriter& json_writer) const
{
    json_writer.BeginObject()
               .Write(JK::server, server)
               .Write(JK::direction, sync_direction)
               .EndObject();
}


void AppSyncParameters::serialize(Serializer& ar)
{
    ar & server;
    ar.SerializeEnum(sync_direction);
    ar.IgnoreUnusedVariable<std::wstring>(Serializer::Iteration_8_0_000_1); // m_csAppServerPath
}
