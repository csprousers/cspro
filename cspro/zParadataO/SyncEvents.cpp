#include "stdafx.h"
#include "SyncEvents.h"
#include <zJson/Json.h>

using namespace Paradata;


// --------------------------------------------------------------------------
// SyncServiceInstance
// --------------------------------------------------------------------------

SyncServiceInstance::SyncServiceInstance(std::string device_id)
    :   m_deviceId(std::move(device_id)),
        m_instanceId(-1)
{
}



// --------------------------------------------------------------------------
// SyncEvent
// --------------------------------------------------------------------------

SyncEvent::SyncEvent(std::shared_ptr<const SyncServiceInstance> sync_service_instance)
    :   m_syncServiceInstance(std::move(sync_service_instance)),
        m_duration(0)
{
    ASSERT(m_syncServiceInstance != nullptr);
}


void SyncEvent::SetResultDuration()
{
    m_duration = ::GetTimestamp() - this->GetTimestamp();
}


void SyncEvent::SetResultFailure(std::string exception_text)
{
    SetResultDuration();
    m_exception = std::make_unique<std::string>(std::move(exception_text));
}


std::optional<long> SyncEvent::GetExceptionTextId(Log& log) const
{
    return log.AddNullableText(m_exception);
}



// --------------------------------------------------------------------------
// SyncConnectionEvent
// --------------------------------------------------------------------------

void SyncConnectionEvent::SetupTables(Log& log)
{
    Table& table = log.CreateTable(ParadataTable::SyncServiceInfo)
            .AddColumn("connection", Table::ColumnType::Text)
            .AddColumn("type", Table::ColumnType::Integer)
        ;

    for( SyncServiceType sync_service_type = SyncServiceType::Bluetooth;
         sync_service_type <= SyncServiceType::LocalFiles;
         sync_service_type = static_cast<SyncServiceType>(static_cast<int>(sync_service_type) + 1) )
    {
        table.AddCode(SyncServiceTypeToParadataInt(sync_service_type), ToString(sync_service_type));
    }

    log.CreateTable(ParadataTable::SyncServiceInstance)
            .AddColumn("sync_service_info", Table::ColumnType::Long)
            .AddColumn("deviceid", Table::ColumnType::Text)
            .AddColumn("connection_source", Table::ColumnType::Integer)
                    .AddCode(SyncRunner::ConnectionSource::Logic, "logic")
                    .AddCode(SyncRunner::ConnectionSource::ActionInvoker, "action_invoker")
        ;

    log.CreateTable(ParadataTable::SyncConnectionEvent)
            .AddColumn("sync_service_instance", Table::ColumnType::Long)
            .AddColumn("action", Table::ColumnType::Integer)
                    .AddCode(Action::Connect, "connect")
                    .AddCode(Action::Disconnect, "disconnect")
            .AddColumn("duration", Table::ColumnType::Double)
            .AddColumn("exception_text", Table::ColumnType::Long, true)
        ;
}


constexpr int SyncConnectionEvent::SyncServiceTypeToParadataInt(const SyncServiceType sync_service_type)
{
    return static_cast<int>(sync_service_type);
}


SyncConnectionEvent::SyncConnectionEvent(std::shared_ptr<const SyncServiceInstance> sync_service_instance, const Action action)
    :   SyncEvent(std::move(sync_service_instance)),
        m_action(action)
{
}


std::tuple<std::unique_ptr<SyncConnectionEvent>,
           std::shared_ptr<const SyncServiceInstance>> SyncConnectionEvent::CreateConnectEvent(std::string device_id,
                                                                                               const SyncRunner::ConnectionSource connection_source,
                                                                                               SyncConnectionString sync_connection_string)
{
    std::shared_ptr<SyncServiceInstance> sync_service_instance(new SyncServiceInstance(std::move(device_id)));

    SyncConnectionEvent* const event = new SyncConnectionEvent(sync_service_instance, Action::Connect);
    event->m_connectData.emplace(sync_service_instance, connection_source, std::move(sync_connection_string));

    return { std::unique_ptr<SyncConnectionEvent>(event), std::move(sync_service_instance) };
}


std::unique_ptr<SyncConnectionEvent> SyncConnectionEvent::CreateDisconnectEvent(std::shared_ptr<const SyncServiceInstance> sync_service_instance)
{
    return std::unique_ptr<SyncConnectionEvent>(new SyncConnectionEvent(std::move(sync_service_instance), Action::Disconnect));
}


