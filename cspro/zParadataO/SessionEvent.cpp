#include "stdafx.h"
#include "SessionEvent.h"

using namespace Paradata;


void SessionEvent::SetupTables(Log& log)
{
    log.CreateTable(ParadataTable::OperatorIdInfo)
            .AddColumn("operatorid", Table::ColumnType::Text)
            .AddIndex("operatorid_info_index", { 0 });
        ;

    log.CreateTable(ParadataTable::SessionInfo)
            .AddColumn("mode", Table::ColumnType::Integer, true)
                    .AddCode(1, "add")
                    .AddCode(2, "modify")
                    .AddCode(3, "verify")
            .AddColumn("operatorid_info", Table::ColumnType::Long, true)
        ;

    log.CreateTable(ParadataTable::SessionInstance)
            .AddColumn("session_info", Table::ColumnType::Long)
        ;

    log.CreateTable(ParadataTable::SessionEvent)
            .AddColumn("action", Table::ColumnType::Boolean)
                    .AddCode(0, "stop")
                    .AddCode(1, "start")
        ;
}


SessionEvent::SessionEvent(const bool start)
    :   m_start(start)
{
}


std::unique_ptr<SessionEvent> SessionEvent::CreateStartEvent()
{
    return std::unique_ptr<SessionEvent>(new SessionEvent(true));
}


std::unique_ptr<SessionEvent> SessionEvent::CreateStartEvent(const int mode, std::string operator_id)
{
    std::unique_ptr<SessionEvent> session_event(new SessionEvent(true));

    session_event->m_mode = mode;
    session_event->m_operatorId = std::move(operator_id);

    return session_event;
}


std::unique_ptr<SessionEvent> SessionEvent::CreateStopEvent()
{
    return std::unique_ptr<SessionEvent>(new SessionEvent(false));
}


bool SessionEvent::PreSave(Log& log) const
{
    if( m_start )
    {
        // fill the operator ID info table
        const std::optional<long> operator_id_info_id = m_operatorId.has_value() ? std::make_optional(log.AddOperatorIdInfo(*m_operatorId)) :
                                                                                   std::nullopt;

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
