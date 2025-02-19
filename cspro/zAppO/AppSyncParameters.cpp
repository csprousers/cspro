#include "stdafx.h"
#include "AppSyncParameters.h"


CREATE_JSON_KEY(server)


AppSyncParameters AppSyncParameters::CreateFromJson(const JsonNode& json_node)
{
    if( json_node.IsEmpty() )
        return AppSyncParameters();

    // 8.0 sync service details were written using the key "server"
    return AppSyncParameters
    {
        json_node.Contains(JK::server) ? json_node.Get<SyncConnectionString>(JK::server) : json_node.Get<SyncConnectionString>(JK::connection),
        json_node.Get<SyncDirection>(JK::direction)
    };
}


void AppSyncParameters::WriteJson(JsonWriter& json_writer) const
{
    json_writer.BeginObject();

    if( sync_connection_string.IsDefined() )
    {
        json_writer.Write(JK::connection, sync_connection_string)
                   .Write(JK::direction, sync_direction);
    }

    json_writer.EndObject();
}


void AppSyncParameters::serialize(Serializer& ar)
{
    if( ar.PredatesVersionIteration(Serializer::Iteration_8_1_000_1) )
    {
        sync_connection_string = SyncConnectionString(ar.Read<std::string>());
    }

    else
    {
        ar & sync_connection_string;
    }

    ar.SerializeEnum(sync_direction);
    ar.IgnoreUnusedVariable<std::string>(Serializer::Iteration_8_0_000_1); // m_csAppServerPath
}
