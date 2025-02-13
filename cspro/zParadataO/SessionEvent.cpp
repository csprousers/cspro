#include "stdafx.h"
#include "SessionEvent.h"

namespace Paradata
{
    void SessionEvent::SetupTables(Log& log)
    {
        log.CreateTable(ParadataTable::OperatorIdInfo)
                .AddColumn(_T("operatorid"), Table::ColumnType::Text)
                .AddIndex(_T("operatorid_info_index"), { 0 });
            ;

        log.CreateTable(ParadataTable::SessionInfo)
                .AddColumn(_T("mode"), Table::ColumnType::Integer, true)
                        .AddCode(1, _T("add"))
                        .AddCode(2, _T("modify"))
                        .AddCode(3, _T("verify"))
                .AddColumn(_T("operatorid_info"), Table::ColumnType::Long, true)
            ;

        log.CreateTable(ParadataTable::SessionInstance)
                .AddColumn(_T("session_info"), Table::ColumnType::Long)
            ;

        log.CreateTable(ParadataTable::SessionEvent)
                .AddColumn(_T("action"), Table::ColumnType::Boolean)
                        .AddCode(0, _T("stop"))
                        .AddCode(1, _T("start"))
            ;
    }

    SessionEvent::SessionEvent(bool start)
        :   m_start(start)
    {
    }

    std::shared_ptr<SessionEvent> SessionEvent::CreateStartEvent()
    {
        return std::shared_ptr<SessionEvent>(new SessionEvent(true));
    }

    std::shared_ptr<SessionEvent> SessionEvent::CreateStartEvent(int mode, const CString& operator_id)
    {
        std::shared_ptr<SessionEvent> session_event(new SessionEvent(true));

        session_event->m_mode = mode;
        session_event->m_operatorId = operator_id;

        return session_event;
    }

    std::shared_ptr<SessionEvent> SessionEvent::CreateStopEvent()
    {
        return std::shared_ptr<SessionEvent>(new SessionEvent(false));
    }

    bool SessionEvent::PreSave(Log& log) const
    {
        if( m_start )
        {
            // fill the operator ID info table
            std::optional<long> operator_id_info_id = m_operatorId.has_value() ?
                (std::optional<long>)log.AddOperatorIdInfo(*m_operatorId) : std::nullopt;

            // fill the session info table
            Table& session_info_table = log.GetTable(ParadataTable::SessionInfo);
            long session_info_id = 0;
            session_info_table.Insert(&session_info_id,
                GetOptionalValueOrNull(m_mode),
                GetOptionalValueOrNull(operator_id_info_id)
            );

            // fill the session instance table
            Table& session_instance_table = log.GetTable(ParadataTable::SessionInstance);
            long session_instance_id = 0;
            session_instance_table.Insert(&session_instance_id,
                session_info_id
            );

            log.StartInstance(Log::Instance::Session, session_instance_id);
        }

        return log.GetInstance(Log::Instance::Session).has_value();
    }

    void SessionEvent::Save(Log& log, long base_event_id) const
    {
        Table& session_event_table = log.GetTable(ParadataTable::SessionEvent);
        session_event_table.Insert(&base_event_id,
            m_start
        );

        if( !m_start )
            log.StopInstance(Log::Instance::Session);
    }
}
