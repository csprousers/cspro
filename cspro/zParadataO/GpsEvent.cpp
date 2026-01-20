#include "stdafx.h"
#include "GpsEvent.h"

using namespace Paradata;


// --------------------------------------------------------------------------
// GpsEvent
// --------------------------------------------------------------------------

void GpsEvent::SetupTables(Log& log)
{
    log.CreateTable(ParadataTable::GpsInstance)
            .AddColumn("source", Table::ColumnType::Integer)
                    .AddCode(0, "logic")
                    .AddCode(1, "background_reading")
        ;

    log.CreateTable(ParadataTable::GpsReadingInstance)
            .AddColumn("latitude", Table::ColumnType::Double, true)
            .AddColumn("longitude", Table::ColumnType::Double, true)
            .AddColumn("altitude", Table::ColumnType::Double, true)
            .AddColumn("satellites", Table::ColumnType::Double, true)
            .AddColumn("accuracy", Table::ColumnType::Double, true)
            .AddColumn("readtime", Table::ColumnType::Double, true)
        ;

    log.CreateTable(ParadataTable::GpsReadRequestInstance)
            .AddColumn("max_read_duration", Table::ColumnType::Integer)
            .AddColumn("desired_accuracy", Table::ColumnType::Integer, true)
            .AddColumn("dialog_text", Table::ColumnType::Long, true)
            .AddColumn("read_duration", Table::ColumnType::Double)
        ;

    log.CreateTable(ParadataTable::GpsEvent)
            .AddColumn("gps_instance", Table::ColumnType::Long, true)
            .AddColumn("action", Table::ColumnType::Integer)
                    .AddCode(Action::Close, "close")
                    .AddCode(Action::Open, "open")
                    .AddCode(Action::Read, "read")
                    .AddCode(Action::ReadLast, "readlast")
                    .AddCode(Action::BackgroundReading, "background_reading")
                    .AddCode(Action::ReadInteractive, "readInteractive")
                    .AddCode(Action::Select, "select")
            .AddColumn("return_value", Table::ColumnType::Double, true)
            .AddColumn("gps_read_request_instance", Table::ColumnType::Long, true)
            .AddColumn("gps_reading_instance", Table::ColumnType::Long, true)
        ;
}


GpsEvent::GpsEvent(const Action action, std::unique_ptr<GpsReadingInstance> gps_reading_instance/* = nullptr*/)
    :   m_action(action),
        m_gpsReadingInstance(std::move(gps_reading_instance))
{
}


void GpsEvent::SetPostExecutionValues(const double return_value, std::unique_ptr<GpsReadingInstance> gps_reading_instance/* = nullptr*/)
{
    m_returnValue = return_value;

    ASSERT(m_gpsReadingInstance == nullptr);
    m_gpsReadingInstance = std::move(gps_reading_instance);
}


void GpsEvent::Save(Log& log, long base_event_id) const
{
    Action action = m_action;
    Log::Instance gps_instance = Log::Instance::Gps;

    auto change_background_action_to_savable_action = [&](const Action background_action, const Action saveable_action)
    {
        if( m_action == background_action )
        {
            action = saveable_action;
            gps_instance = Log::Instance::BackgroundGps;
            return true;
        }

        return false;
    };

    change_background_action_to_savable_action(Action::BackgroundOpen, Action::Open) ||
    change_background_action_to_savable_action(Action::BackgroundClose, Action::Close) ||
    change_background_action_to_savable_action(Action::BackgroundReading, Action::BackgroundReading);

    if( action == Action::Open )
    {
        Table& gps_instance_table = log.GetTable(ParadataTable::GpsInstance);
        long gps_instance_id = 0;
        gps_instance_table.Insert(&gps_instance_id,
            ( m_action == Action::Open ) ? 0 : 1
        );

        log.StartInstance(gps_instance, gps_instance_id);

        action = Action::Open;
    }

    std::optional<long> gps_reading_instance_id;

    if( m_gpsReadingInstance != nullptr )
    {
        Table& gps_reading_instance_table = log.GetTable(ParadataTable::GpsReadingInstance);
        gps_reading_instance_id.emplace(0);
        gps_reading_instance_table.Insert(&(*gps_reading_instance_id),
            GetOptionalValueOrNull(m_gpsReadingInstance->latitude),
            GetOptionalValueOrNull(m_gpsReadingInstance->longitude),
            GetOptionalValueOrNull(m_gpsReadingInstance->altitude),
            GetOptionalValueOrNull(m_gpsReadingInstance->satellites),
            GetOptionalValueOrNull(m_gpsReadingInstance->accuracy),
            GetOptionalValueOrNull(m_gpsReadingInstance->readtime)
        );
    }

    Table& gps_event_table = log.GetTable(ParadataTable::GpsEvent);
    gps_event_table.Insert(&base_event_id,
        GetOptionalValueOrNull(log.GetInstance(gps_instance)),
        static_cast<int>(action),
        GetOptionalValueOrNull(m_returnValue),
        GetOptionalValueOrNull(m_gpsReadRequestInstanceId),
        GetOptionalValueOrNull(gps_reading_instance_id)
    );

    if( action == Action::Close )
        log.StopInstance(gps_instance);
}



// --------------------------------------------------------------------------
// GpsReadRequestEvent
// --------------------------------------------------------------------------

GpsReadRequestEvent::GpsReadRequestEvent(const Action action, const int max_read_duration, std::optional<int> desired_accuracy, std::optional<std::string> dialog_text)
    :   GpsEvent(action),
        m_maxReadDuration(max_read_duration),
        m_desiredAccuracy(std::move(desired_accuracy)),
        m_dialogText(std::move(dialog_text)),
        m_readDuration(0)
{
}


void GpsReadRequestEvent::SetPostExecutionValues(const double return_value, std::unique_ptr<GpsReadingInstance> gps_reading_instance/* = nullptr*/)
{
    m_readDuration = ::GetTimestamp<double>() - this->GetTimestamp();

    GpsEvent::SetPostExecutionValues(return_value, std::move(gps_reading_instance));
}


void GpsReadRequestEvent::Save(Log& log, long base_event_id) const
{
    Table& gps_read_request_instance_table = log.GetTable(ParadataTable::GpsReadRequestInstance);
    m_gpsReadRequestInstanceId.emplace(0);
    gps_read_request_instance_table.Insert(&(*m_gpsReadRequestInstanceId),
        m_maxReadDuration,
        GetOptionalValueOrNull(m_desiredAccuracy),
        GetOptionalValueOrNull(log.AddNullableText(m_dialogText)),
        m_readDuration
    );

    GpsEvent::Save(log, base_event_id);
}
