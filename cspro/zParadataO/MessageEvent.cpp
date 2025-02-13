#include "stdafx.h"
#include "MessageEvent.h"


void Paradata::MessageEvent::SetupTables(Log& log)
{
    log.CreateTable(ParadataTable::MessageEvent)
            .AddColumn(_T("source"), Table::ColumnType::Integer)
                    .AddCode((int)Source::System, _T("system"))
                    .AddCode((int)Source::Errmsg, _T("errmsg"))
                    .AddCode((int)Source::Warning, _T("warning"))
                    .AddCode((int)Source::Logtext, _T("logtext"))
            .AddColumn(_T("type"), Table::ColumnType::Integer)
                    .AddCode((int)MessageType::Abort, _T("abort"))
                    .AddCode((int)MessageType::Error, _T("error"))
                    .AddCode((int)MessageType::Warning, _T("warning"))
                    .AddCode((int)MessageType::User, _T("user"))
            .AddColumn(_T("number"), Table::ColumnType::Integer)
            .AddColumn(_T("message_text"), Table::ColumnType::Long)
            .AddColumn(_T("unformatted_message_text"), Table::ColumnType::Long)
            .AddColumn(_T("message_language_name"), Table::ColumnType::Long)
            .AddColumn(_T("display_duration"), Table::ColumnType::Double, true)
            .AddColumn(_T("return_value"), Table::ColumnType::Integer, true)
        ;
}


Paradata::MessageEvent::MessageEvent(Source source, MessageType message_type, int message_number, std::wstring message_text,
                                     std::wstring unformatted_message_text, std::shared_ptr<NamedObject> message_language)
    :   m_source(source),
        m_messageType(message_type),
        m_messageNumber(message_number),
        m_messageText(std::move(message_text)),
        m_unformattedMessageText(std::move(unformatted_message_text)),
        m_messageLanguage(std::move(message_language))
{
}


void Paradata::MessageEvent::SetPostDisplayReturnValue(int return_value)
{
    m_displayDuration = ::GetTimestamp() - Event::GetTimestamp();
    m_returnValue = return_value;
}


void Paradata::MessageEvent::Save(Log& log, long base_event_id) const
{
    Table& message_event_table = log.GetTable(ParadataTable::MessageEvent);
    message_event_table.Insert(&base_event_id,
                               static_cast<int>(m_source),
                               static_cast<int>(m_messageType),
                               m_messageNumber,
                               log.AddText(m_messageText.c_str()),
                               log.AddText(m_unformattedMessageText.c_str()),
                               log.AddNamedObject(m_messageLanguage),
                               GetOptionalValueOrNull(m_displayDuration),
                               GetOptionalValueOrNull(m_returnValue)
    );
}