void SyncConnectionEvent::Save(Log& log, long base_event_id) const
{
    if( m_action == Action::Connect )
    {
        ASSERT(m_connectData.has_value());

        Table& sync_service_info_table = log.GetTable(ParadataTable::SyncServiceInfo);
        long sync_service_info_id = 0;
        sync_service_info_table.Insert(&sync_service_info_id,
            std::get<SyncConnectionString>(*m_connectData).ToSafeString().c_str(),
            SyncServiceTypeToParadataInt(std::get<SyncConnectionString>(*m_connectData).GetType())
        );

        ASSERT(std::get<std::shared_ptr<SyncServiceInstance>>(*m_connectData)->m_instanceId == -1);

        Table& sync_service_instance_table = log.GetTable(ParadataTable::SyncServiceInstance);
        sync_service_instance_table.Insert(&std::get<std::shared_ptr<SyncServiceInstance>>(*m_connectData)->m_instanceId,
            sync_service_info_id,
            GetDeviceId().c_str(),
            static_cast<int>(std::get<SyncRunner::ConnectionSource>(*m_connectData))
        );
    }

    Table& sync_service_event_table = log.GetTable(ParadataTable::SyncConnectionEvent);
    sync_service_event_table.Insert(&base_event_id,
        GetSyncServiceInstanceId(),
        static_cast<int>(m_action),
        GetDuration(),
        GetOptionalValueOrNull(GetExceptionTextId(log))
    );
}



// --------------------------------------------------------------------------
// SyncMessageEvent
// --------------------------------------------------------------------------

void SyncMessageEvent::SetupTables(Log& log)
{
    log.CreateTable(ParadataTable::SyncMessageEvent)
            .AddColumn("sync_service_instance", Table::ColumnType::Long)
            .AddColumn("name_text", Table::ColumnType::Long)
            .AddColumn("value_text", Table::ColumnType::Long, true)
            .AddColumn("response_text", Table::ColumnType::Long, true)
            .AddColumn("duration", Table::ColumnType::Double)
            .AddColumn("exception_text", Table::ColumnType::Long, true)
        ;
}


SyncMessageEvent::SyncMessageEvent(std::shared_ptr<const SyncServiceInstance> sync_service_instance,
                                   SharableString message_name, SharableString message_value)
    :   SyncEvent(std::move(sync_service_instance)),
        m_name(std::move(message_name)),
        m_value(std::move(message_value))
{
    ASSERT(m_name.IsSet());
}


void SyncMessageEvent::SetResponse(const std::optional<JsonNode>& response_json_node)
{
    ASSERT(!m_response.IsSet());

    if( response_json_node.has_value() )
        m_response = response_json_node->GetNodeAsSharableString();
}


void SyncMessageEvent::Save(Log& log, long base_event_id) const
{
    Table& sync_message_event_table = log.GetTable(ParadataTable::SyncMessageEvent);
    sync_message_event_table.Insert(&base_event_id,
        GetSyncServiceInstanceId(),
        log.AddText(m_name.GetString()),
        GetOptionalValueOrNull(log.AddNullableText(m_value)),
        GetOptionalValueOrNull(log.AddNullableText(m_response)),
        GetDuration(),
        GetOptionalValueOrNull(GetExceptionTextId(log))
    );
}



// --------------------------------------------------------------------------
// SyncParadataEvent
// --------------------------------------------------------------------------

void SyncParadataEvent::SetupTables(Log& log)
{
    log.CreateTable(ParadataTable::SyncParadataEvent)
            .AddColumn("sync_service_instance", Table::ColumnType::Long)
            .AddColumn("direction", Table::ColumnType::Integer)
                    .AddCode(SyncDirection::Put, ToString(SyncDirection::Put))
                    .AddCode(SyncDirection::Get, ToString(SyncDirection::Get))
                    .AddCode(SyncDirection::Both, ToString(SyncDirection::Both))
            .AddColumn("path_text", Table::ColumnType::Long)
            .AddColumn("duration", Table::ColumnType::Double)
            .AddColumn("exception_text", Table::ColumnType::Long, true)
        ;
}


SyncParadataEvent::SyncParadataEvent(std::shared_ptr<const SyncServiceInstance> sync_service_instance,
                                     const SyncDirection sync_direction, std::string file_path)
    :   SyncEvent(std::move(sync_service_instance)),
        m_syncDirection(sync_direction),
        m_filePath(std::move(file_path))
{
}


void SyncParadataEvent::Save(Log& log, long base_event_id) const
{
    Table& sync_paradata_event_table = log.GetTable(ParadataTable::SyncParadataEvent);
    sync_paradata_event_table.Insert(&base_event_id,
        GetSyncServiceInstanceId(),
        static_cast<int>(m_syncDirection),
        log.AddText(m_filePath),
        GetDuration(),
        GetOptionalValueOrNull(GetExceptionTextId(log))
    );
}
