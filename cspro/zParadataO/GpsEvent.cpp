#include "stdafx.h"
#include "GpsEvent.h"

namespace Paradata
{
    void GpsEvent::SetupTables(Log& log)
    {
        log.CreateTable(ParadataTable::GpsInstance)
                .AddColumn(_T("source"), Table::ColumnType::Integer)
                        .AddCode(0, _T("logic"))
                        .AddCode(1, _T("background_reading"))
            ;

        log.CreateTable(ParadataTable::GpsReadingInstance)
                .AddColumn(_T("latitude"), Table::ColumnType::Double, true)
                .AddColumn(_T("longitude"), Table::ColumnType::Double, true)
                .AddColumn(_T("altitude"), Table::ColumnType::Double, true)
                .AddColumn(_T("satellites"), Table::ColumnType::Double, true)
                .AddColumn(_T("accuracy"), Table::ColumnType::Double, true)
                .AddColumn(_T("readtime"), Table::ColumnType::Double, true)
            ;

        log.CreateTable(ParadataTable::GpsReadRequestInstance)
                .AddColumn(_T("max_read_duration"), Table::ColumnType::Integer)
                .AddColumn(_T("desired_accuracy"), Table::ColumnType::Integer, true)
                .AddColumn(_T("dialog_text"), Table::ColumnType::Long, true)
                .AddColumn(_T("read_duration"), Table::ColumnType::Double)
            ;

        log.CreateTable(ParadataTable::GpsEvent)
                .AddColumn(_T("gps_instance"), Table::ColumnType::Long, true)
                .AddColumn(_T("action"), Table::ColumnType::Integer)
                        .AddCode((int)Action::Close, _T("close"))
                        .AddCode((int)Action::Open, _T("open"))
                        .AddCode((int)Action::Read, _T("read"))
                        .AddCode((int)Action::ReadLast, _T("readlast"))
                        .AddCode((int)Action::BackgroundReading, _T("background_reading"))
                        .AddCode((int)Action::ReadInteractive, _T("readInteractive"))
                        .AddCode((int)Action::Select, _T("select"))
                .AddColumn(_T("return_value"), Table::ColumnType::Double, true)
                .AddColumn(_T("gps_read_request_instance"), Table::ColumnType::Long, true)
                .AddColumn(_T("gps_reading_instance"), Table::ColumnType::Long, true)
            ;
    }


    GpsEvent::GpsEvent(Action action, const std::optional<GpsReadingInstance>& gps_reading_instance/* = std::nullopt*/)
        :   m_action(action),
            m_gpsReadingInstance(gps_reading_instance)
    {
    }

    void GpsEvent::SetPostExecutionValues(double return_value, const std::optional<GpsReadingInstance>& gps_reading_instance/* = std::nullopt*/)
    {
        m_returnValue = return_value;

        ASSERT(!m_gpsReadingInstance.has_value());
        m_gpsReadingInstance = gps_reading_instance;
    }

    void GpsEvent::Save(Log& log, long base_event_id) const
    {
        Action action = m_action;
        Log::Instance gps_instance = Log::Instance::Gps;

        auto change_background_action_to_savable_action = [&](Action background_action, Action saveable_action) -> bool
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

        if( m_gpsReadingInstance.has_value() )
        {
            Table& gps_reading_instance_table = log.GetTable(ParadataTable::GpsReadingInstance);
            long id = 0;
            gps_reading_instance_table.Insert(&id,
                GetOptionalValueOrNull(m_gpsReadingInstance->latitude),
                GetOptionalValueOrNull(m_gpsReadingInstance->longitude),
                GetOptionalValueOrNull(m_gpsReadingInstance->altitude),
                GetOptionalValueOrNull(m_gpsReadingInstance->satellites),
                GetOptionalValueOrNull(m_gpsReadingInstance->accuracy),
                GetOptionalValueOrNull(m_gpsReadingInstance->readtime)
            );

            gps_reading_instance_id = id;
        }

        Table& gps_event_table = log.GetTable(ParadataTable::GpsEvent);
        gps_event_table.Insert(&base_event_id,
            GetOptionalValueOrNull(log.GetInstance(gps_instance)),
            (int)action,
            GetOptionalValueOrNull(m_returnValue),
            GetOptionalValueOrNull(m_gpsReadRequestInstanceId),
            GetOptionalValueOrNull(gps_reading_instance_id)
        );

        if( action == Action::Close )
            log.StopInstance(gps_instance);
    }


    GpsReadRequestEvent::GpsReadRequestEvent(Action action, int max_read_duration, const std::optional<int>& desired_accuracy,
        const std::optional<CString>& dialog_text)
        :   GpsEvent(action),
            m_maxReadDuration(max_read_duration),
            m_desiredAccuracy(desired_accuracy),
            m_dialogText(dialog_text),
            m_readDuration(0)
    {
    }

    void GpsReadRequestEvent::SetPostExecutionValues(double return_value, const std::optional<GpsReadingInstance>& gps_reading_instance/* = std::nullopt*/)
    {
        m_readDuration = ::GetTimestamp() - this->GetTimestamp();
        GpsEvent::SetPostExecutionValues(return_value, gps_reading_instance);
    }

    void GpsReadRequestEvent::Save(Log& log, long base_event_id) const
    {
        Table& gps_read_request_instance_table = log.GetTable(ParadataTable::GpsReadRequestInstance);
        long id = 0;
        gps_read_request_instance_table.Insert(&id,
            m_maxReadDuration,
            GetOptionalValueOrNull(m_desiredAccuracy),
            GetOptionalValueOrNull(log.AddNullableText(m_dialogText)),
            m_readDuration
        );

        m_gpsReadRequestInstanceId = id;

        GpsEvent::Save(log, base_event_id);
    }
}
