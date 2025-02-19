#include "stdafx.h"
#include "ExternalApplicationEvent.h"

using namespace Paradata;


void ExternalApplicationEvent::SetupTables(Log& log)
{
    log.CreateTable(ParadataTable::ExternalApplicationEvent)
            .AddColumn("source", Table::ColumnType::Integer)
                    .AddCode(Source::ExecPff, "execpff")
                    .AddCode(Source::ExecSystem, "execsystem")
                    .AddCode(Source::SystemAppExec, "SystemApp.exec")
            .AddColumn("action", Table::ColumnType::Text)
            .AddColumn("stop", Table::ColumnType::Boolean)
            .AddColumn("success", Table::ColumnType::Boolean)
            .AddColumn("wait_duration", Table::ColumnType::Double, true)
        ;
}


ExternalApplicationEvent::ExternalApplicationEvent(const Source source, std::string action, const bool stop)
    :   m_source(source),
        m_action(std::move(action)),
        m_stop(stop),
        m_success(false)
{
}


void ExternalApplicationEvent::SetPostExecutionValues(const bool success, const bool wait)
{
    m_success = success;

    if( m_success && wait )
        m_waitDuration = ::GetTimestamp() - this->GetTimestamp();
}


void ExternalApplicationEvent::Save(Log& log, long base_event_id) const
{
    Table& external_application_event_table = log.GetTable(ParadataTable::ExternalApplicationEvent);
    external_application_event_table.Insert(&base_event_id,
        static_cast<int>(m_source),
        m_action.c_str(),
        m_stop,
        m_success,
        GetOptionalValueOrNull(m_waitDuration)
    );
}
