#include "stdafx.h"
#include "MessageEvent.h"

using namespace Paradata;


void MessageEvent::SetupTables(Log& log)
{
    log.CreateTable(ParadataTable::MessageEvent)
            .AddColumn("source", Table::ColumnType::Integer)
                    .AddCode(Source::System, "system")
                    .AddCode(Source::Errmsg, "errmsg")
                    .AddCode(Source::Warning, "warning")
                    .AddCode(Source::Logtext, "logtext")
            .AddColumn("type", Table::ColumnType::Integer)
                    .AddCode(MessageType::Abort, "abort")
                    .AddCode(MessageType::Error, "error")
                    .AddCode(MessageType::Warning, "warning")
                    .AddCode(MessageType::User, "user")
            .AddColumn("number", Table::ColumnType::Integer)
            .AddColumn("message_text", Table::ColumnType::Long)
            .AddColumn("unformatted_message_text", Table::ColumnType::Long)
            .AddColumn("message_language_name", Table::ColumnType::Long)
            .AddColumn("display_duration", Table::ColumnType::Double, true)
            .AddColumn("return_value", Table::ColumnType::Integer, true)
        ;
}


MessageEvent::MessageEvent(const Source source, const MessageType message_type, const int message_number, SharableString message_text,
                           SharableString unformatted_message_text, std::shared_ptr<NamedObject> message_language)
    :   m_source(source),
        m_messageType(message_type),
        m_messageNumber(message_number),
        m_messageText(std::move(message_text)),
        m_unformattedMessageText(std::move(unformatted_message_text)),
        m_messageLanguage(std::move(message_language))
{
    ASSERT(m_messageText.IsSet() && m_unformattedMessageText.IsSet());
}


void MessageEvent::SetPostDisplayReturnValue(const int return_value)
{
    m_displayDuration = ::GetTimestamp<double>() - this->GetTimestamp();
    m_returnValue = return_value;
}


void MessageEvent::Save(Log& log, long base_event_id) const
{
    Table& message_event_table = log.GetTable(ParadataTable::MessageEvent);
    message_event_table.Insert(&base_event_id,
        static_cast<int>(m_source),
        static_cast<int>(m_messageType),
        m_messageNumber,
        log.AddText(*m_messageText),
        log.AddText(*m_unformattedMessageText),
        log.AddNamedObject(m_messageLanguage.get()),
        GetOptionalValueOrNull(m_displayDuration),
        GetOptionalValueOrNull(m_returnValue)
    );
}
