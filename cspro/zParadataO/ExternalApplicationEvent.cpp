#include "stdafx.h"
#include "ExternalApplicationEvent.h"

using namespace Paradata;


void ExternalApplicationEvent::SetupTables(Log& log)
{
    log.CreateTable(ParadataTable::ExternalApplicationEvent)
            .AddColumn(_T("source"), Table::ColumnType::Integer)
                    .AddCode((int)Source::ExecPff, _T("execpff"))
                    .AddCode((int)Source::ExecSystem, _T("execsystem"))
                    .AddCode((int)Source::SystemAppExec, _T("SystemApp.exec"))
            .AddColumn(_T("action"), Table::ColumnType::Text)
            .AddColumn(_T("stop"), Table::ColumnType::Boolean)
            .AddColumn(_T("success"), Table::ColumnType::Boolean)
            .AddColumn(_T("wait_duration"), Table::ColumnType::Double, true)
        ;
}


ExternalApplicationEvent::ExternalApplicationEvent(Source source, std::wstring action, bool stop)
    :   m_source(source),
        m_action(std::move(action)),
        m_stop(stop),
        m_success(false)
{
}


void ExternalApplicationEvent::SetPostExecutionValues(bool success, bool wait)
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
